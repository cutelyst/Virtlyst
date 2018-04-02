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
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QDebug>

using namespace Cutelyst;

Infrastructure::Infrastructure(QObject *parent) : Controller(parent)
{

}

void Infrastructure::index(Context *c)
{
    virConnectPtr conn = virConnectOpen("qemu:///system");
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    char *host = virConnectGetHostname(conn);
    QVariantHash hostVms;
    hostVms.insert(QStringLiteral("id"), 1);
    hostVms.insert(QStringLiteral("name"), QString::fromUtf8(host));
    free(host);

    hostVms.insert(QStringLiteral("status"), 1);


    virNodeInfo nodeinfo;
    virNodeGetInfo(conn, &nodeinfo);

    hostVms.insert(QStringLiteral("cpus"), nodeinfo.cpus);
    hostVms.insert(QStringLiteral("memory"), Virtlyst::prettyKibiBytes(nodeinfo.memory));
    double freeMemory = virNodeGetFreeMemory(conn) / 1024;// To KibiBytes
    double difference = freeMemory / nodeinfo.memory;
    hostVms.insert(QStringLiteral("mem_usage"), QString::number(difference * 100, 'g', 3));

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

            virDomainInfo info;
            if (virDomainGetInfo(dom, &info) < 0) {
                qWarning() << "Failed to get info for domain" << name;
                continue;
            }

            double difference = double(info.memory) / nodeinfo.memory;
            vms.append(QVariantHash{
                           {QStringLiteral("name"), QString::fromUtf8(name)},
                           {QStringLiteral("status"), info.state},
                           {QStringLiteral("memory"), Virtlyst::prettyKibiBytes(info.memory)},
                           {QStringLiteral("mem_usage"), QString::number(difference * 100, 'g', 3)},
                           {QStringLiteral("vcpu"), info.nrVirtCpu},
                       });
        }
        for (int i = 0; i < ret; i++) {
            virDomainFree(domains[i]);
        }
        free(domains);
        hostVms.insert(QStringLiteral("vms"), vms);
    }

    virConnectClose(conn);

    QVariantList hosts;
    hosts.append(hostVms);
    c->setStash(QStringLiteral("hosts_vms"), hosts);
    c->setStash(QStringLiteral("template"), QStringLiteral("infrastructure.html"));
}
