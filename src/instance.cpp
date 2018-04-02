#include "instance.h"
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QDebug>

using namespace Cutelyst;

Instance::Instance(QObject *parent)
    : Controller(parent)
{
}

void Instance::index(Context *c, const QString &hostId, const QString &name)
{
    QStringList errors;
    c->setStash(QStringLiteral("template"), QStringLiteral("instance.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    virConnectPtr conn = virConnectOpen("qemu:///system");
    if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }

    virDomainPtr domain = virDomainLookupByName(conn, name.toUtf8().constData());
    if (!domain) {
        virConnectClose(conn);
        errors.append(QStringLiteral("Domain not found: no domain with matching name '%1'").arg(name));
        c->setStash(QStringLiteral("errors"), errors);
        return;
    }

    virDomainInfo info;
    if (virDomainGetInfo(domain, &info) < 0) {
        qWarning() << "Failed to get info for domain" << name;
    }

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        bool redir = false;
        if (params.contains(QStringLiteral("start"))) {
            virDomainCreate(domain);
            redir = true;
        } else if (params.contains(QStringLiteral("power"))) {
            const QString power = params.value(QStringLiteral("power"));
            if (power == QLatin1String("shutdown")) {
                virDomainShutdown(domain);
            } else if (power == QLatin1String("destroy")) {
                virDomainDestroy(domain);
            } else if (power == QLatin1String("managedsave")) {
                virDomainManagedSave(domain, 0);
            }
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
        } else if (params.contains(QStringLiteral("unset_autostart"))) {
            virDomainSetAutostart(domain, 0);
            redir = true;
        } else if (params.contains(QStringLiteral("set_autostart"))) {
            virDomainSetAutostart(domain, 1);
            redir = true;
        } else if (params.contains(QStringLiteral("delete"))) {
            if (info.state == VIR_DOMAIN_RUNNING) {
                virDomainDestroy(domain);
            }

            virDomainUndefine(domain);
            if (params.contains(QStringLiteral("delete_disk"))) {
                //TODO
            }
            redir = true;
        } else if (params.contains(QStringLiteral("change_xml"))) {
            const QString xml = params.value(QStringLiteral("inst_xml"));
            virDomainPtr dom = virDomainDefineXML(conn, xml.toUtf8().constData());
            virDomainFree(dom);
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

            }
            redir = true;
        } else if (params.contains(QStringLiteral("change_settings"))) {
//            const QString name = params.value(QStringLiteral("name"));
            redir = true;
        }

        if (redir) {
            virDomainFree(domain);
            virConnectClose(conn);
            c->response()->redirect(c->uriFor(CActionFor("index"), QStringList{ hostId, name }));
            return;
        }
    }

    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    char uuid[VIR_UUID_STRING_BUFLEN];
    if (virDomainGetUUIDString(domain, uuid) < 0) {
        qWarning() << "Failed to get UUID string for domain" << name;
    }

    int autostart = 0;
    if (virDomainGetAutostart(domain, &autostart) < 0) {
        qWarning() << "Failed to get autostart for domain" << name;
    }

    int has_managed_save_image = virDomainHasManagedSaveImage(domain, 0);

    char *description = virDomainGetMetadata(domain, VIR_DOMAIN_METADATA_DESCRIPTION, NULL, VIR_DOMAIN_AFFECT_CURRENT);
    char *xml = virDomainGetXMLDesc(domain, VIR_DOMAIN_XML_SECURE);

    int vcpus = virConnectGetMaxVcpus(conn, NULL);
    QVector<int> vcpu_range;
    for (int i = 1; i <= vcpus; ++i) {
        vcpu_range << i;
    }
    c->setStash(QStringLiteral("vcpu_range"), QVariant::fromValue(vcpu_range));

    QVector<int> memory_range({256, 512, 768, 1024, 2048, 4096, 6144, 8192, 16384});
    int cur_memory = info.memory / 1024;
    if (!memory_range.contains(cur_memory)) {
        memory_range.append(cur_memory);
        std::sort(memory_range.begin(), memory_range.end(),[] (int a, int b) -> int { return a < b; });
    }
    int memory = info.maxMem / 1024;
    if (!memory_range.contains(memory)) {
        memory_range.append(memory);
        std::sort(memory_range.begin(), memory_range.end(),[] (int a, int b) -> int { return a < b; });
    }
    c->setStash(QStringLiteral("memory_range"), QVariant::fromValue(memory_range));


    virNodeInfo nodeinfo;
    virNodeGetInfo(conn, &nodeinfo);
    c->setStash(QStringLiteral("vcpu_host"), nodeinfo.cpus);
    c->setStash(QStringLiteral("memory_host"), Virtlyst::prettyKibiBytes(nodeinfo.memory));

    c->setStash(QStringLiteral("host"), QVariantHash{
                    {QStringLiteral("name"), name},
                    {QStringLiteral("uuid"), QString::fromUtf8(uuid)},
                    {QStringLiteral("status"), info.state},
                    {QStringLiteral("memory_pretty"), Virtlyst::prettyKibiBytes(info.memory)},
                    {QStringLiteral("cur_memory"), cur_memory},
                    {QStringLiteral("memory"), memory},
                    {QStringLiteral("vcpu"), info.nrVirtCpu},
                    {QStringLiteral("autostart"), autostart},
                    {QStringLiteral("has_managed_save_image"), has_managed_save_image},
                    {QStringLiteral("description"), QString::fromUtf8(description)},
                    {QStringLiteral("inst_xml"), QString::fromUtf8(xml)},
                    {QStringLiteral("vcpu_range"), QVariant::fromValue(vcpu_range)},
                });
    free(description);
    free(xml);

    c->setStash(QStringLiteral("errors"), errors);

    virConnectClose(conn);
}
