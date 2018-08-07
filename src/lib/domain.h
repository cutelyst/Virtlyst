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
#ifndef DOMAIN_H
#define DOMAIN_H

#include <QMap>
#include <QHash>
#include <QObject>
#include <QVector>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class Connection;
class DomainSnapshot;
class Domain : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString xml READ xml CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString uuid READ uuid CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(int status READ status CONSTANT)
    Q_PROPERTY(int currentVcpu READ currentVcpu CONSTANT)
    Q_PROPERTY(int vcpu READ vcpu CONSTANT)
    Q_PROPERTY(quint64 memory READ memory CONSTANT)
    Q_PROPERTY(quint64 memoryMiB READ memoryMiB CONSTANT)
    Q_PROPERTY(quint64 currentMemory READ currentMemory CONSTANT)
    Q_PROPERTY(quint64 currentMemoryMiB READ currentMemoryMiB CONSTANT)
    Q_PROPERTY(QString currentMemoryPretty READ currentMemoryPretty CONSTANT)
    Q_PROPERTY(bool hasManagedSaveImage READ hasManagedSaveImage CONSTANT)
    Q_PROPERTY(bool autostart READ autostart CONSTANT)
    Q_PROPERTY(QString consoleType READ consoleType CONSTANT)
    Q_PROPERTY(QString consolePassword READ consolePassword CONSTANT)
    Q_PROPERTY(QString consoleKeymap READ consoleKeymap CONSTANT)
    Q_PROPERTY(QVariantList disks READ disks CONSTANT)
    Q_PROPERTY(QVariantList cloneDisks READ cloneDisks CONSTANT)
    Q_PROPERTY(QVariantList media READ media CONSTANT)
    Q_PROPERTY(QVariantList networks READ networks CONSTANT)
    Q_PROPERTY(QVariantList snapshots READ snapshots CONSTANT)
public:
    explicit Domain(virDomainPtr domain, Connection *conn, QObject *parent = nullptr);
    ~Domain();

    bool saveXml();

    QString xml();

    QString name() const;
    QString uuid();

    QString title();
    void setTitle(const QString &title);

    QString description();
    void setDescription(const QString &description);

    int status();

    int currentVcpu();
    void setCurrentVcpu(int number);

    int vcpu();
    void setVcpu(int number);

    quint64 memory();
    void setMemory(quint64 kBytes);
    quint64 memoryMiB();

    quint64 currentMemory();
    void setCurrentMemory(quint64 kBytes);
    quint64 currentMemoryMiB();

    QString currentMemoryPretty();

    bool hasManagedSaveImage() const;
    bool autostart() const;

    bool snapshot(const QString &name);
    QVariantList snapshots();
    DomainSnapshot *getSnapshot(const QString &name);

    QString consoleType();
    void setConsoleType(const QString &type);

    QString consolePassword();
    void setConsolePassword(const QString &password);

    quint16 consolePort();
    QString consoleListenAddress();
    QString consoleKeymap();
    void setConsoleKeymap(const QString &keymap);

    int cpuUsage();
    QVector<std::pair<qint64, qint64>> netUsageMiBs();
    QMap<QString, std::pair<qint64, qint64>> hddUsageMiBs();
    QStringList blkDevices();
    QVariantList disks();
    QVariantList cloneDisks();
    QVariantList media();
    QVariantList networks();
    QStringList networkTargetDevs();

    void start();
    void shutdown();
    void suspend();
    void resume();
    void destroy();
    void undefine();
    void managedSave();
    void managedSaveRemove();
    void setAutostart(bool enable);

    bool attachDevice(const QString &xml);
    bool updateDevice(const QString &xml, uint flags);

    void mountIso(const QString &dev, const QString &image);
    void umountIso(const QString &dev, const QString &image);

private:
    QDomDocument xmlDoc();
    QString dataFromSimpleNode(const QString &element);
    void setDataToSimpleNode(const QString &element, const QString &data);

    bool getStats();

    QVariantHash m_cache;
    Connection *m_conn;
    virDomainPtr m_domain;
    virDomainInfo m_info;
    QDomDocument m_xml;
    int m_cpuUsage = 0;
    QVector<std::pair<qint64, qint64>> m_netUsageMiBs;
    QMap<QString, std::pair<qint64, qint64>> m_hddUsageMiBs;
    bool m_gotStats = false;
    bool m_gotInfo = false;
};

#endif // DOMAIN_H
