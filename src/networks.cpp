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
#include "networks.h"

#include "lib/connection.h"
#include "lib/network.h"
#include "virtlyst.h"

#include <QNetworkAddressEntry>

Networks::Networks(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Networks::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("networks.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    if (c->request()->isPost()) {
        c->response()->redirect(
            c->uriFor(CActionFor(QStringLiteral("index")), QStringList{hostId}));

        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create"))) {
            const QString name     = params.value(u"name"_qs);
            const QString forward  = params.value(u"forward"_qs);
            const QString subnet   = params.value(u"subnet"_qs);
            const QString bridge   = params.value(u"bridge_name"_qs);
            const bool openvswitch = params.contains(QStringLiteral("openvswitch"));
            const bool dhcp        = params.contains(QStringLiteral("openvswitch"));
            const bool fixed       = params.contains(QStringLiteral("openvswitch"));
            QNetworkAddressEntry entry;
            entry.setIp(QHostAddress(subnet.section(QLatin1Char('/'), 0, 0)));
            entry.setPrefixLength(subnet.section(QLatin1Char('/'), -1).toInt());
            conn->createNetwork(name,
                                forward,
                                entry.ip().toString(),
                                entry.netmask().toString(),
                                bridge,
                                dhcp,
                                openvswitch,
                                fixed);
        }
    }

    const QVector<Network *> networks = conn->networks(0, c);
    c->setStash(QStringLiteral("networks"), QVariant::fromValue(networks));
}

void Networks::network(Context *c, const QString &hostId, const QString &netName)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("network.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    Network *network = conn->getNetwork(netName, c);
    if (!network) {
        c->response()->redirect(
            c->uriFor(CActionFor(QStringLiteral("index")), QStringList{hostId}));
        return;
    }
    c->setStash(QStringLiteral("network"), QVariant::fromValue(network));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("start"))) {
            network->start();
        } else if (params.contains(QStringLiteral("stop"))) {
            network->stop();
        } else if (params.contains(QStringLiteral("delete"))) {
            network->undefine();
            c->response()->redirect(
                c->uriFor(CActionFor(QStringLiteral("index")), QStringList{hostId}));
            return;
        } else if (params.contains(QStringLiteral("set_autostart"))) {
            network->setAutostart(true);
        } else if (params.contains(QStringLiteral("unset_autostart"))) {
            network->setAutostart(false);
        }
        c->response()->redirect(
            c->uriFor(CActionFor(QStringLiteral("network")), QStringList{hostId, netName}));
    }
}
