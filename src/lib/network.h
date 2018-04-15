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
#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QDomDocument>
#include <libvirt/libvirt.h>

class Connection;
class Network : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString bridgeName READ bridgeName CONSTANT)
    Q_PROPERTY(bool autostart READ autostart CONSTANT)
    Q_PROPERTY(bool active READ active CONSTANT)
    Q_PROPERTY(QString forwardMode READ forwardMode CONSTANT)
    Q_PROPERTY(QString ipAddress READ ipAddress CONSTANT)
    Q_PROPERTY(QString ipDhcpRangeStart READ ipDhcpRangeStart CONSTANT)
    Q_PROPERTY(QString ipDhcpRangeEnd READ ipDhcpRangeEnd CONSTANT)
    Q_PROPERTY(QVariantList ipDhcpHosts READ ipDhcpHosts CONSTANT)
public:
    explicit Network(virNetworkPtr net, Connection *conn, QObject *parent = nullptr);
    ~Network();

    QString name() const;
    QString bridgeName() const;

    bool autostart();
    void setAutostart(bool enable);

    bool active();
    void start();
    void stop();
    void undefine();

    QString forwardMode();
    QString ipAddress();
    QString ipDhcpRangeStart();
    QString ipDhcpRangeEnd();
    QVariantList ipDhcpHosts();

    QString ipAddressForMac(const QString &mac);

private:
    QDomDocument xmlDoc();

    virNetworkPtr m_net;
    Connection *m_conn;
    QDomDocument m_xml;
};

#endif // NETWORK_H
