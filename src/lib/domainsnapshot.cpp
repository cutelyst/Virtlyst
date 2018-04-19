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
#include "domainsnapshot.h"

#include <QDateTime>
#include <QLoggingCategory>

DomainSnapshot::DomainSnapshot(virDomainSnapshotPtr snap, QObject *parent) : QObject(parent)
  , m_snap(snap)
{

}

DomainSnapshot::~DomainSnapshot()
{
    virDomainSnapshotFree(m_snap);
}

QString DomainSnapshot::name()
{
    return QString::fromUtf8(virDomainSnapshotGetName(m_snap));
}

QString DomainSnapshot::date()
{
    const QString creationTime = xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("creationTime"))
            .firstChild()
            .nodeValue();
    const QDateTime date = QDateTime::fromMSecsSinceEpoch(creationTime.toLongLong() * 1000);
//    qDebug() << creationTime << date << date.toString();
    return date.toString();
}

bool DomainSnapshot::undefine()
{
    return virDomainSnapshotDelete(m_snap, 0);
}

bool DomainSnapshot::revert()
{
    return virDomainRevertToSnapshot(m_snap, 0) == 0;
}

QDomDocument DomainSnapshot::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virDomainSnapshotGetXMLDesc(m_snap, VIR_DOMAIN_XML_SECURE);
        const QString xmlString = QString::fromUtf8(xml);
//        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from snapshot" << error;
        }
        free(xml);
    }
    return m_xml;
}
