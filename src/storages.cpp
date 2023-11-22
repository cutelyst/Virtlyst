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

#include "lib/connection.h"
#include "lib/secret.h"
#include "lib/storagepool.h"
#include "lib/storagevol.h"

#include "virtlyst.h"

#include <QLoggingCategory>

Storages::Storages(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Storages::index(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("storages.html"));
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
            QStringList errors;
            const QString name = params.value(u"name"_qs);
            const QString type = params.value(u"stg_type"_qs);
            if (type == QLatin1String("rbd")) {
                const QString ceph_pool = params.value(u"ceph_pool"_qs);
                const QString ceph_host = params.value(u"ceph_host"_qs);
                const QString ceph_user = params.value(u"ceph_user"_qs);
                const QString secret = params.value(u"secret"_qs);
                if (secret.isEmpty()) {
                    errors << QStringLiteral("You need create secret for pool");
                }

                if (errors.isEmpty()) {
                    conn->createStoragePoolCeph(name,
                                                ceph_pool,
                                                ceph_host,
                                                ceph_user,
                                                secret);
                }
            } else if (type ==  QLatin1String("netfs")) {
                const QString netfs_host = params.value(u"netfs_host"_qs);
                const QString source = params.value(u"source"_qs);
                const QString source_format = params.value(u"source_format"_qs);
                const QString target = params.value(u"target"_qs);
                conn->createStoragePoolNetFs(name,
                                             netfs_host,
                                             source,
                                             source_format,
                                             target);
            } else {
                const QString source = params.value(u"source"_qs);
                const QString target = params.value(u"target"_qs);
                conn->createStoragePool(name, type, source, target);
            }
        }

        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
        return;
    }

    const QVector<StoragePool *> storages = conn->storagePools(0, c);
    c->setStash(QStringLiteral("storages"), QVariant::fromValue(storages));

    const QVector<Secret *> secrets = conn->secrets(0, c);
    c->setStash(QStringLiteral("secrets"), QVariant::fromValue(secrets));
}

void Storages::storage(Context *c, const QString &hostId, const QString &pool)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("storage.html"));
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    StoragePool *storage = conn->getStoragePool(pool, c);
    if (!storage) {
        return;
    }

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("start"))) {
            storage->start();
        } else if (params.contains(QStringLiteral("stop"))) {
            storage->stop();
        } else if (params.contains(QStringLiteral("delete"))) {
            storage->undefine();
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index")), QStringList{ hostId }));
            return;
        } else if (params.contains(QStringLiteral("set_autostart"))) {
            storage->setAutostart(true);
        } else if (params.contains(QStringLiteral("unset_autostart"))) {
            storage->setAutostart(false);
        } else if (params.contains(QStringLiteral("add_volume"))) {
            const QString name = params.value(u"name"_qs);
            const QString size = params.value(u"size"_qs);
            const QString format = params.value(u"format"_qs);
            int flags = 0;
            if (params.contains(QStringLiteral("meta_prealloc")) && format == QLatin1String("qcow2")) {
                flags = VIR_STORAGE_VOL_CREATE_PREALLOC_METADATA;
            }
            StorageVol *vol = storage->createStorageVolume(name, format, size.toLongLong(), flags);
            if (!vol) {
                // failed to create storage
            }
        } else if (params.contains(QStringLiteral("del_volume"))) {
            const QString name = params.value(u"volname"_qs);
            StorageVol *vol = storage->getVolume(name);
            if (vol) {
                vol->undefine();
            }
        } else if (params.contains(QStringLiteral("cln_volume"))) {
            QString imageName = params.value(u"name"_qs) + QLatin1String(".img");
            const QString volName = params.value(u"image"_qs);
            QString format;
            int flags = 0;
            if (params.contains(QStringLiteral("convert"))) {
                format = params.value(u"format"_qs);
                if (params.contains(QStringLiteral("meta_prealloc")) && format == QLatin1String("qcow2")) {
                    flags = VIR_STORAGE_VOL_CREATE_PREALLOC_METADATA;
                }
            }
            StorageVol *vol = storage->getVolume(volName);
            if (vol) {
                vol->clone(imageName, format, flags);
            }
        }
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("storage")), QStringList{ hostId, pool }));
    }
    c->setStash(QStringLiteral("storage"), QVariant::fromValue(storage));
}
