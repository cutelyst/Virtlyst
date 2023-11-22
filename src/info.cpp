/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "info.h"
#include "virtlyst.h"

#include "lib/connection.h"
#include "lib/domain.h"

#include <libvirt/libvirt.h>

#include <QNetworkCookie>

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

using namespace Cutelyst;

Info::Info(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Info::hostusage(Context *c, const QString &hostId)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    int points = 5;
    QStringList timerArray = QString::fromLatin1(c->request()->cookie("timer")).split(u' ', Qt::SkipEmptyParts);
    QStringList cpuArray = QString::fromLatin1(c->request()->cookie("cpu")).split(u' ', Qt::SkipEmptyParts);
    QStringList memArray = QString::fromLatin1(c->request()->cookie("mem")).split(u' ', Qt::SkipEmptyParts);

    timerArray.append(QTime::currentTime().toString());
    cpuArray.append(QString::number(conn->allCpusUsage()));
    memArray.append(QString::number(conn->usedMemoryKiB() / 1024));

    if (timerArray.size() > points) {
        timerArray = timerArray.mid(timerArray.size() - points);
    }
    if (cpuArray.size() > points) {
        cpuArray = cpuArray.mid(cpuArray.size() - points);
    }
    if (memArray.size() > points) {
        memArray = memArray.mid(memArray.size() - points);
    }

    QJsonObject cpu {
        {QStringLiteral("labels"), QJsonArray::fromStringList(timerArray)},
        {QStringLiteral("datasets"), QJsonArray{
                QJsonObject{
                    {QStringLiteral("fillColor"), QStringLiteral("rgba(241,72,70,0.5)")},
                    {QStringLiteral("strokeColor"), QStringLiteral("rgba(241,72,70,1)")},
                    {QStringLiteral("pointColor"), QStringLiteral("rgba(241,72,70,1)")},
                    {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                    {QStringLiteral("data"), QJsonArray::fromStringList(cpuArray)},
                }
            }},
    };

    QJsonObject memory{
        {QStringLiteral("labels"), QJsonArray::fromStringList(timerArray)},
        {QStringLiteral("datasets"), QJsonArray{
                QJsonObject{
                    {QStringLiteral("fillColor"), QStringLiteral("rgba(249,134,33,0.5)")},
                    {QStringLiteral("strokeColor"), QStringLiteral("rgba(249,134,33,1)")},
                    {QStringLiteral("pointColor"), QStringLiteral("rgba(249,134,33,1)")},
                    {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                    {QStringLiteral("data"), QJsonArray::fromStringList(memArray)},
                }
            }
        },
    };

    c->response()->setJsonObjectBody({
                                         {QStringLiteral("cpu"), cpu},
                                         {QStringLiteral("memory"), memory},
                                     });
    c->response()->setCookie(QNetworkCookie("cpu", cpuArray.join(QLatin1Char(' ')).toLatin1()));
    c->response()->setCookie(QNetworkCookie("timer", timerArray.join(QLatin1Char(' ')).toLatin1()));
    c->response()->setCookie(QNetworkCookie("mem", memArray.join(QLatin1Char(' ')).toLatin1()));
}

void Info::insts_status(Context *c, const QString &hostId)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    QJsonArray vms;

    const QVector<Domain *> domains = conn->domains(
                VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_INACTIVE, c);
    for (Domain *domain : domains) {
        double difference = double(domain->memory()) / conn->memory();
        domain->setProperty("mem_usage", QString::number(difference * 100, 'g', 3));
        vms.append(QJsonObject{
                       {QStringLiteral("host"), hostId},
                       {QStringLiteral("uuid"), domain->uuid()},
                       {QStringLiteral("name"), domain->name()},
                       {QStringLiteral("dump"), 0},
                       {QStringLiteral("status"), domain->status()},
                       {QStringLiteral("memory"), domain->currentMemoryPretty()},
                       {QStringLiteral("vcpu"), domain->vcpu()},
                   });
    }

    c->response()->setJsonArrayBody(vms);
}

void Info::inst_status(Context *c, const QString &hostId, const QString &name)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    Domain *dom = conn->getDomainByName(name, c);
    if (dom) {
        c->response()->setJsonObjectBody({
                                             {QStringLiteral("status"), dom->status()},
                                         });
    } else {
        c->response()->setJsonObjectBody({
                                             {QStringLiteral("error"), QStringLiteral("Domain not found: no domain with matching name '%1'").arg(name)},
                                         });
    }
}

QStringList buildArrayFromCookie(const QString &cookie, qint64 value, int points)
{
    QStringList values = cookie.split(u' ', Qt::SkipEmptyParts);
    values.append(QString::number(value));

    if (values.size() > points) {
        values = values.mid(values.size() - points);
    }

    return values;
}

