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
#ifndef NODEDEVICE_H
#define NODEDEVICE_H

#include <libvirt/libvirt.h>

#include <QDomDocument>
#include <QObject>

class Connection;
class NodeDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString capabilityInterface READ capabilityInterface CONSTANT)
public:
    explicit NodeDevice(virNodeDevicePtr nodeDev, Connection *conn, QObject *parent = nullptr);

    QString capabilityInterface();

private:
    QDomDocument xml();

    QDomDocument m_xml;
    virNodeDevicePtr m_node;
    Connection *m_conn;
};

#endif // NODEDEVICE_H
