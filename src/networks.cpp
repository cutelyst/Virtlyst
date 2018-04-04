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

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/network.h"

Networks::Networks(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Networks::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("networks.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));


    QVariantList networks;
    virNetworkPtr *nets;
    int ret = virConnectListAllNetworks(conn->raw(), &nets, 0);
    if (ret > 0) {
        for (int i = 0; i < ret; ++i) {
            auto net = new Network(nets[i], conn, c);
            networks.append(QVariant::fromValue(net));
        }
        free(nets);
    }
    c->setStash(QStringLiteral("networks"), networks);
}
