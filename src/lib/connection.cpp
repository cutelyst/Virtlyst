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

virConnectPtr Connection::raw() const
{
    return m_conn;
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
