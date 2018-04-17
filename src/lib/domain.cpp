/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "domain.h"
#include "connection.h"

#include "virtlyst.h"
#include "storagepool.h"
#include "storagevol.h"
#include "network.h"

#include <QTextStream>
#include <QDomElement>
#include <QFileInfo>
#include <QEventLoop>
#include <QTimer>

#include <QLoggingCategory>

#include <sys/time.h>

Q_LOGGING_CATEGORY(VIRT_DOM, "virt.domain")

Domain::Domain(virDomainPtr domain, Connection *conn, QObject *parent) : QObject(parent)
  , m_conn(conn)
  , m_domain(domain)
{

}

Domain::~Domain()
{
    virDomainFree(m_domain);
}

bool Domain::saveXml()
{
    const QString xmlData = xmlDoc().toString(0);
    m_xml.clear();
//    qCDebug(VIRT_DOM) << xmlData;
    return m_conn->domainDefineXml(xmlData);
}

QString Domain::xml()
{
    return xmlDoc().toString(2);
}

QString Domain::name() const
{
    const char *name = virDomainGetName(m_domain);
    if (!name) {
        qCWarning(VIRT_DOM) << "Failed to get domain name";
        return QString();
    }
    return QString::fromUtf8(name);
}

QString Domain::uuid()
{
    return dataFromSimpleNode(QStringLiteral("uuid"));
}

QString Domain::title()
{
    return dataFromSimpleNode(QStringLiteral("title"));
}

void Domain::setTitle(const QString &title)
{
    setDataToSimpleNode(QStringLiteral("title"), title);
}

QString Domain::description()
{
    return dataFromSimpleNode(QStringLiteral("description"));
}

void Domain::setDescription(const QString &description)
{
    setDataToSimpleNode(QStringLiteral("description"), description);
}

int Domain::status()
{
    if (!m_gotInfo) {
        if (virDomainGetInfo(m_domain, &m_info) < 0) {
            qCWarning(VIRT_DOM) << "Failed to get info for domain" << name();
            return -1;
        }
        m_gotInfo = true;
    }
    return m_info.state;
}

int Domain::currentVcpu()
{
    QDomElement docElem = xmlDoc().documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    if (node.hasAttribute(QStringLiteral("current"))) {
        return node.attribute(QStringLiteral("current")).toInt();
    }
    QDomNode data = node.firstChild();
    return data.nodeValue().toInt();
}

void Domain::setCurrentVcpu(int number)
{
    QDomElement docElem = xmlDoc().documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    node.setAttribute(QStringLiteral("current"), number);
}

int Domain::vcpu()
{
    return dataFromSimpleNode(QStringLiteral("vcpu")).toULongLong();
}

void Domain::setVcpu(int number)
{
    setDataToSimpleNode(QStringLiteral("vcpu"), QString::number(number));
}

quint64 Domain::memory()
{
    return dataFromSimpleNode(QStringLiteral("memory")).toULongLong();
}

void Domain::setMemory(quint64 kBytes)
{
    setDataToSimpleNode(QStringLiteral("memory"), QString::number(kBytes));
}

int Domain::memoryMiB()
{
    return memory() / 1024;
}

quint64 Domain::currentMemory()
{
    return dataFromSimpleNode(QStringLiteral("currentMemory")).toULongLong();
}

void Domain::setCurrentMemory(quint64 kBytes)
{
    qCritical() << Q_FUNC_INFO << QString::number(kBytes) << kBytes;
    setDataToSimpleNode(QStringLiteral("currentMemory"), QString::number(kBytes));
}

int Domain::currentMemoryMiB()
{
    return currentMemory() / 1024;
}

QString Domain::currentMemoryPretty()
{
    return Virtlyst::prettyKibiBytes(currentMemory());
}

bool Domain::hasManagedSaveImage() const
{
    return virDomainHasManagedSaveImage(m_domain, 0);
}

bool Domain::autostart() const
{
    int autostart = 0;
    if (virDomainGetAutostart(m_domain, &autostart) < 0) {
        qWarning() << "Failed to get autostart for domain" << name();
    }
    return autostart;
}

QString Domain::consoleType()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("type"));
}

void Domain::setConsoleType(const QString &type)
{
    xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .setAttribute(QStringLiteral("type"), type);
}

QString Domain::consolePassword()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("passwd"));
}

void Domain::setConsolePassword(const QString &password)
{
    xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .setAttribute(QStringLiteral("passwd"), password);
}

