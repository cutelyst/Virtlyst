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
#include "domain.h"
#include "connection.h"

#include "virtlyst.h"

#include <QLoggingCategory>

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

bool Domain::loadXml(bool secure)
{
    char *xml = virDomainGetXMLDesc(m_domain, secure ? VIR_DOMAIN_XML_SECURE : 0);
    const QString xmlString = QString::fromUtf8(xml);
    free(xml);

    QString errorString;
    if (!m_xmlDoc.setContent(xmlString, &errorString)) {
        qCCritical(VIRT_DOM) << "error" << m_xmlDoc.isNull() << m_xmlDoc.toString();
        return false;
    }
    //    qCCritical(VIRT_DOM) << "LOADED" << m_xmlDoc.isNull() << m_xmlDoc.toString();
    qCCritical(VIRT_DOM) << "LOADED" << memory() << currentMemory();

    if (virDomainGetInfo(m_domain, &m_info) < 0) {
        qCWarning(VIRT_DOM) << "Failed to get info for domain" << name();
    }

    return true;
}

bool Domain::saveXml()
{
    qCCritical(VIRT_DOM) << m_xmlDoc.toString();
    return m_conn->domainDefineXml(m_xmlDoc.toString(0));
}

QString Domain::xml() const
{
    return m_xmlDoc.toString(2);
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

QString Domain::uuid() const
{
    return dataFromSimpleNode(QStringLiteral("uuid"));
}

QString Domain::title() const
{
    return dataFromSimpleNode(QStringLiteral("title"));
}

void Domain::setTitle(const QString &title)
{
    setDataToSimpleNode(QStringLiteral("title"), title);
}

QString Domain::description() const
{
    return dataFromSimpleNode(QStringLiteral("description"));
}

void Domain::setDescription(const QString &description)
{
    setDataToSimpleNode(QStringLiteral("description"), description);
}

int Domain::status() const
{
    return m_info.state;
}

int Domain::currentVcpu() const
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    if (node.hasAttribute(QStringLiteral("current"))) {
        return node.attribute(QStringLiteral("current")).toInt();
    }
    QDomNode data = node.firstChild();
    return data.nodeValue().toInt();
}

void Domain::setCurrentVcpu(int number)
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    node.setAttribute(QStringLiteral("current"), number);
}

int Domain::vcpu() const
{
    return dataFromSimpleNode(QStringLiteral("vcpu")).toULongLong();
}

void Domain::setVcpu(int number)
{
    setDataToSimpleNode(QStringLiteral("vcpu"), QString::number(number));
}

quint64 Domain::memory() const
{
    return dataFromSimpleNode(QStringLiteral("memory")).toULongLong();
}

void Domain::setMemory(quint64 kBytes)
{
    setDataToSimpleNode(QStringLiteral("memory"), QString::number(kBytes));
}

int Domain::memoryMiB() const
{
    return memory() / 1024;
}

quint64 Domain::currentMemory() const
{
    return dataFromSimpleNode(QStringLiteral("currentMemory")).toULongLong();
}

void Domain::setCurrentMemory(quint64 kBytes)
{
    qCritical() << Q_FUNC_INFO << QString::number(kBytes) << kBytes;
    setDataToSimpleNode(QStringLiteral("currentMemory"), QString::number(kBytes));
}

int Domain::currentMemoryMiB() const
{
    return currentMemory() / 1024;
}

QString Domain::currentMemoryPretty() const
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

QString Domain::dataFromSimpleNode(const QString &element) const
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(element);
    QDomNode data = node.firstChild();
    return data.nodeValue();
}

void Domain::setDataToSimpleNode(const QString &element, const QString &data)
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(element);
    QDomNode nodeData = node.firstChild();
    nodeData.setNodeValue(data);
}
