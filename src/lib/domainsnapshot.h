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
#ifndef DOMAINSNAPSHOT_H
#define DOMAINSNAPSHOT_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class DomainSnapshot : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString date READ date CONSTANT)
public:
    explicit DomainSnapshot(virDomainSnapshotPtr snap, QObject *parent = nullptr);
    ~DomainSnapshot();

    QString name();
    QString date();

    bool undefine();
    bool revert();

private:
    QDomDocument xmlDoc();

    QDomDocument m_xml;
    virDomainSnapshotPtr m_snap;
};

#endif // DOMAINSNAPSHOT_H