quint32 Domain::consolePort()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("port")).toUInt();
}

QString Domain::consoleListenAddress()
{
    QString ret = xmlDoc().documentElement()
                .firstChildElement(QStringLiteral("devices"))
                .firstChildElement(QStringLiteral("graphics"))
                .attribute(QStringLiteral("listen"));
    if (ret.isEmpty()) {
        ret = xmlDoc().documentElement()
                .firstChildElement(QStringLiteral("devices"))
                .firstChildElement(QStringLiteral("graphics"))
                .firstChildElement(QStringLiteral("listen"))
                .attribute(QStringLiteral("address"));
        if (ret.isEmpty()) {
            return QStringLiteral("127.0.0.1");
        }
    }
    return ret;
}

QString Domain::consoleKeymap()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("keymap"));
}

void Domain::setConsoleKeymap(const QString &keymap)
{
    xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .setAttribute(QStringLiteral("keymap"), keymap);
}

bool getCpuTime(virDomainPtr dom, int nparams, quint64 &cpu_time)
{
    virTypedParameter params[nparams];
    if (virDomainGetCPUStats(dom, params, nparams, -1, 1, 0) != nparams) {
        return false;
    }

    for (int i = 0; i < nparams; ++i) {
        if ((strcmp(params[i].field, VIR_DOMAIN_CPU_STATS_CPUTIME) == 0 || strcmp(params[i].field, VIR_DOMAIN_CPU_STATS_VCPUTIME) == 0)
                && params[i].type == VIR_TYPED_PARAM_ULLONG) {
            cpu_time = params[i].value.ul;
            return true;
        }
    }

    return false;
}

int Domain::cpuUsage()
{
    int nparams = virDomainGetCPUStats(m_domain, NULL, 0, -1, 1, 0); // nparams
    if (nparams == -1) {
        return -1;
    }

    struct timeval then_t, now_t;
    /* Get current time */
    if (gettimeofday(&then_t, NULL) < 0) {
        qCritical("unable to get time");
        return -1;
    }

    quint64 then_stats;
    if (!getCpuTime(m_domain, nparams, then_stats)) {
        return -1;
    }

    QEventLoop loop;
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();

    if (gettimeofday(&now_t, NULL) < 0) {
        qCritical("unable to get time");
        return -1;
    }

    quint64 now_stats;
    if (!getCpuTime(m_domain, nparams, now_stats)) {
        return -1;
    }

    quint64 then = then_t.tv_sec * 1000000 + then_t.tv_usec;
    quint64 now = now_t.tv_sec * 1000000 + now_t.tv_usec;

    double usage = (now_stats - then_stats) / (now - then) / 10 / currentVcpu();
//    qDebug("CPU: %.2lf", usage);

    return usage;
}

QVector<std::pair<qint64, qint64> > Domain::netUsage()
{
    QVector<std::pair<qint64, qint64> > ret;

    const QStringList networks = networkTargetDevs();
    for (const QString &net : networks) {
        virDomainInterfaceStatsStruct stats;
        quint64 rx = 0;
        quint64 tx = 0;
        if (virDomainInterfaceStats(m_domain, net.toUtf8().constData(), &stats, sizeof(virDomainInterfaceStatsStruct)) == 0) {
            if (stats.rx_bytes != -1) {
                rx = stats.rx_bytes;
            }
            if (stats.tx_bytes != -1) {
                tx = stats.tx_bytes;
            }
        }
        ret.append({ rx, tx });
    }

    QEventLoop loop;
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();

    int i = 0;
    for (const QString &net : networks) {
        virDomainInterfaceStatsStruct stats;
        std::pair<quint64, quint64> rx_tx = ret[i];
        quint64 rx = 0;
        quint64 tx = 0;
        if (virDomainInterfaceStats(m_domain, net.toUtf8().constData(), &stats, sizeof(virDomainInterfaceStatsStruct)) == 0) {
            if (stats.rx_bytes != -1) {
                rx = (stats.rx_bytes - rx_tx.first);
            }
            if (stats.tx_bytes != -1) {
                tx = (stats.tx_bytes - rx_tx.second);
            }
            qDebug() << net << stats.rx_bytes << rx_tx.first << rx;
            qDebug() << net << stats.tx_bytes << rx_tx.second << tx;
        }
        ret[i++] = { rx, tx };
    }

    return ret;
}

