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
#include "overview.h"

#include "lib/connection.h"
#include "virtlyst.h"

#include <QLoggingCategory>

Overview::Overview(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Overview::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("hostdetail.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));
}
