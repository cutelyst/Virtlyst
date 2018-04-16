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
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    int points = 5;
    QStringList timerArray = c->request()->cookie(QStringLiteral("timer")).split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList cpuArray = c->request()->cookie(QStringLiteral("cpu")).split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList memArray = c->request()->cookie(QStringLiteral("mem")).split(QLatin1Char(' '), QString::SkipEmptyParts);

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
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
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
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
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

void Info::instusage(Context *c, const QString &hostId, const QString &name)
{
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
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
    QStringList timerArray = c->request()->cookie(QStringLiteral("timer")).split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList cpuArray = c->request()->cookie(QStringLiteral("cpu")).split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList hddArray = c->request()->cookie(QStringLiteral("hdd")).split(QLatin1Char(' '), QString::SkipEmptyParts);
    QStringList netArray = c->request()->cookie(QStringLiteral("net")).split(QLatin1Char(' '), QString::SkipEmptyParts);

    timerArray.append(QTime::currentTime().toString());
    cpuArray.append(QString::number(dom->cpuUsage()));

    if (timerArray.size() > points) {
        timerArray = timerArray.mid(timerArray.size() - points);
    }
    if (cpuArray.size() > points) {
        cpuArray = cpuArray.mid(cpuArray.size() - points);
    }
    if (hddArray.size() > points) {
        hddArray = hddArray.mid(hddArray.size() - points);
    }
    if (netArray.size() > points) {
        netArray = netArray.mid(netArray.size() - points);
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

    QJsonObject net;


    QJsonObject hdd;


    c->response()->setJsonObjectBody({
                                         {QStringLiteral("cpu"), cpu},
                                         {QStringLiteral("hdd"), hdd},
                                         {QStringLiteral("net"), net},
                                     });
    c->response()->setCookie(QNetworkCookie("timer", timerArray.join(QLatin1Char(' ')).toLatin1()));
    c->response()->setCookie(QNetworkCookie("cpu", cpuArray.join(QLatin1Char(' ')).toLatin1()));
    c->response()->setCookie(QNetworkCookie("hdd", cpuArray.join(QLatin1Char(' ')).toLatin1()));
    c->response()->setCookie(QNetworkCookie("net", cpuArray.join(QLatin1Char(' ')).toLatin1()));
}


