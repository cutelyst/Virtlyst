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
    if (iface) {
        c->setStash(QStringLiteral("iface"), QVariant::fromValue(iface));
    }
}
