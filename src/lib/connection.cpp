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
#include "connection.h"

#include "virtlyst.h"
#include "domain.h"
#include "interface.h"
#include "network.h"
#include "secret.h"
#include "nodedevice.h"
#include "storagepool.h"

#include <QXmlStreamWriter>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(VIRT_CONN, "virt.connection")

static int authCreds[] = {
    VIR_CRED_AUTHNAME,
    VIR_CRED_PASSPHRASE,
};

static int authCb(virConnectCredentialPtr cred, unsigned int ncred, void *cbdata)
{
    int i;
    char buf[1024];

    for (i = 0; i < ncred; i++) {
        if (cred[i].type == VIR_CRED_AUTHNAME) {
            printf("%s: ", cred[i].prompt);
            fflush(stdout);
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';
            cred[i].result = strdup(buf);
            if (cred[i].result == NULL)
                return -1;
            cred[i].resultlen = strlen(cred[i].result);
        }
        else if (cred[i].type == VIR_CRED_PASSPHRASE) {
            printf("%s: ", cred[i].prompt);
            fflush(stdout);
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';
            cred[i].result = strdup(buf);
            if (cred[i].result == NULL)
                return -1;
            cred[i].resultlen = strlen(cred[i].result);
        }
    }

    return 0;
}

Connection::Connection(const QString &name, QObject *parent) : QObject(parent)
  , m_uri(name)
{
    virConnectAuth auth;
    auth.credtype = authCreds;
    auth.ncredtype = sizeof(authCreds)/sizeof(int);
    auth.cb = authCb;
    auth.cbdata = this;

    m_conn = virConnectOpenAuth(name.toUtf8().constData(), &auth, 0);
    if (m_conn == NULL) {
        qCWarning(VIRT_CONN) << "Failed to open connection to" << name;
        return;
    }
}

Connection::~Connection()
{
    virConnectClose(m_conn);
}

QString Connection::uri() const
{
    return QString::fromUtf8(virConnectGetURI(m_conn));
}

QString Connection::hostname() const
{
    QString ret;
    if (m_conn) {
        char *host = virConnectGetHostname(m_conn);
        ret = QString::fromUtf8(host);
        free(host);
    }
    return ret;
}

QString Connection::hypervisor() const
{
    return QString::fromUtf8(virConnectGetType(m_conn));
}

quint64 Connection::freeMemoryBytes() const
{
    if (m_conn) {
        return virNodeGetFreeMemory(m_conn);
    }
    return 0;
}

quint64 Connection::memory()
{
    if (!m_nodeInfoLoaded) {
        loadNodeInfo();
    }
    return m_nodeInfo.memory;
}

QString Connection::memoryPretty()
{
    if (!m_nodeInfoLoaded) {
        loadNodeInfo();
    }
    return Virtlyst::prettyKibiBytes(m_nodeInfo.memory);
}

uint Connection::cpus()
{
    if (!m_nodeInfoLoaded) {
        loadNodeInfo();
    }
    return m_nodeInfo.cpus;
}

int Connection::maxVcpus() const
{
    return virConnectGetMaxVcpus(m_conn, NULL);
}

QString Connection::arch()
{
    if (!m_domainCapabilitiesLoaded) {
        loadDomainCapabilities();
    }
    return dataFromSimpleNode(QStringLiteral("arch"));
}

QString Connection::cpuVendor()
{
    if (!m_domainCapabilitiesLoaded) {
        loadDomainCapabilities();
    }
    QDomElement docElem = m_xmlCapsDoc.documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("cpu"));

    node = node.firstChildElement(QStringLiteral("mode"));
    while (!node.isNull() && node.attribute(QStringLiteral("name")) != QLatin1String("host-model")) {
        node = node.nextSiblingElement(QStringLiteral("mode"));
    }

    if (node.isNull()) {
        return QString();
    }
    node = node.firstChildElement(QStringLiteral("vendor"));

    return node.text();
}

bool Connection::domainDefineXml(const QString &xml)
{
    virDomainPtr dom = virDomainDefineXML(m_conn, xml.toUtf8().constData());
    virDomainFree(dom);
    return dom != NULL;
}

