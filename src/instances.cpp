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

#include "lib/connection.h"
#include "lib/domain.h"

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
    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        const QString name = params.value(QStringLiteral("name"));
        Domain *domain = conn->getDomainByName(name, c);
        if (domain) {
            if (params.contains(QStringLiteral("start"))) {
                domain->start();
            } else if (params.contains(QStringLiteral("shutdown"))) {
                domain->shutdown();
            } else if (params.contains(QStringLiteral("destroy"))) {
                domain->destroy();
            } else if (params.contains(QStringLiteral("managedsave"))) {
                domain->managedSave();
            } else if (params.contains(QStringLiteral("deletesaveimage"))) {
                domain->managedSaveRemove();
            } else if (params.contains(QStringLiteral("suspend"))) {
                domain->suspend();
            } else if (params.contains(QStringLiteral("resume"))) {
                domain->resume();
            }

            c->response()->redirect(c->uriFor(CActionFor("index"), QStringList{ hostId }));
        }
    }

    const QVector<Domain *> domains = conn->domains(
                VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_INACTIVE, c);
    c->setStash(QStringLiteral("instances"), QVariant::fromValue(domains));
    c->setStash(QStringLiteral("template"), QStringLiteral("instances.html"));
}

void Instances::instance(Context *c, const QString &hostId, const QString &name)
{
    QStringList errors;
    c->setStash(QStringLiteral("template"), QStringLiteral("instance.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    Domain *dom = conn->getDomainByName(name, c);
    if (!dom) {
        errors.append(QStringLiteral("Domain not found: no domain with matching name '%1'").arg(name));
        c->setStash(QStringLiteral("errors"), errors);
        return;
    }

    qDebug() << "CONSOLE type" << dom->consoleType();

    c->setStash(QStringLiteral("console_types"), QStringList{QStringLiteral("vnc"), QStringLiteral("spice")});

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        bool redir = false;
        if (params.contains(QStringLiteral("start"))) {
            dom->start();
            redir = true;
        } else if (params.contains(QStringLiteral("power"))) {
            const QString power = params.value(QStringLiteral("power"));
            if (power == QLatin1String("shutdown")) {
                dom->shutdown();
            } else if (power == QLatin1String("destroy")) {
                dom->destroy();
            } else if (power == QLatin1String("managedsave")) {
                dom->managedSave();
            }
            redir = true;
        } else if (params.contains(QStringLiteral("deletesaveimage"))) {
            dom->managedSaveRemove();
            redir = true;
        } else if (params.contains(QStringLiteral("suspend"))) {
            dom->suspend();
            redir = true;
        } else if (params.contains(QStringLiteral("resume"))) {
            dom->resume();
            redir = true;
        } else if (params.contains(QStringLiteral("unset_autostart"))) {
            dom->setAutostart(false);
            redir = true;
        } else if (params.contains(QStringLiteral("set_autostart"))) {
            dom->setAutostart(true);
            redir = true;
        } else if (params.contains(QStringLiteral("delete"))) {
            if (dom->status() == VIR_DOMAIN_RUNNING) {
                dom->destroy();
            }

            dom->undefine();
            if (params.contains(QStringLiteral("delete_disk"))) {
                //TODO
            }

            c->response()->redirect(c->uriFor(CActionFor("index"), QStringList{ hostId }));
            return;
        } else if (params.contains(QStringLiteral("change_xml"))) {
            const QString xml = params.value(QStringLiteral("inst_xml"));
            conn->domainDefineXml(xml);
            redir = true;
        } else if (params.contains(QStringLiteral("snapshot"))) {
//            const QString name = params.value(QStringLiteral("name"));
            redir = true;
        } else if (params.contains(QStringLiteral("set_console_passwd"))) {
            QString password;
            if (params.contains(QStringLiteral("auto_pass"))) {

            } else {
                password = params.value(QStringLiteral("console_passwd"));
                bool clear = params.contains(QStringLiteral("clear_pass"));
                if (clear) {
                    password.clear();
                }
                if (password.isEmpty() && !clear) {
                    errors.append(QStringLiteral("Enter the console password or select Generate"));
                }
            }

            if (errors.isEmpty()) {
                dom->setConsolePassword(password);
                dom->saveXml();
            }
            redir = true;
        } else if (params.contains(QStringLiteral("set_console_type"))) {
            const QString type = params.value(QStringLiteral("console_type"));
            dom->setConsoleType(type);
            dom->saveXml();
            redir = true;
        } else if (params.contains(QStringLiteral("mount_iso"))) {
            const QString dev = params.value(QStringLiteral("mount_iso"));
            const QString image = params.value(QStringLiteral("media"));
            dom->mountIso(dev, image);
            redir = true;
        } else if (params.contains(QStringLiteral("umount_iso"))) {
            const QString dev = params.value(QStringLiteral("umount_iso"));
            const QString image = params.value(QStringLiteral("path"));
            dom->umountIso(dev, image);
            redir = true;
        } else if (params.contains(QStringLiteral("change_settings"))) {
            const QString description = params.value(QStringLiteral("description"));

            ulong memory;
            const QString memory_custom = params.value(QStringLiteral("memory_custom"));
            if (memory_custom.isEmpty()) {
                memory = params.value(QStringLiteral("memory")).toULong();
            } else {
                memory = memory_custom.toULong();
            }

            ulong cur_memory;
            const QString cur_memory_custom = params.value(QStringLiteral("cur_memory_custom"));
            if (memory_custom.isEmpty()) {
                cur_memory = params.value(QStringLiteral("cur_memory")).toULong();
            } else {
                cur_memory = cur_memory_custom.toULong();
            }

            ulong vcpu = params.value(QStringLiteral("vcpu")).toUInt();
            ulong cur_vcpu = params.value(QStringLiteral("cur_vcpu")).toUInt();

            dom->setDescription(description);
            dom->setMemory(memory * 1024);
            dom->setCurrentMemory(cur_memory * 1024);
            dom->setVcpu(vcpu);
            dom->setCurrentVcpu(cur_vcpu);
            dom->saveXml();
            redir = true;
        }

        if (redir) {
            c->response()->redirect(c->uriFor(CActionFor("index"), QStringList{ hostId, name }));
            return;
        }
    }

    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    int vcpus = conn->maxVcpus();
    QVector<int> vcpu_range;
    for (int i = 1; i <= vcpus; ++i) {
        vcpu_range << i;
    }
    c->setStash(QStringLiteral("vcpu_range"), QVariant::fromValue(vcpu_range));

    QVector<int> memory_range;
    int last = 256;
    // conn.memory is in kilobytes, I guess we need to convert to kibi bytes
    while (last <= conn->memory() / 1024) {
        memory_range.append(last);
        last *= 2;
    }
    int cur_memory = dom->currentMemory() / 1024;
    if (!memory_range.contains(cur_memory)) {
        memory_range.append(cur_memory);
        std::sort(memory_range.begin(), memory_range.end(),[] (int a, int b) -> int { return a < b; });
    }
    int memory = dom->memory() / 1024;
    if (!memory_range.contains(memory)) {
        memory_range.append(memory);
        std::sort(memory_range.begin(), memory_range.end(),[] (int a, int b) -> int { return a < b; });
    }
    c->setStash(QStringLiteral("memory_range"), QVariant::fromValue(memory_range));

    c->setStash(QStringLiteral("vcpu_host"), conn->cpus());
    c->setStash(QStringLiteral("memory_host"), conn->freeMemoryBytes());

    c->setStash(QStringLiteral("domain"), QVariant::fromValue(dom));

    c->setStash(QStringLiteral("errors"), errors);
}

