#include "create.h"

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/storagepool.h"
#include "lib/storagevol.h"
#include "lib/network.h"
#include "lib/domain.h"

#include <Cutelyst/Plugins/StatusMessage>

#include <QUuid>
#include <QDomDocument>
#include <QLoggingCategory>

using namespace Cutelyst;

Create::Create(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Create::index(Context *c, const QString &hostId)
{
    StatusMessage::load(c);
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);
    c->setStash(QStringLiteral("template"), QStringLiteral("create.html"));

    const QVector<Domain *> domains = conn->domains(
                VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_INACTIVE, c);
    c->setStash(QStringLiteral("instances"), QVariant::fromValue(domains));

    if (c->request()->isPost()) {
        QStringList errors;
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("create_flavor"))) {

        } else if (params.contains(QStringLiteral("delete_flavor"))) {

        } else if (params.contains(QStringLiteral("create_xml"))) {
            const QString xml = params[QStringLiteral("from_xml")];
            QDomDocument xmlDoc;
            if (xmlDoc.setContent(xml)) {
                const QString name = xmlDoc
                        .documentElement()
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
                        c->response()->redirect(c->uriFor(QStringLiteral("/instances"), QStringList{ hostId, name }));
                        return;
                    }
                    errors.append(QStringLiteral("Failed to create virtual machine:"));
                    errors.append(conn->lastError());
                } else {
                    errors.append(QStringLiteral("A virtual machine with this name already exists"));
                }
            } else {
                errors.append(QStringLiteral("Invalid XML"));
            }
        } else if (params.contains(QStringLiteral("create"))) {
            const QString name = params[QStringLiteral("name")];
            const QString memory = params[QStringLiteral("memory")];
            const QString vcpu = params[QStringLiteral("vcpu")];
            const bool hostModel = params.contains(QStringLiteral("host_model"));
            const QString cacheMode = params[QStringLiteral("cache_mode")];
            const bool virtio = params.contains(QStringLiteral("virtio"));
            const QString consoleType = QStringLiteral("spice");
            const QStringList networks = params.values(QStringLiteral("network-control"));
            const QStringList imageControl = params.values(QStringLiteral("image-control"));

            QVector<StorageVol *> images;
            for (const QString &image : imageControl) {
                StorageVol *vol = conn->getStorageVolByPath(image, c);
                if (vol) {
                    images << vol;
                }
            }

            const QString uuid = QUuid::createUuid().toString().remove(0, 1).remove(QLatin1Char('}'));
            if (conn->createDomain(name, memory, vcpu, hostModel, uuid, images, cacheMode, networks, virtio, consoleType)) {
                c->response()->redirect(c->uriFor(QStringLiteral("/instances"), QStringList{ hostId, name }));
                return;
            } else {
                errors.append(conn->lastError());
            }
        }

        c->response()->redirect(c->uriFor(CActionFor("index"),
                                          QStringList(),
                                          QStringList{ hostId },
                                          StatusMessage::errorQuery(c, errors.join(QLatin1String("\n")))));
        return;
    }

    const QVector<StoragePool *> storages = conn->storagePools(0, c);
    c->setStash(QStringLiteral("storages"), QVariant::fromValue(storages));
    const QVector<Network *> networks = conn->networks(0, c);
    c->setStash(QStringLiteral("networks"), QVariant::fromValue(networks));
//    const QVector<Domain *> domains = conn->domains(VIR_CONNECT_LIST_DOMAINS_ACTIVE, c);
//    c->setStash(QStringLiteral("instances"), QVariant::fromValue(domains));
    c->setStash(QStringLiteral("get_images"), QVariant::fromValue(conn->getStorageImages(c)));
    c->setStash(QStringLiteral("cache_modes"), QVariant::fromValue(conn->getCacheModes()));
}