QVector<Domain *> Connection::domains(int flags, QObject *parent)
{
    QVector<Domain *> ret;
    virDomainPtr *domains;
    int count = virConnectListAllDomains(m_conn, &domains, flags);
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            auto domain = new Domain(domains[i], this, parent);
            domain->loadXml();
            ret.append(domain);
        }
        free(domains);
    }
    return ret;
}

Domain *Connection::getDomainByUuid(const QString &uuid, QObject *parent)
{
    virDomainPtr domain = virDomainLookupByUUIDString(m_conn, uuid.toUtf8().constData());
    if (!domain) {
        return nullptr;
    }
    auto dom = new Domain(domain, this, parent);
    dom->loadXml();
    return dom;
}

Domain *Connection::getDomainByName(const QString &name, QObject *parent)
{
    virDomainPtr domain = virDomainLookupByName(m_conn, name.toUtf8().constData());
    if (!domain) {
        return nullptr;
    }
    auto dom = new Domain(domain, this, parent);
    dom->loadXml();
    return dom;
}

QVector<Interface *> Connection::interfaces(uint flags, QObject *parent)
{
    QVector<Interface *> ret;
    virInterfacePtr *ifaces;
    int count = virConnectListAllInterfaces(m_conn, &ifaces, flags);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto iface = new Interface(ifaces[i], this, parent);
            ret.append(iface);
        }
        free(ifaces);
    }
    return ret;
}

Interface *Connection::getInterface(const QString &name, QObject *parent)
{
    virInterfacePtr iface = virInterfaceLookupByName(m_conn, name.toUtf8().constData());
    if (!iface) {
        return nullptr;
    }
    return new Interface(iface, this, parent);
}

void Connection::createInterface(const QString &name, const QString &netdev, const QString &type,
                                 const QString &startMode, int delay, bool stp,
                                 const QString &ipv4Addr, const QString &ipv4Gw, const QString &ipv4Type,
                                 const QString &ipv6Addr, const QString &ipv6Gw, const QString &ipv6Type)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("interface"));
    if (!name.isEmpty()) {
        stream.writeAttribute(QStringLiteral("name"), name);
    }
    if (!type.isEmpty()) {
        stream.writeAttribute(QStringLiteral("type"), type);
    }

    stream.writeStartElement(QStringLiteral("start"));
    stream.writeAttribute(QStringLiteral("mode"), startMode);
    stream.writeEndElement(); // start

    if (ipv4Type == QLatin1String("dhcp")) {
        stream.writeStartElement(QStringLiteral("protocol"));
        stream.writeAttribute(QStringLiteral("family"), QStringLiteral("ipv4"));
        stream.writeStartElement(QStringLiteral("dhcp"));
        stream.writeEndElement(); // dhcp
        stream.writeEndElement(); // protocol
    } else if (ipv4Type == QLatin1String("static")) {
        const QString address = ipv4Addr.section(QLatin1Char('/'), 0, 0);
        const QString prefix = ipv4Addr.section(QLatin1Char('/'), -1);
        stream.writeStartElement(QStringLiteral("protocol"));
        stream.writeAttribute(QStringLiteral("family"), QStringLiteral("ipv4"));
        stream.writeStartElement(QStringLiteral("ip"));
        stream.writeAttribute(QStringLiteral("address"), address);
        stream.writeAttribute(QStringLiteral("prefix"), prefix);
        stream.writeEndElement(); // ip
        stream.writeStartElement(QStringLiteral("route"));
        stream.writeAttribute(QStringLiteral("gateway"), ipv4Gw);
        stream.writeEndElement(); // route
        stream.writeEndElement(); // protocol
    }

    if (ipv6Type == QLatin1String("dhcp")) {
        stream.writeStartElement(QStringLiteral("protocol"));
        stream.writeAttribute(QStringLiteral("family"), QStringLiteral("ipv6"));
        stream.writeStartElement(QStringLiteral("dhcp"));
        stream.writeEndElement(); // dhcp
        stream.writeEndElement(); // protocol
    } else if (ipv6Type == QLatin1String("static")) {
        const QString address = ipv6Addr.section(QLatin1Char('/'), 0, 0);
        const QString prefix = ipv6Addr.section(QLatin1Char('/'), -1);
        stream.writeStartElement(QStringLiteral("protocol"));
        stream.writeAttribute(QStringLiteral("family"), QStringLiteral("ipv6"));
        stream.writeStartElement(QStringLiteral("ip"));
        stream.writeAttribute(QStringLiteral("address"), address);
        stream.writeAttribute(QStringLiteral("prefix"), prefix);
        stream.writeEndElement(); // ip
        stream.writeStartElement(QStringLiteral("route"));
        stream.writeAttribute(QStringLiteral("gateway"), ipv6Gw);
        stream.writeEndElement(); // route
        stream.writeEndElement(); // protocol
    }

    if (type == QLatin1String("bridge")) {
        stream.writeStartElement(QStringLiteral("bridge"));
        if (stp) {
            stream.writeAttribute(QStringLiteral("stp"), QStringLiteral("on"));
        }
        stream.writeAttribute(QStringLiteral("delay"), QString::number(delay));
        stream.writeStartElement(QStringLiteral("interface"));
        stream.writeAttribute(QStringLiteral("name"), netdev);
        stream.writeAttribute(QStringLiteral("type"), QStringLiteral("ethernet"));
        stream.writeEndElement(); // interface
        stream.writeEndElement(); // bridge
    }

    stream.writeEndElement(); // interface
    qDebug() << "XML output" << output;

    virInterfacePtr iface = virInterfaceDefineXML(m_conn, output.constData(), 0);
    virInterfaceFree(iface);
}

