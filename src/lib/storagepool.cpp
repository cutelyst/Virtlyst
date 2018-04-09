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
#include "storagepool.h"

#include "virtlyst.h"
#include "storagevol.h"

#include <QLoggingCategory>

StoragePool::StoragePool(virStoragePoolPtr storage, Connection *conn, QObject *parent) : QObject(parent)
  , m_storage(storage)
  , m_conn(conn)
{

}

QString StoragePool::name()
{
    return QString::fromUtf8(virStoragePoolGetName(m_storage));
}

QString StoragePool::type()
{
    return xmlDoc()
            .documentElement()
            .attribute(QStringLiteral("type"));
}

QString StoragePool::size()
{
    if (getInfo()) {
        return Virtlyst::prettyKibiBytes(m_info.capacity / 1024);
    }
    return QString();
}

QString StoragePool::used()
{
    if (getInfo()) {
        return Virtlyst::prettyKibiBytes(m_info.allocation / 1024);
    }
    return QString();
}

int StoragePool::percent()
{
    if (getInfo()) {
        return double(m_info.allocation) / m_info.capacity * 100;
    }
    return -1;
}

int StoragePool::state()
{
    if (getInfo()) {
        return m_info.state;
    }
    return -1;
}

QString StoragePool::status()
{
    switch (state()) {
    case VIR_STORAGE_POOL_INACTIVE:
        return QStringLiteral("Not running");
    case VIR_STORAGE_POOL_BUILDING:
        return QStringLiteral("Initializing pool, not available");
    case VIR_STORAGE_POOL_RUNNING:
        return QStringLiteral("Running normally");
    case VIR_STORAGE_POOL_DEGRADED:
        return QStringLiteral("Running degraded");
    case VIR_STORAGE_POOL_INACCESSIBLE:
        return QStringLiteral("Running, but not accessible");
    default:
        return QStringLiteral("Unknown");

    }
}

bool StoragePool::active()
{
    return virStoragePoolIsActive(m_storage) == 1;
}

bool StoragePool::autostart()
{
    int autostart;
    if (virStoragePoolGetAutostart(m_storage, &autostart) == 0) {
        return autostart == 1;
    }
    return false;
}

int StoragePool::volumeCount()
{
    return virStoragePoolNumOfVolumes(m_storage);
}

QString StoragePool::path()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("target"))
            .firstChildElement(QStringLiteral("path"))
            .firstChild().nodeValue();
}

void StoragePool::start()
{
    virStoragePoolCreate(m_storage, 0);
}

void StoragePool::stop()
{
    virStoragePoolDestroy(m_storage);
}

void StoragePool::undefine()
{
    virStoragePoolUndefine(m_storage);
}

void StoragePool::setAutostart(bool enable)
{
    virStoragePoolSetAutostart(m_storage, enable ? 1 : 0);
}

QVariant StoragePool::volumes()
{
    return QVariant::fromValue(storageVols(0));
}

QVector<StorageVol *> StoragePool::storageVols(unsigned int flags)
{
    if (!m_gotVols) {
        virStorageVolPtr *vols;
        int count = virStoragePoolListAllVolumes(m_storage, &vols, flags);
        if (count > 0) {
            for (int i = 0; i < count; ++i) {
                auto vol = new StorageVol(vols[i], this);
                m_vols.append(vol);
            }
            free(vols);
        }
        m_gotVols = true;
    }

    return m_vols;
}

QDomDocument StoragePool::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virStoragePoolGetXMLDesc(m_storage, 0);
        const QString xmlString = QString::fromUtf8(xml);
        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from interface" << error;
        }
        free(xml);
    }
    return m_xml;
}

bool StoragePool::getInfo()
{
    if (virStoragePoolGetInfo(m_storage, &m_info) == 0) {
        m_gotInfo = true;
    }
    return m_gotInfo;
}
