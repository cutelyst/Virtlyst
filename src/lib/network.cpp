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
#include "network.h"

#include <QHostAddress>
#include <QVariant>
#include <QLoggingCategory>

Network::Network(virNetworkPtr net, Connection *conn, QObject *parent) : QObject(parent)
  , m_net(net)
  , m_conn(conn)
{

}

Network::~Network()
{
    virNetworkFree(m_net);
}

QString Network::name() const
{
    return QString::fromUtf8(virNetworkGetName(m_net));
}

QString Network::bridgeName() const
{
    return QString::fromUtf8(virNetworkGetBridgeName(m_net));
}

bool Network::autostart()
{
    int autostart;
    if (virNetworkGetAutostart(m_net, &autostart) == 0) {
        return autostart == 1;
    }
    return false;
}

void Network::setAutostart(bool enable)
{
    virNetworkSetAutostart(m_net, enable ? 1 : 0);
}

bool Network::active()
{
    return virNetworkIsActive(m_net) == 1;
}

void Network::start()
{
    virNetworkCreate(m_net);
}

void Network::stop()
{
    virNetworkDestroy(m_net);
}

void Network::undefine()
{
    virNetworkUndefine(m_net);
}

QString Network::forwardMode()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("forward"))
            .attribute(QStringLiteral("mode"));
}

QString Network::ipAddress()
{
    QDomElement ip = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("ip"));

    QPair<QHostAddress, int> pair =
            QHostAddress::parseSubnet(
                ip.attribute(QStringLiteral("address")) + QLatin1Char('/') + ip.attribute(QStringLiteral("netmask")));
    if (pair.first.isNull()) {
        return QStringLiteral("None");
    }
    return pair.first.toString() + QLatin1Char('/') + QString::number(pair.second);
}

QString Network::ipDhcpRangeStart()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("ip"))
            .firstChildElement(QStringLiteral("dhcp"))
            .firstChildElement(QStringLiteral("range"))
            .attribute(QStringLiteral("start"));
}

QString Network::ipDhcpRangeEnd()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("ip"))
            .firstChildElement(QStringLiteral("dhcp"))
            .firstChildElement(QStringLiteral("range"))
            .attribute(QStringLiteral("end"));
}

QVariantList Network::ipDhcpHosts()
{
    QVariantList ret;
    QDomElement host = xmlDoc()
                .documentElement()
                .firstChildElement(QStringLiteral("ip"))
                .firstChildElement(QStringLiteral("dhcp"))
                .firstChildElement(QStringLiteral("host"));
    while (!host.isNull()) {
        QMap<QString, QString> map{
            {QStringLiteral("ip"), host.attribute(QStringLiteral("ip"))},
            {QStringLiteral("mac"), host.attribute(QStringLiteral("mac"))},
        };
        ret.append(QVariant::fromValue(map));
        host = host.nextSiblingElement(QStringLiteral("host"));
    }
    return ret;
}

QString Network::ipAddressForMac(const QString &mac)
{
    QDomElement host = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("ip"))
            .firstChildElement(QStringLiteral("dhcp"))
            .firstChildElement(QStringLiteral("host"));
    while (!host.isNull()) {
        if (host.attribute(QStringLiteral("host")) == mac) {
            return host.attribute(QStringLiteral("ip"));
        }
        host = host.nextSiblingElement(QStringLiteral("host"));
    }
    return QString();
}

QDomDocument Network::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virNetworkGetXMLDesc(m_net, 0);
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