QVector<Network *> Connection::networks(uint flags, QObject *parent)
{
    QVector<Network *> ret;
    virNetworkPtr *nets;
    int count = virConnectListAllNetworks(m_conn, &nets, flags);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto net = new Network(nets[i], this, parent);
            ret.append(net);
        }
        free(nets);
    }
    return ret;
}

Network *Connection::getNetwork(const QString &name, QObject *parent)
{
    virNetworkPtr network = virNetworkLookupByName(m_conn, name.toUtf8().constData());
    if (!network) {
        return nullptr;
    }
    return new Network(network, this, parent);
}

bool Connection::createNetwork(const QString &name, const QString &forward, const QString &gateway, const QString &mask,
                               const QString &bridge, bool dhcp, bool openvswitch, bool fixed)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("network"));

    stream.writeTextElement(QStringLiteral("name"), name);

    bool isForward = QStringList{ QStringLiteral("nat"), QStringLiteral("route"), QStringLiteral("bridge")}
            .contains(forward);
    if (isForward) {
        stream.writeStartElement(QStringLiteral("forward"));
        stream.writeAttribute(QStringLiteral("mode"), forward);
        stream.writeEndElement(); // forward
    }

    bool isForwardStp = QStringList{ QStringLiteral("nat"), QStringLiteral("route"), QStringLiteral("none")}
            .contains(forward);
    stream.writeStartElement(QStringLiteral("bridge"));
    if (isForwardStp) {
        stream.writeAttribute(QStringLiteral("stp"), QStringLiteral("on"));
        stream.writeAttribute(QStringLiteral("delay"), QStringLiteral("0"));
    } else if (forward == QLatin1String("bridge")) {
        stream.writeAttribute(QStringLiteral("name"), bridge);
    }
    stream.writeEndElement(); // bridge

    if (openvswitch) {
        stream.writeStartElement(QStringLiteral("virtualport"));
        stream.writeAttribute(QStringLiteral("type"), QStringLiteral("openvswitch"));
        stream.writeEndElement(); // virtualport
    }

    if (forward != QLatin1String("bridge")) {
        stream.writeStartElement(QStringLiteral("ip"));
        stream.writeAttribute(QStringLiteral("address"), gateway);
        stream.writeAttribute(QStringLiteral("netmask"), mask);
        if (dhcp) {
            stream.writeStartElement(QStringLiteral("dhcp"));
            stream.writeEndElement(); // dhcp
        }

        stream.writeEndElement(); // ip
    }

    stream.writeEndElement(); // network
    qDebug() << "XML output" << output;
    virNetworkPtr net = virNetworkDefineXML(m_conn, output.constData());
    virNetworkFree(net);
}

QVector<Secret *> Connection::secrets(uint flags, QObject *parent)
{
    QVector<Secret *> ret;
    virSecretPtr *secrets;
    int count = virConnectListAllSecrets(m_conn, &secrets, flags);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto secret = new Secret(secrets[i], this, parent);
            ret.append(secret);
        }
        free(secrets);
    }
    return ret;
}

