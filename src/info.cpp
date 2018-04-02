#include "info.h"
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

using namespace Cutelyst;

Info::Info(QObject *parent)
    : Controller(parent)
{
}

void Info::insts_status(Context *c, const QString &hostId)
{
    c->setStash(QStringLiteral("host_id"), hostId);

    virConnectPtr conn = virConnectOpen("qemu:///system");
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    QJsonArray vms;

    virDomainPtr *domains;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains(conn, &domains, flags);
    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            virDomainPtr dom = domains[i];

            const char *name = virDomainGetName(dom);
            if (!name) {
                qWarning() << "Failed to get domain name";
                continue;
            }

            char uuid[VIR_UUID_STRING_BUFLEN];
            if (virDomainGetUUIDString(dom, uuid) < 0) {
                qWarning() << "Failed to get UUID string for domain" << name;
                continue;
            }

            virDomainInfo info;
            if (virDomainGetInfo(dom, &info) < 0) {
                qWarning() << "Failed to get info for domain" << name;
                continue;
            }

            vms.append(QJsonObject{
                           {QStringLiteral("host"), hostId},
                           {QStringLiteral("uuid"), QString::fromUtf8(uuid)},
                           {QStringLiteral("name"), QString::fromUtf8(name)},
                           {QStringLiteral("dump"), 0},
                           {QStringLiteral("status"), info.state},
                           {QStringLiteral("memory"), int(info.memory / 1024)},
                           {QStringLiteral("vcpu"), info.nrVirtCpu},
                       });
        }
        for (int i = 0; i < ret; i++) {
            virDomainFree(domains[i]);
        }
        free(domains);
        c->setStash(QStringLiteral("instances"), vms);
    }

    virConnectClose(conn);

    c->response()->setJsonArrayBody(vms);
}

void Info::instusage(Context *c, const QString &hostId, const QString &name)
{
    QJsonObject obj;

    QJsonObject cpu;

    obj.insert(QStringLiteral("cpu"), cpu);

    QJsonObject net;

    obj.insert(QStringLiteral("net"), net);

    QJsonObject hdd;

    obj.insert(QStringLiteral("hdd"), hdd);

    c->response()->setJsonObjectBody(obj);
}