QVariantList Domain::disks()
{
    QVariantList ret;
    QDomElement disk = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("disk"));
    while (!disk.isNull()) {
        if (disk.attribute(QStringLiteral("device")) == QLatin1String("disk")) {
            const QString dev = disk.firstChildElement(QStringLiteral("target")).attribute(QStringLiteral("dev"));
            const QString srcFile = disk.firstChildElement(QStringLiteral("source")).attribute(QStringLiteral("file"));
            const QString diskFormat = disk.firstChildElement(QStringLiteral("driver")).attribute(QStringLiteral("type"));
            QString volume;
            QString storage;
            if (!srcFile.isEmpty()) {
                StorageVol *vol = m_conn->getStorageVolByPath(srcFile, this);
                if (vol) {
                    volume = vol->name();
                    StoragePool *pool = vol->pool();
                    if (pool) {
                        storage = pool->name();
                    }
                }
            }

            QHash<QString, QString> data{
                {QStringLiteral("dev"), dev},
                {QStringLiteral("image"), volume},
                {QStringLiteral("storage"), storage},
                {QStringLiteral("path"), srcFile},
                {QStringLiteral("format"), diskFormat},
            };
            ret.append(QVariant::fromValue(data));
        }
        disk = disk.nextSiblingElement(QStringLiteral("disk"));
    }

    return ret;
}

QVariantList Domain::cloneDisks()
{
    QVariantList ret;
    const QVariantList _disks = disks();
    for (const QVariant &var : _disks) {
        QHash<QString, QString> disk = var.value<QHash<QString, QString> >();
        QString image = disk.value(QStringLiteral("image"));
        if (image.isEmpty()) {
            continue;
        }

        if (image.contains(QLatin1Char('.'))) {
            QFileInfo info(image);
            if (info.path() == QLatin1Char('.')) {
                disk[QStringLiteral("image")] = info.baseName() + QLatin1String("-clone.") + info.completeSuffix();
            } else {
                disk[QStringLiteral("image")] = info.path() + QLatin1Char('/') + info.baseName() + QLatin1String("-clone.") + info.completeSuffix();
            }
        } else {
            disk[QStringLiteral("image")] = image + QLatin1String("-clone");
        }
        ret.append(QVariant::fromValue(disk));
    }
    return ret;
}

QVariantList Domain::media()
{
    QVariantList ret;
    QDomElement disk = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("disk"));
    while (!disk.isNull()) {
        if (disk.attribute(QStringLiteral("device")) == QLatin1String("cdrom")) {
            const QString dev = disk.firstChildElement(QStringLiteral("target")).attribute(QStringLiteral("dev"));
            const QString srcFile = disk.firstChildElement(QStringLiteral("source")).attribute(QStringLiteral("file"));
            QString volume;
            QString storage;
            if (!srcFile.isEmpty()) {
                StorageVol *vol = m_conn->getStorageVolByPath(srcFile, this);
                if (vol) {
                    volume = vol->name();
                    StoragePool *pool = vol->pool();
                    if (pool) {
                        storage = pool->name();
                    }
                }
            }

            QHash<QString, QString> data{
                {QStringLiteral("dev"), dev},
                {QStringLiteral("image"), volume},
                {QStringLiteral("storage"), storage},
                {QStringLiteral("path"), srcFile},
            };
            ret.append(QVariant::fromValue(data));
        }
        disk = disk.nextSiblingElement(QStringLiteral("disk"));
    }

    return ret;
}

QVariantList Domain::networks()
{
    QVariantList ret;
    QDomElement interface = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("interface"));
    while (!interface.isNull()) {
        const QString macHost = interface.firstChildElement(QStringLiteral("mac")).attribute(QStringLiteral("address"));
        const QDomElement source = interface.firstChildElement(QStringLiteral("source"));
        QString nicHost;
        if (source.hasAttribute(QStringLiteral("network"))) {
            nicHost = source.attribute(QStringLiteral("network"));
        } else if (source.hasAttribute(QStringLiteral("bridge"))) {
            nicHost = source.attribute(QStringLiteral("bridge"));
        } else if (source.hasAttribute(QStringLiteral("dev"))) {
            nicHost = source.attribute(QStringLiteral("dev"));
        }

        QString ip;
        Network *net = m_conn->getNetwork(nicHost, this);
        if (net) {
            ip = net->ipAddressForMac(macHost);
        }

        QHash<QString, QString> data{
            {QStringLiteral("mac"), macHost},
            {QStringLiteral("nic"), nicHost},
            {QStringLiteral("ip"), ip},
        };
        ret.append(QVariant::fromValue(data));
        interface = interface.nextSiblingElement(QStringLiteral("interface"));
    }

    return ret;
}

