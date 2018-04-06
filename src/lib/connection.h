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
#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QDomDocument>
#include <libvirt/libvirt.h>

class Domain;
class Interface;
class Network;
class Secret;
class Connection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString uri READ uri CONSTANT)
    Q_PROPERTY(QString hostname READ hostname CONSTANT)
    Q_PROPERTY(QString hypervisor READ hypervisor CONSTANT)
    Q_PROPERTY(QString memoryPretty READ memoryPretty CONSTANT)
    Q_PROPERTY(uint cpus READ cpus CONSTANT)
    Q_PROPERTY(QString arch READ arch CONSTANT)
    Q_PROPERTY(QString cpuVendor READ cpuVendor CONSTANT)
public:
    explicit Connection(const QString &uri, QObject *parent = nullptr);
    ~Connection();

    QString uri() const;
    QString hostname() const;
    QString hypervisor() const;
    quint64 freeMemoryBytes() const;

    quint64 memory();
    QString memoryPretty();
    uint cpus();

    int maxVcpus() const;

    QString arch();
    QString cpuVendor();

    bool domainDefineXml(const QString &xml);

    QVector<Domain *> domains(int flags, QObject *parent = nullptr);
    QVector<Interface *> interfaces(uint flags, QObject *parent = nullptr);
    QVector<Network *> networks(uint flags, QObject *parent = nullptr);
    QVector<Secret *> secrets(uint flags, QObject *parent = nullptr);

    virConnectPtr raw() const;

private:
    void loadNodeInfo();
    void loadDomainCapabilities();
    QString dataFromSimpleNode(const QString &element) const;

    QString m_uri;
    virConnectPtr m_conn;
    virNodeInfo m_nodeInfo;
    QDomDocument m_xmlCapsDoc;
    bool m_nodeInfoLoaded = false;
    bool m_domainCapabilitiesLoaded = false;
};

#endif // CONNECTION_H
