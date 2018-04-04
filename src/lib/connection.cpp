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
#include "connection.h"
#include "virtlyst.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(VIRT_CONN, "virt.connection")

Connection::Connection(const QString &name, QObject *parent) : QObject(parent)
  , m_uri(name)
{
    m_conn = virConnectOpen(name.toUtf8().constData());
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

virConnectPtr Connection::raw() const
{
    return m_conn;
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
