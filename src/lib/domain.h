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
#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class Connection;
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
    Q_PROPERTY(int memoryMiB READ memoryMiB CONSTANT)
    Q_PROPERTY(quint64 currentMemory READ currentMemory CONSTANT)
    Q_PROPERTY(int currentMemoryMiB READ currentMemoryMiB CONSTANT)
    Q_PROPERTY(QString currentMemoryPretty READ currentMemoryPretty CONSTANT)
    Q_PROPERTY(bool hasManagedSaveImage READ hasManagedSaveImage CONSTANT)
    Q_PROPERTY(bool autostart READ autostart CONSTANT)
public:
    explicit Domain(virDomainPtr domain, Connection *conn, QObject *parent = nullptr);
    ~Domain();

    bool loadXml(bool secure = true);
    bool saveXml();

    QString xml() const;

    QString name() const;
    QString uuid() const;

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    int status() const;

    int currentVcpu() const;
    void setCurrentVcpu(int number);

    int vcpu() const;
    void setVcpu(int number);

    quint64 memory() const;
    void setMemory(quint64 kBytes);
    int memoryMiB() const;

    quint64 currentMemory() const;
    void setCurrentMemory(quint64 kBytes);
    int currentMemoryMiB() const;

    QString currentMemoryPretty() const;

    bool hasManagedSaveImage() const;
    bool autostart() const;

private:
    QString dataFromSimpleNode(const QString &element) const;
    void setDataToSimpleNode(const QString &element, const QString &data);

    Connection *m_conn;
    virDomainPtr m_domain;
    virDomainInfo m_info;
    QDomDocument m_xmlDoc;
};

#endif // DOMAIN_H