QStringList Domain::networkTargetDevs()
{
    QStringList networks;
    QDomElement interface = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("interface"));
    while (!interface.isNull()) {
        const QDomElement target = interface.firstChildElement(QStringLiteral("target"));
        if (target.hasAttribute(QStringLiteral("dev"))) {
            networks.append(target.attribute(QStringLiteral("dev")));
        }

        interface = interface.nextSiblingElement(QStringLiteral("interface"));
    }
    return networks;
}

void Domain::start()
{
    virDomainCreate(m_domain);
}

void Domain::shutdown()
{
    virDomainShutdown(m_domain);
}

void Domain::suspend()
{
    virDomainSuspend(m_domain);
}

void Domain::resume()
{
    virDomainResume(m_domain);
}

void Domain::destroy()
{
    virDomainDestroy(m_domain);
}

void Domain::undefine()
{
    virDomainUndefine(m_domain);
}

void Domain::managedSave()
{
    virDomainManagedSave(m_domain, 0);
}

void Domain::managedSaveRemove()
{
    virDomainManagedSaveRemove(m_domain, 0);
}

void Domain::setAutostart(bool enable)
{
    virDomainSetAutostart(m_domain, enable ? 1 : 0);
}

bool Domain::attachDevice(const QString &xml)
{
    m_xml.clear();
    return virDomainAttachDevice(m_domain, xml.toUtf8().constData()) == 0;
}

bool Domain::updateDevice(const QString &xml, int flags)
{
    m_xml.clear();
    return virDomainUpdateDeviceFlags(m_domain, xml.toUtf8().constData(), flags) == 0;
}

void Domain::mountIso(const QString &dev, const QString &image)
{
    QDomDocument doc = xmlDoc();
    QDomElement disk = doc
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("disk"));
    while (!disk.isNull()) {
        if (disk.attribute(QStringLiteral("device")) == QLatin1String("cdrom")) {
            if (disk.firstChildElement(QStringLiteral("target")).attribute(QStringLiteral("dev")) == dev) {
                QDomElement source = disk.firstChildElement(QStringLiteral("source"));
                if (source.isNull()) {
                    source = doc.createElement(QStringLiteral("source"));
                    disk.appendChild(source);
                }
                source.setAttribute(QStringLiteral("file"), image);
                QByteArray xml;
                QTextStream s(&xml);
                disk.save(s, 2);
                bool ret = updateDevice(xml, VIR_DOMAIN_AFFECT_CONFIG);
                qDebug() << "DISK update" << ret;
                break;
            }
        }
        disk = disk.nextSiblingElement(QStringLiteral("disk"));
    }
    qDebug() << 2 << image;

    qDebug() << 2 << doc.toString(2).toUtf8().constData();
//    saveXml();
}

void Domain::umountIso(const QString &dev, const QString &image)
{
    qDebug() << 1 << xmlDoc().toString(2);
    QDomElement disk = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("disk"));
    while (!disk.isNull()) {
        if (disk.attribute(QStringLiteral("device")) == QLatin1String("cdrom")) {
            QDomElement source = disk.firstChildElement(QStringLiteral("source"));
            if (disk.firstChildElement(QStringLiteral("target")).attribute(QStringLiteral("dev")) == dev &&
                    source.attribute(QStringLiteral("file")) == image) {
                disk.removeChild(source);
                QByteArray xml;
                QTextStream s(&xml);
                disk.save(s, 2);
                bool ret = updateDevice(xml, VIR_DOMAIN_AFFECT_CONFIG);
                qDebug() << "DISK remove update" << ret;
            }
        }
        disk = disk.nextSiblingElement(QStringLiteral("disk"));
    }
//    saveXml();
    qDebug() << 2 << xmlDoc().toString(2).toUtf8().constData();
}

QDomDocument Domain::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virDomainGetXMLDesc(m_domain, VIR_DOMAIN_XML_SECURE);
        const QString xmlString = QString::fromUtf8(xml);
//        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from interface" << error;
        }
        free(xml);
    }
    return m_xml;
}

QString Domain::dataFromSimpleNode(const QString &element)
{
    return xmlDoc().documentElement()
            .firstChildElement(element)
            .firstChild()
            .nodeValue();
}

void Domain::setDataToSimpleNode(const QString &element, const QString &data)
{
    xmlDoc().documentElement()
            .firstChildElement(element)
            .firstChild()
            .setNodeValue(data);
}
