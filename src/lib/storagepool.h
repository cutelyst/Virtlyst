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
#ifndef STORAGEPOOL_H
#define STORAGEPOOL_H

#include <QObject>
#include <QDomDocument>
#include <QVector>
#include <libvirt/libvirt.h>

class StorageVol;
class Connection;
class StoragePool : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString size READ size CONSTANT)
    Q_PROPERTY(QString used READ used CONSTANT)
    Q_PROPERTY(int percent READ percent CONSTANT)
    Q_PROPERTY(int state READ state CONSTANT)
    Q_PROPERTY(QString status READ status CONSTANT)
    Q_PROPERTY(bool active READ active CONSTANT)
    Q_PROPERTY(bool autostart READ autostart CONSTANT)
    Q_PROPERTY(int volumeCount READ volumeCount CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QVariant volumes READ volumes CONSTANT)
public:
    explicit StoragePool(virStoragePoolPtr storage, QObject *parent = nullptr);

    QString name();
    QString type();
    QString size();
    QString used();
    int percent();
    int state();
    QString status();
    bool active();
    bool autostart();
    int volumeCount();

    QString path();

    bool start();
    bool stop();
    bool undefine();
    bool setAutostart(bool enable);

    QVariant volumes();
    QVector<StorageVol *> storageVols(unsigned int flags = 0);

    bool build(int flags);
    bool create(int flags);
    StorageVol *createStorageVolume(const QString &name, const QString &format, int sizeGiB, int flags);
//    bool cloneStorageVolume(StorageVol *volume, const QString &name, const QString &format, int flags);
    StorageVol *getVolume(const QString &name);

private:
    QDomDocument xmlDoc();
    bool getInfo();

    QDomDocument m_xml;
    virStoragePoolPtr m_pool;
    QVector<StorageVol *> m_vols;
    virStoragePoolInfo m_info;
    bool m_gotVols = false;
    bool m_gotInfo = false;
};

#endif // STORAGEPOOL_H
