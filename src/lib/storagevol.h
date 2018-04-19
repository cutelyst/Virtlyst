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
#ifndef STORAGEVOL_H
#define STORAGEVOL_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class StoragePool;
class StorageVol : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString size READ size CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)
public:
    explicit StorageVol(virStorageVolPtr vol, virStoragePoolPtr pool, QObject *parent = nullptr);

    QString name();
    QString type();
    QString size();
    QString path();

    bool undefine(int flags = 0);
    StorageVol *clone(const QString &name, const QString &format, int flags);

    StoragePool *pool();

private:
    bool getInfo();
    QDomDocument xmlDoc();
    virStoragePoolPtr poolPtr();

    QDomDocument m_xml;
    virStoragePoolPtr m_pool;
    virStorageVolInfo m_info;
    virStorageVolPtr m_vol;
    bool m_gotInfo = false;
};

#endif // STORAGEVOL_H
