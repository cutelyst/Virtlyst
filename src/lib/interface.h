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
#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <libvirt/libvirt.h>

class Connection;
class Interface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString mac READ mac CONSTANT)
public:
    explicit Interface(virInterfacePtr iface, Connection *conn, QObject *parent = nullptr);
    ~Interface();

    QString name() const;
    QString type() const;
    QString mac() const;

private:
    virInterfacePtr m_iface;
    Connection *m_conn;
};

#endif // INTERFACE_H