void Info::instusage(Context *c, const QString &hostId, const QString &name)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    Domain *dom = conn->getDomainByName(name, c);
    if (!dom) {
        c->response()->setJsonObjectBody({
                                             {QStringLiteral("error"), QStringLiteral("Domain not found: no domain with matching name '%1'").arg(name)},
                                         });
        return;
    }

    int points = 5;
    QStringList timerArray = QString::fromLatin1(c->request()->cookie("timer")).split(u' ', Qt::SkipEmptyParts);
    QStringList cpuArray = QString::fromLatin1(c->request()->cookie("cpu")).split(u' ', Qt::SkipEmptyParts);

    timerArray.append(QTime::currentTime().toString());
    cpuArray.append(QString::number(dom->cpuUsage()));

    if (timerArray.size() > points) {
        timerArray = timerArray.mid(timerArray.size() - points);
    }
    if (cpuArray.size() > points) {
        cpuArray = cpuArray.mid(cpuArray.size() - points);
    }

    QJsonObject cpu {
        {QStringLiteral("labels"), QJsonArray::fromStringList(timerArray)},
        {QStringLiteral("datasets"), QJsonArray{
                QJsonObject{
                    {QStringLiteral("fillColor"), QStringLiteral("rgba(241,72,70,0.5)")},
                    {QStringLiteral("strokeColor"), QStringLiteral("rgba(241,72,70,1)")},
                    {QStringLiteral("pointColor"), QStringLiteral("rgba(241,72,70,1)")},
                    {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                    {QStringLiteral("data"), QJsonArray::fromStringList(cpuArray)},
                }
            }},
    };
    c->response()->setCookie(QNetworkCookie("cpu", cpuArray.join(QLatin1Char(' ')).toLatin1()));

    QJsonArray net;
    const QVector<std::pair<qint64, qint64> > net_usage = dom->netUsageMiBs();

    int netDev = 0;
    for (const std::pair<qint64, qint64> &rx_tx : net_usage) {
        const auto cookieRx = "net-rx-" + QByteArray::number(netDev);
        const auto cookieTx = "net-tx-" + QByteArray::number(netDev);
        const QString rx = c->request()->cookie(cookieRx);
        const QString tx = c->request()->cookie(cookieTx);
        const QStringList rxArray = buildArrayFromCookie(rx, rx_tx.first, points);
        const QStringList txArray = buildArrayFromCookie(tx, rx_tx.second, points);
        c->response()->setCookie(QNetworkCookie(cookieRx, rxArray.join(QLatin1Char(' ')).toLatin1()));
        c->response()->setCookie(QNetworkCookie(cookieTx, txArray.join(QLatin1Char(' ')).toLatin1()));

        QJsonObject network {
            {QStringLiteral("labels"), QJsonArray::fromStringList(timerArray)},
            {QStringLiteral("datasets"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("fillColor"), QStringLiteral("rgba(83,191,189,0.5)")},
                        {QStringLiteral("strokeColor"), QStringLiteral("rgba(83,191,189,1)")},
                        {QStringLiteral("pointColor"), QStringLiteral("rgba(83,191,189,1)")},
                        {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                        {QStringLiteral("data"), QJsonArray::fromStringList(rxArray)},
                    },
                    QJsonObject{
                        {QStringLiteral("fillColor"), QStringLiteral("rgba(249,134,33,0.5)")},
                        {QStringLiteral("strokeColor"), QStringLiteral("rgba(249,134,33,1)")},
                        {QStringLiteral("pointColor"), QStringLiteral("rgba(1249,134,33,1)")},
                        {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                        {QStringLiteral("data"), QJsonArray::fromStringList(txArray)},
                    }
                }},
        };
        net.append(QJsonObject{
                       {QStringLiteral("dev"), netDev++},
                       {QStringLiteral("data"), network},
                   });
    }


    QJsonArray hdd;

    const QMap<QString, std::pair<qint64, qint64> > hdd_usage = dom->hddUsageMiBs();

    auto it = hdd_usage.constBegin();
    while (it != hdd_usage.constEnd()) {
        const std::pair<qint64, qint64> &rd_wr = it.value();
        const auto cookieRd = "hdd-rd-" + it.key().toLatin1();
        const auto cookieWr = "hdd-wr-" + it.key().toLatin1();
        const QString rd = c->request()->cookie(cookieRd);
        const QString wr = c->request()->cookie(cookieWr);
        const QStringList rdArray = buildArrayFromCookie(rd, rd_wr.first, points);
        const QStringList wrArray = buildArrayFromCookie(wr, rd_wr.second, points);
        c->response()->setCookie(QNetworkCookie(cookieRd, rdArray.join(u' ').toLatin1()));
        c->response()->setCookie(QNetworkCookie(cookieWr, wrArray.join(u' ').toLatin1()));

        QJsonObject network {
            {QStringLiteral("labels"), QJsonArray::fromStringList(timerArray)},
            {QStringLiteral("datasets"), QJsonArray{
                    QJsonObject{
                        {QStringLiteral("fillColor"), QStringLiteral("rgba(83,191,189,0.5)")},
                        {QStringLiteral("strokeColor"), QStringLiteral("rgba(83,191,189,1)")},
                        {QStringLiteral("pointColor"), QStringLiteral("rgba(83,191,189,1)")},
                        {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                        {QStringLiteral("data"), QJsonArray::fromStringList(rdArray)},
                    },
                    QJsonObject{
                        {QStringLiteral("fillColor"), QStringLiteral("rgba(151,187,205,0.5)")},
                        {QStringLiteral("strokeColor"), QStringLiteral("rgba(151,187,205,1)")},
                        {QStringLiteral("pointColor"), QStringLiteral("rgba(151,187,205,1)")},
                        {QStringLiteral("pointStrokeColor"), QStringLiteral("#fff")},
                        {QStringLiteral("data"), QJsonArray::fromStringList(wrArray)},
                    }
                }},
        };
        hdd.append(QJsonObject{
                       {QStringLiteral("dev"), it.key()},
                       {QStringLiteral("data"), network},
                   });

        ++it;
    }


    c->response()->setJsonObjectBody({
                                         {QStringLiteral("cpu"), cpu},
                                         {QStringLiteral("hdd"), hdd},
                                         {QStringLiteral("net"), net},
                                     });
    c->response()->setCookie(QNetworkCookie("timer", timerArray.join(QLatin1Char(' ')).toLatin1()));
}


