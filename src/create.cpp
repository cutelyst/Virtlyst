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
#include "create.h"

#include "lib/connection.h"
#include "lib/domain.h"
#include "lib/network.h"
#include "lib/storagepool.h"
#include "lib/storagevol.h"
#include "virtlyst.h"

#include <Cutelyst/Plugins/StatusMessage>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QDomDocument>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

using namespace Cutelyst;

Create::Create(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Create::index(Context *c, const QString &hostId)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qWarning() << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("template"), QStringLiteral("create.html"));

    const QVector<Domain *> domains =
        conn->domains(VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_INACTIVE, c);
    c->setStash(QStringLiteral("instances"), QVariant::fromValue(domains));

    if (c->request()->isPost()) {
        QStringList errors;
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create_flavor"))) {
            const QString label = params.value(u"label"_qs);
            const int memory    = params.value(u"memory"_qs).toInt();
            const int vcpu      = params.value(u"vcpu"_qs).toInt();
            const int disk      = params.value(u"disk"_qs).toInt();
            QSqlQuery query =
                CPreparedSqlQueryThreadForDB(QStringLiteral("INSERT INTO create_flavor "
                                                            "(label, memory, vcpu, disk) "
                                                            "VALUES "
                                                            "(:label, :memory, :vcpu, :disk)"),
                                             QStringLiteral("virtlyst"));
            if (!Virtlyst::createDbFlavor(query, label, memory, vcpu, disk)) {
                qWarning() << "Failed to create flavor" << label
                           << query.lastError().databaseText();
            }
        } else if (params.contains(QStringLiteral("delete_flavor"))) {
            const QString id = params.value(u"flavor"_qs);
            QSqlQuery query  = CPreparedSqlQueryThreadForDB(
                QStringLiteral("DELETE FROM create_flavor WHERE id = :id"),
                QStringLiteral("virtlyst"));
            query.bindValue(QStringLiteral(":id"), id);
            if (!query.exec()) {
                qWarning() << "Failed to delete flavor" << id << query.lastError().databaseText();
            }
        } else if (params.contains(QStringLiteral("create_xml"))) {
            const QString xml = params.value(u"from_xml"_qs);
            QDomDocument xmlDoc;
            if (xmlDoc.setContent(xml)) {
                const QString name = xmlDoc.documentElement()
                                         .firstChildElement(QStringLiteral("name"))
                                         .firstChild()
                                         .nodeValue();
                bool found = false;
                for (Domain *dom : domains) {
                    if (dom->name() == name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (conn->domainDefineXml(xml)) {
                        c->response()->redirect(
                            c->uriFor(QStringLiteral("/instances"), QStringList{hostId, name}));
                        return;
                    }
                    errors.append(QStringLiteral("Failed to create virtual machine:"));
                    errors.append(conn->lastError());
                } else {
                    errors.append(
                        QStringLiteral("A virtual machine with this name already exists"));
                }
            } else {
                errors.append(QStringLiteral("Invalid XML"));
            }
        } else if (params.contains(QStringLiteral("create"))) {
            const QString name         = params.value(u"name"_qs);
            const QString memory       = params.value(u"memory"_qs);
            const QString vcpu         = params.value(u"vcpu"_qs);
            const bool hostModel       = params.contains(QStringLiteral("host_model"));
            const QString cacheMode    = params.value(u"cache_mode"_qs);
            const bool virtio          = params.contains(QStringLiteral("virtio"));
            const QString consoleType  = QStringLiteral("spice");
            const QStringList networks = params.values(QStringLiteral("network-control"));

            int flags = 0;
            if (params.contains(QStringLiteral("meta_prealloc"))) {
                flags = VIR_STORAGE_VOL_CREATE_PREALLOC_METADATA;
            }

            QVector<StorageVol *> volumes;
            if (params.contains(QStringLiteral("hdd_size"))) {
                const QString storageName = params.value(u"storage"_qs);
                const QString hddSize     = params.value(u"hdd_size"_qs);
                StoragePool *storage      = conn->getStoragePool(storageName);
                if (storage) {
                    StorageVol *vol = storage->createStorageVolume(
                        name, QStringLiteral("qcow2"), hddSize.toInt(), flags);
                    if (vol) {
                        volumes << vol;
                    } else {
                        errors.append(QStringLiteral("Could not create storage volume"));
                        errors.append(conn->lastError());
                    }
                } else {
                    errors.append(QStringLiteral("Could not find storage"));
                }
            } else if (params.contains(QStringLiteral("template"))) {
                const QString templ = params.value(u"template"_qs);
                StorageVol *vol     = conn->getStorageVolByPath(templ, c);
                if (vol) {
                    // This is SLOW and will block clients
                    StorageVol *cloned = vol->clone(name, vol->type(), flags);
                    if (cloned) {
                        volumes << cloned;
                    } else {
                        errors.append(conn->lastError());
                    }
                }
            } else {
                const QStringList imageControl = params.values(QStringLiteral("image-control"));
                for (const QString &image : imageControl) {
                    StorageVol *vol = conn->getStorageVolByPath(image, c);
                    if (vol) {
                        volumes << vol;
                    }
                }
            }

            if (errors.isEmpty()) {
                const QString uuid =
                    QUuid::createUuid().toString().remove(0, 1).remove(QLatin1Char('}'));
                if (conn->createDomain(name,
                                       memory,
                                       vcpu,
                                       hostModel,
                                       uuid,
                                       volumes,
                                       cacheMode,
                                       networks,
                                       virtio,
                                       consoleType)) {
                    c->response()->redirect(
                        c->uriFor(QStringLiteral("/instances"), QStringList{hostId, name}));
                    return;
                } else {
                    errors.append(conn->lastError());
                }
            }
        }

        c->response()->redirect(
            c->uriFor(CActionFor(u"index"),
                      QStringList(),
                      QStringList{hostId},
                      StatusMessage::errorQuery(c, errors.join(QLatin1String("\n")))));
        return;
    }

    const QVector<StoragePool *> storages =
        conn->storagePools(VIR_CONNECT_LIST_STORAGE_POOLS_ACTIVE, c);
    c->setStash(QStringLiteral("storages"), QVariant::fromValue(storages));
    const QVector<Network *> networks = conn->networks(0, c);
    c->setStash(QStringLiteral("networks"), QVariant::fromValue(networks));
    c->setStash(QStringLiteral("get_images"), QVariant::fromValue(conn->getStorageImages(c)));
    c->setStash(QStringLiteral("cache_modes"), QVariant::fromValue(conn->getCacheModes()));

    QSqlQuery query = CPreparedSqlQueryThreadForDB(QStringLiteral("SELECT * FROM create_flavor"),
                                                   QStringLiteral("virtlyst"));
    if (query.exec()) {
        c->setStash(QStringLiteral("flavors"), Sql::queryToHashList(query));
    }
}
