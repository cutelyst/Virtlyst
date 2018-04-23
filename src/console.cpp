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
#include "console.h"

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/domain.h"

#include <libvirt/libvirt.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(V_CONSOLE, "virtlyst.console")

Console::Console(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Console::index(Context *c, const QString &hostId, const QString &uuid)
{
    QStringList errors;
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qCWarning(V_CONSOLE) << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    Domain *dom = conn->getDomainByUuid(uuid, c);
    if (!dom) {
        errors.append(QStringLiteral("Domain not found: no domain with matching name '%1'").arg(uuid));
        c->setStash(QStringLiteral("errors"), errors);
        return;
    }

    const QString type = dom->consoleType();
    if (type == QLatin1String("spice")) {
        c->setStash(QStringLiteral("template"), QStringLiteral("console-spice.html"));
    } else if (type == QLatin1String("vnc")) {
        c->setStash(QStringLiteral("template"), QStringLiteral("console-vnc.html"));
    } else {
        qCDebug(V_CONSOLE) << "Console type not known for domain" << uuid;
        return;
    }
    const QUrl uri = c->request()->uri();
    c->setStash(QStringLiteral("domain"), QVariant::fromValue(dom));
    c->setStash(QStringLiteral("ws_host"), uri.host());
    c->setStash(QStringLiteral("ws_port"), uri.port(c->request()->secure() ? 443 : 80));
    c->setStash(QStringLiteral("console_passwd"), dom->consolePassword());
    const QString path = QLatin1String("ws/") + hostId + QLatin1Char('/') + uuid;
    c->setStash(QStringLiteral("ws_path"), path);
}
