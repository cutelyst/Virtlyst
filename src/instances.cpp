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
#include "instances.h"
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QDebug>

using namespace Cutelyst;

Instances::Instances(Virtlyst *parent)
    : Controller(parent)
    , m_virtlyst(parent)
{
}

void Instances::index(Context *c, const QString &hostId)
{
    virConnectPtr conn = m_virtlyst->connection(hostId);
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        const QString name = params.value(QStringLiteral("name"));
        virDomainPtr domain = virDomainLookupByName(conn, name.toUtf8().constData());
        if (domain) {
            bool redir = false;
            if (params.contains(QStringLiteral("start"))) {
                virDomainCreate(domain);
                redir = true;
            } else if (params.contains(QStringLiteral("shutdown"))) {
                virDomainShutdown(domain);
                redir = true;
            } else if (params.contains(QStringLiteral("destroy"))) {
                virDomainDestroy(domain);
                redir = true;
            } else if (params.contains(QStringLiteral("managedsave"))) {
                virDomainManagedSave(domain, 0);
                redir = true;
            } else if (params.contains(QStringLiteral("deletesaveimage"))) {
                virDomainManagedSaveRemove(domain, 0);
                redir = true;
            } else if (params.contains(QStringLiteral("suspend"))) {
                virDomainSuspend(domain);
                redir = true;
            } else if (params.contains(QStringLiteral("resume"))) {
                virDomainResume(domain);
                redir = true;
            }
            virDomainFree(domain);

            if (redir) {
                c->response()->redirect(c->uriFor(CActionFor("index"), QStringList{ hostId }));
                return;
            }
        }
    }

    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    virDomainPtr *domains;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains(conn, &domains, flags);
    if (ret > 0) {
        QVariantList vms;
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

            vms.append(QVariantHash{
                           {QStringLiteral("name"), QString::fromUtf8(name)},
                           {QStringLiteral("uuid"), QString::fromUtf8(uuid)},
                           {QStringLiteral("status"), info.state},
                           {QStringLiteral("memory"), Virtlyst::prettyKibiBytes(info.memory)},
                           {QStringLiteral("vcpu"), info.nrVirtCpu},
                       });
        }
        for (int i = 0; i < ret; i++) {
            virDomainFree(domains[i]);
        }
        free(domains);
        c->setStash(QStringLiteral("instances"), vms);
    }

    c->setStash(QStringLiteral("template"), QStringLiteral("instances.html"));
}