void Connection::createSecret(const QString &ephemeral, const QString &usageType, const QString &priv,  const QString &data)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("secret"));
    if (!ephemeral.isEmpty()) {
        stream.writeAttribute(QStringLiteral("ephemeral"), ephemeral);
    }
    if (!priv.isEmpty()) {
        stream.writeAttribute(QStringLiteral("private"), priv);
    }

    stream.writeStartElement(QStringLiteral("usage"));
    stream.writeAttribute(QStringLiteral("type"), usageType);
    if (usageType == QLatin1String("ceph")) {
        stream.writeTextElement(QStringLiteral("name"), data);
    } else if (usageType == QLatin1String("volume")) {
        stream.writeTextElement(usageType, data);
    } else if (usageType == QLatin1String("iscsi")) {
        stream.writeTextElement(QStringLiteral("target"), data);
    }
    stream.writeEndElement(); // usage

    stream.writeEndElement(); // secret
    qDebug() << "XML output" << output;
//    xml.appendChild();
    virSecretPtr secret = virSecretDefineXML(m_conn, output.constData(), 0);
    virSecretFree(secret);
}

Secret *Connection::getSecretByUuid(const QString &uuid, QObject *parent)
{
    virSecretPtr secret = virSecretLookupByUUIDString(m_conn, uuid.toLatin1().constData());
    if (!secret) {
        return nullptr;
    }
    return new Secret(secret, this, parent);
}

bool Connection::deleteSecretByUuid(const QString &uuid)
{
    virSecretPtr secret = virSecretLookupByUUIDString(m_conn, uuid.toLatin1().constData());
    if (!secret) {
        return true;
    }
    return virSecretUndefine(secret) == 0;
}

QVector<StoragePool *> Connection::storagePools(int flags, QObject *parent)
{
    QVector<StoragePool *> ret;
    virStoragePoolPtr *storagePools;
    int count = virConnectListAllStoragePools(m_conn, &storagePools, flags);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto storagePool = new StoragePool(storagePools[i], this, parent);
            ret.append(storagePool);
        }
        free(storagePools);
    }
    return ret;
}

void Connection::createStoragePool(const QString &name, const QString &type, const QString &source, const QString &target)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("pool"));
    stream.writeAttribute(QStringLiteral("type"), type);
    stream.writeTextElement(QStringLiteral("name"), name);


    if (type == QLatin1String("logical")) {
        stream.writeStartElement(QStringLiteral("source"));

        stream.writeStartElement(QStringLiteral("device"));
        stream.writeAttribute(QStringLiteral("path"), source);
        stream.writeEndElement(); // device

        stream.writeTextElement(QStringLiteral("name"), name);

        stream.writeStartElement(QStringLiteral("format"));
        stream.writeAttribute(QStringLiteral("type"), QStringLiteral("lvm2"));
        stream.writeEndElement(); // format

        stream.writeEndElement(); // source


    }

    stream.writeStartElement(QStringLiteral("target"));
    if (type == QLatin1String("logical")) {
        stream.writeTextElement(QStringLiteral("path"), QLatin1String("/dev/") + name);
    } else {
        stream.writeTextElement(QStringLiteral("path"), target);
    }
    stream.writeEndElement(); // target

    stream.writeEndElement(); // pool
    qDebug() << "XML output" << output;

    virStoragePoolPtr pool = virStoragePoolDefineXML(m_conn, output.constData(), 0);
    if (!pool) {
        qDebug() << "virStoragePoolDefineXML" << output;
        return;
    }
    StoragePool storage(pool, this);
    if (type == QLatin1String("logical")) {
        storage.build(0);
    }
    storage.create(0);
    storage.setAutostart(true);
}

