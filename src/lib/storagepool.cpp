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

#include <QXmlStreamWriter>
#include <QLoggingCategory>

StoragePool::StoragePool(virStoragePoolPtr storage, QObject *parent) : QObject(parent)
  , m_pool(storage)
{

}

QString StoragePool::name()
{
    return QString::fromUtf8(virStoragePoolGetName(m_pool));
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
    return virStoragePoolIsActive(m_pool) == 1;
}

bool StoragePool::autostart()
{
    int autostart;
    if (virStoragePoolGetAutostart(m_pool, &autostart) == 0) {
        return autostart == 1;
    }
    return false;
}

int StoragePool::volumeCount()
{
    return virStoragePoolNumOfVolumes(m_pool);
}

QString StoragePool::path()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("target"))
            .firstChildElement(QStringLiteral("path"))
            .firstChild().nodeValue();
}

bool StoragePool::start()
{
    return virStoragePoolCreate(m_pool, 0) == 0;
}

bool StoragePool::stop()
{
    return virStoragePoolDestroy(m_pool) == 0;
}

bool StoragePool::undefine()
{
    return virStoragePoolUndefine(m_pool) == 0;
}

bool StoragePool::setAutostart(bool enable)
{
    return virStoragePoolSetAutostart(m_pool, enable ? 1 : 0) == 0;
}

QVariant StoragePool::volumes()
{
    return QVariant::fromValue(storageVols(0));
}

QVector<StorageVol *> StoragePool::storageVols(unsigned int flags)
{
    if (!m_gotVols) {
        virStoragePoolRefresh(m_pool, 0);

        virStorageVolPtr *vols;
        int count = virStoragePoolListAllVolumes(m_pool, &vols, flags);
        if (count > 0) {
            for (int i = 0; i < count; ++i) {
                auto vol = new StorageVol(vols[i], m_pool, this);
                m_vols.append(vol);
            }
            free(vols);
        }
        m_gotVols = true;
    }

    return m_vols;
}

bool StoragePool::build(int flags)
{
    return virStoragePoolBuild(m_pool, flags) == 0;
}

bool StoragePool::create(int flags)
{
    return virStoragePoolCreate(m_pool, flags) == 0;
}

StorageVol *StoragePool::createStorageVolume(const QString &name, const QString &format, qint64 sizeGiB, int flags)
{
    QByteArray output;
    QXmlStreamWriter stream(&output);

    QString localName(name);
    QString localFormat(format);
    qint64 sizeByte = sizeGiB * 1073741824;
    qint64 alloc = sizeByte;
    
    if (format == QLatin1String("unknown")) {
        localFormat = QStringLiteral("raw");
    }
    if (format == QLatin1String("dir")) {
        localName.append(QLatin1String(".img"));
        alloc = 0;
    }

    stream.writeStartElement(QStringLiteral("volume"));
    stream.writeTextElement(QStringLiteral("name"), localName);
    stream.writeTextElement(QStringLiteral("capacity"), QString::number(sizeByte));
    stream.writeTextElement(QStringLiteral("allocation"), QString::number(alloc));

    stream.writeStartElement(QStringLiteral("target"));
    stream.writeStartElement(QStringLiteral("format"));
    stream.writeAttribute(QStringLiteral("type"), format);
    stream.writeEndElement(); // format
    stream.writeEndElement(); // target

    stream.writeEndElement(); // volume
    qDebug() << "XML output" << output;

    virStorageVolPtr vol = virStorageVolCreateXML(m_pool, output.constData(), flags);
    if (vol) {
        return new StorageVol(vol, m_pool, this);
    }
    return nullptr;
}

StorageVol *StoragePool::getVolume(const QString &name)
{
    virStorageVolPtr vol = virStorageVolLookupByName(m_pool, name.toUtf8().constData());
    if (!vol) {
        return nullptr;
    }
    return new StorageVol(vol, m_pool, this);
}

QDomDocument StoragePool::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virStoragePoolGetXMLDesc(m_pool, 0);
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

bool StoragePool::getInfo()
{
    if (virStoragePoolGetInfo(m_pool, &m_info) == 0) {
        m_gotInfo = true;
    }
    return m_gotInfo;
}
