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
#include "storages.h"

#include "lib/storagepool.h"

#include "virtlyst.h"
#include "lib/connection.h"

Storages::Storages(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Storages::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("storages.html"));
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
        const QString name = params[QStringLiteral("name")];
        const QString type = params[QStringLiteral("stg_type")];

        if (params.contains(QStringLiteral("create"))) {
            conn->createStoragePool(name,
                                    type,
                                    params[QStringLiteral("source")],
                                    params[QStringLiteral("target")]);
        }

        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
        return;
    }

    const QVector<StoragePool *> storages = conn->storagePools(0, c);
    c->setStash(QStringLiteral("storages"), QVariant::fromValue(storages));
}

void Storages::storage(Context *c, const QString &hostId, const QString &pool)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("storage.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    StoragePool *storage = conn->getStoragePoll(pool, c);
    if (!storage) {
        return;
    }

    if (c->request()->isPost()) {
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("storage")), QStringList{ hostId, pool }));

        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("start"))) {
            storage->start();
        } else if (params.contains(QStringLiteral("stop"))) {
            storage->stop();
        } else if (params.contains(QStringLiteral("delete"))) {
            storage->undefine();
        } else if (params.contains(QStringLiteral("set_autostart"))) {
            storage->setAutostart(true);
        } else if (params.contains(QStringLiteral("unset_autostart"))) {
            storage->setAutostart(false);
        }
    }
    c->setStash(QStringLiteral("storage"), QVariant::fromValue(storage));
}