void Connection::createStoragePoolCeph(const QString &name, const QString &ceph_pool, const QString &ceph_host, const QString &ceph_user, const QString &secret_uuid)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("pool"));
    stream.writeAttribute(QStringLiteral("type"), QStringLiteral("rbd"));
    stream.writeTextElement(QStringLiteral("name"), name);

    stream.writeStartElement(QStringLiteral("source"));

    stream.writeStartElement(QStringLiteral("host"));
    stream.writeAttribute(QStringLiteral("name"), ceph_host);
    stream.writeAttribute(QStringLiteral("port"), QStringLiteral("6789"));
    stream.writeEndElement(); // host

    stream.writeTextElement(QStringLiteral("name"), ceph_pool);

    stream.writeStartElement(QStringLiteral("auth"));
    stream.writeAttribute(QStringLiteral("username"), ceph_user);
    stream.writeAttribute(QStringLiteral("type"), QStringLiteral("ceph"));

    stream.writeStartElement(QStringLiteral("secret"));
    stream.writeAttribute(QStringLiteral("uuid"), secret_uuid);
    stream.writeEndElement(); // secret
    stream.writeEndElement(); // auth

    stream.writeEndElement(); // source

    stream.writeEndElement(); // pool
    qDebug() << "XML output" << output;

    virStoragePoolPtr pool = virStoragePoolDefineXML(m_conn, output.constData(), 0);
    if (!pool) {
        qDebug() << "virStoragePoolDefineXML" << output;
        return;
    }
    StoragePool storage(pool, this);
    storage.create(0);
    storage.setAutostart(true);
}

void Connection::createStoragePoolNetFs(const QString &name, const QString &netfs_host, const QString &source, const QString &source_format, const QString &target)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    stream.writeStartElement(QStringLiteral("pool"));
    stream.writeAttribute(QStringLiteral("type"), QStringLiteral("netfs"));
    stream.writeTextElement(QStringLiteral("name"), name);

    stream.writeStartElement(QStringLiteral("source"));

    stream.writeStartElement(QStringLiteral("host"));
    stream.writeAttribute(QStringLiteral("name"), netfs_host);
    stream.writeEndElement(); // host

    stream.writeStartElement(QStringLiteral("dir"));
    stream.writeAttribute(QStringLiteral("path"), source);
    stream.writeEndElement(); // dir

    stream.writeStartElement(QStringLiteral("format"));
    stream.writeAttribute(QStringLiteral("type"), source_format);
    stream.writeEndElement(); // format

    stream.writeEndElement(); // source

    stream.writeStartElement(QStringLiteral("target"));
    stream.writeTextElement(QStringLiteral("path"), target);
    stream.writeEndElement(); // target

    stream.writeEndElement(); // pool
    qDebug() << "XML output" << output;

    virStoragePoolPtr pool = virStoragePoolDefineXML(m_conn, output.constData(), 0);
    if (!pool) {
        qDebug() << "virStoragePoolDefineXML" << output;
        return;
    }
    StoragePool storage(pool, this);
    storage.create(0);
    storage.setAutostart(true);
}

StoragePool *Connection::getStoragePoll(const QString &name, QObject *parent)
{

    virStoragePoolPtr pool = virStoragePoolLookupByName(m_conn, name.toUtf8().constData());
    if (!pool) {
        return nullptr;
    }
    return new StoragePool(pool, this, parent);
}

QVector<NodeDevice *> Connection::nodeDevices(uint flags, QObject *parent)
{
    QVector<NodeDevice *> ret;
    virNodeDevicePtr *nodes;
    int count = virConnectListAllNodeDevices(m_conn, &nodes, flags);
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            auto node = new NodeDevice(nodes[i], this, parent);
            ret.append(node);
        }
        free(nodes);
    }
    return ret;
}

void Connection::loadNodeInfo()
{
    m_nodeInfoLoaded = true;
    virNodeGetInfo(m_conn, &m_nodeInfo);
}

void Connection::loadDomainCapabilities()
{
    char *xml = virConnectGetDomainCapabilities(m_conn, NULL, NULL, NULL, NULL, 0);
    if (!xml) {
        qCWarning(VIRT_CONN) << "Failed to load domain capabilities";
        return;
    }
    const QString xmlString = QString::fromUtf8(xml);
    qDebug() << xml;
    free(xml);

    QString errorString;
    if (!m_xmlCapsDoc.setContent(xmlString, &errorString)) {
        qCCritical(VIRT_CONN) << "error" << m_xmlCapsDoc.isNull() << m_xmlCapsDoc.toString();
        return;
    }

    m_domainCapabilitiesLoaded = true;
}

QString Connection::dataFromSimpleNode(const QString &element) const
{
    QDomElement docElem = m_xmlCapsDoc.documentElement();
    QDomElement node = docElem.firstChildElement(element);
    QDomNode data = node.firstChild();
    return data.nodeValue();
}
