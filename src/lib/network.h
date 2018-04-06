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
#include <libvirt/libvirt.h>

class Connection;
class Network : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString bridgeName READ bridgeName CONSTANT)
public:
    explicit Network(virNetworkPtr net, Connection *conn, QObject *parent = nullptr);
    ~Network();

    QString name() const;
    QString bridgeName() const;

private:
    virNetworkPtr m_net;
    Connection *m_conn;
};

#endif // NETWORK_H
