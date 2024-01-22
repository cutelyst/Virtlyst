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
#include "interfaces.h"

#include "lib/connection.h"
#include "lib/interface.h"
#include "lib/nodedevice.h"
#include "virtlyst.h"

#include <QLoggingCategory>

Interfaces::Interfaces(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Interfaces::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("interfaces.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(u"/index"));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create"))) {
            const QString delay      = params.value(u"delay"_qs);
            const QString ipv4_addr  = params.value(u"ipv4_addr"_qs);
            const QString ipv4_gw    = params.value(u"ipv4_gw"_qs);
            const QString ipv4_type  = params.value(u"ipv4_type"_qs);
            const QString ipv6_addr  = params.value(u"ipv6_addr"_qs);
            const QString ipv6_gw    = params.value(u"ipv6_gw"_qs);
            const QString ipv6_type  = params.value(u"ipv6_type"_qs);
            const QString itype      = params.value(u"itype"_qs);
            const QString name       = params.value(u"name"_qs);
            const QString netdev     = params.value(u"netdev"_qs);
            const QString start_mode = params.value(u"start_mode"_qs);
            const QString stp        = params.value(u"stp"_qs);
            conn->createInterface(name,
                                  netdev,
                                  itype,
                                  start_mode,
                                  delay.toInt(),
                                  stp == QLatin1String("on"),
                                  ipv4_addr,
                                  ipv4_gw,
                                  ipv4_type,
                                  ipv6_addr,
                                  ipv6_gw,
                                  ipv6_type);
        } else if (params.contains(QStringLiteral("stop"))) {
        }

        c->response()->redirect(
            c->uriFor(CActionFor(QStringLiteral("index")), QStringList{hostId}));
    }

    QVector<NodeDevice *> netdevs = conn->nodeDevices(VIR_CONNECT_LIST_NODE_DEVICES_CAP_NET, c);
    c->setStash(QStringLiteral("netdevs"), QVariant::fromValue(netdevs));

    const QVector<Interface *> ifaces = conn->interfaces(
        VIR_CONNECT_LIST_INTERFACES_INACTIVE | VIR_CONNECT_LIST_INTERFACES_ACTIVE, c);
    c->setStash(QStringLiteral("ifaces_all"), QVariant::fromValue(ifaces));
}

void Interfaces::interface(Context *c, const QString &hostId, const QString &ifaceName)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("interface.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    Interface *iface = conn->getInterface(ifaceName, c);
    if (!iface) {
        return;
    }

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("stop"))) {
            iface->stop();
        } else if (params.contains(QStringLiteral("start"))) {
            iface->start();
        } else if (params.contains(QStringLiteral("delete"))) {
            iface->undefine();
            c->response()->redirect(
                c->uriFor(CActionFor(QStringLiteral("index")), QStringList{hostId}));
            return;
        }
        c->response()->redirect(
            c->uriFor(CActionFor(QStringLiteral("interface")), QStringList{hostId, ifaceName}));
    }
    c->setStash(QStringLiteral("iface"), QVariant::fromValue(iface));
}
