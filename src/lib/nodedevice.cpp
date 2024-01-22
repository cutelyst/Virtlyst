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
#include "nodedevice.h"

#include <QLoggingCategory>

NodeDevice::NodeDevice(virNodeDevicePtr nodeDev, Connection *conn, QObject *parent)
    : QObject(parent)
    , m_node(nodeDev)
    , m_conn(conn)
{
}

QString NodeDevice::capabilityInterface()
{
    return xml()
        .documentElement()
        .firstChildElement(QStringLiteral("capability"))
        .firstChildElement(QStringLiteral("interface"))
        .firstChild()
        .nodeValue();
}

QDomDocument NodeDevice::xml()
{
    if (m_xml.isNull()) {
        char *xml               = virNodeDeviceGetXMLDesc(m_node, 0);
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
