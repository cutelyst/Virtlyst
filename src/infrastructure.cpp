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
#include "infrastructure.h"

#include "lib/connection.h"
#include "lib/domain.h"
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QDebug>

using namespace Cutelyst;

Infrastructure::Infrastructure(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Infrastructure::index(Context *c)
{
    QVariantList hosts;

    const QVector<ServerConn *> conns = m_virtlyst->servers(c);
    auto it                           = conns.constBegin();
    while (it != conns.constEnd()) {
        ServerConn *server = *it;
        Connection *conn   = server->conn;
        if (!conn) {
            ++it;
            continue;
        }

        double freeMemory = conn->freeMemoryBytes() / 1024; // To KibiBytes
        double difference = freeMemory / conn->memory();

        const QVector<Domain *> domains =
            conn->domains(VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_INACTIVE, c);
        for (Domain *domain : domains) {
            double difference = double(domain->memory()) / conn->memory();
            domain->setProperty("mem_usage", QString::number(difference * 100, 'g', 3));
        }

        hosts.append(QVariantHash{
            {QStringLiteral("id"), server->id},
            {QStringLiteral("name"), server->name},
            {QStringLiteral("status"), conn->isAlive()},
            {QStringLiteral("cpus"), conn->cpus()},
            {QStringLiteral("memory"), conn->memoryPretty()},
            {QStringLiteral("mem_usage"), QString::number(difference * 100, 'g', 3)},
            {QStringLiteral("vms"), QVariant::fromValue(domains)},
        });

        ++it;
    }

    c->setStash(QStringLiteral("hosts_vms"), hosts);
    c->setStash(QStringLiteral("template"), QStringLiteral("infrastructure.html"));
}
