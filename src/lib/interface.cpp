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
#include "interface.h"

#include <QLoggingCategory>

Interface::Interface(virInterfacePtr iface, Connection *conn, QObject *parent) : QObject(parent)
  , m_iface(iface)
  , m_conn(conn)
{

}

Interface::~Interface()
{
    virInterfaceFree(m_iface);
}

QString Interface::name()
{
    return xml()
            .documentElement()
            .attribute(QStringLiteral("name"));
}

QString Interface::mac() const
{
    return QString::fromUtf8(virInterfaceGetMACString(m_iface));
}

QString Interface::type()
{
    return xml()
            .documentElement()
            .attribute(QStringLiteral("type"));
}

QString Interface::ipv4()
{
    return xmlGetAddress(QStringLiteral("ipv4"));
}

QString Interface::ipv4Type()
{
    if (ipv4().isEmpty()) {
        return QStringLiteral("dhcp");
    } else {
        return QStringLiteral("static");
    }
}

QString Interface::ipv6()
{
    return xmlGetAddress(QStringLiteral("ipv6"));
}

QString Interface::ipv6Type()
{
    if (ipv6().isEmpty()) {
        return QStringLiteral("dhcp");
    } else {
        return QStringLiteral("static");
    }
}

QString Interface::startMode()
{
    return xml()
            .documentElement()
            .firstChildElement(QStringLiteral("start"))
            .attribute(QStringLiteral("mode"));
}

QDomDocument Interface::xml()
{
    if (m_xml.isNull()) {
        char *xml = virInterfaceGetXMLDesc(m_iface, 0);
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

QString Interface::xmlGetAddress(const QString &family)
{
    QDomElement protocol = xml().documentElement().firstChildElement(QStringLiteral("protocol"));
    while (!protocol.isNull()) {
        if (protocol.attribute(QStringLiteral("family")) == family) {
            QDomElement ip = protocol.firstChildElement(QStringLiteral("ip"));
            while (!ip.isNull()) {
                if (ip.hasAttribute(QStringLiteral("address"))) {
                    const QString address = ip.attribute(QStringLiteral("address"));
                    if (ip.hasAttribute(QStringLiteral("prefix"))) {
                        return address + QLatin1Char('/') + ip.attribute(QStringLiteral("prefix"));
                    }
                    return address;
                }
                ip = ip.nextSiblingElement(QStringLiteral("ip"));
            }
        }
        protocol = protocol.nextSiblingElement(QStringLiteral("protocol"));
    }
    return QString();
}
