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
#include "secrets.h"

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/secret.h"

#include <QLoggingCategory>

Secrets::Secrets(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Secrets::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("secrets.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create"))) {
            const QString data = params.value(QStringLiteral("data"));
            const QString ephemeral = params.value(QStringLiteral("ephemeral"));
            const QString priv = params.value(QStringLiteral("private"));
            const QString usage_type = params.value(QStringLiteral("usage_type"));

            conn->createSecret(ephemeral, usage_type, priv, data);
        } else if (params.contains(QStringLiteral("set_value"))) {
            const QString uuid = params.value(QStringLiteral("uuid"));
            Secret *secret = conn->getSecretByUuid(uuid, c);
            if (secret) {
                const QString value = params.value(QStringLiteral("value"));
                secret->setValue(value);
            }
        } else if (params.contains(QStringLiteral("delete"))) {
            conn->deleteSecretByUuid(params.value(QStringLiteral("uuid")));
        }
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
        return;
    }

    const QVector<Secret *> secrets = conn->secrets(0, c);
    c->setStash(QStringLiteral("secrets"), QVariant::fromValue(secrets));
}
