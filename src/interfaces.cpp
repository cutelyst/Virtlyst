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

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/nodedevice.h"
#include "lib/interface.h"

Interfaces::Interfaces(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Interfaces::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("interfaces.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create"))) {
            const QString delay = params[QStringLiteral("delay")];
            const QString ipv4_addr = params[QStringLiteral("ipv4_addr")];
            const QString ipv4_gw = params[QStringLiteral("ipv4_gw")];
            const QString ipv4_type = params[QStringLiteral("ipv4_type")];
            const QString ipv6_addr = params[QStringLiteral("ipv6_addr")];
            const QString ipv6_gw = params[QStringLiteral("ipv6_gw")];
            const QString ipv6_type = params[QStringLiteral("ipv6_type")];
            const QString itype = params[QStringLiteral("itype")];
            const QString name = params[QStringLiteral("name")];
            const QString netdev = params[QStringLiteral("netdev")];
            const QString start_mode = params[QStringLiteral("start_mode")];
            const QString stp = params[QStringLiteral("stp")];
            conn->createInterface(name, netdev, itype, start_mode, delay.toInt(), stp == QLatin1String("on"),
                                  ipv4_addr, ipv4_gw, ipv4_type,
                                  ipv6_addr, ipv6_gw, ipv6_type);
        } else if (params.contains(QStringLiteral("stop"))) {

        }

        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
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
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
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
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
            return;
        }
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("interface")), QStringList{ hostId, ifaceName }));
    }
    c->setStash(QStringLiteral("iface"), QVariant::fromValue(iface));
}
