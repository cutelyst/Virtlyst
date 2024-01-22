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
#ifndef SECRET_H
#define SECRET_H

#include <libvirt/libvirt.h>

#include <QObject>

class Connection;
class Secret : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString uuid READ uuid CONSTANT)
    Q_PROPERTY(QString usage READ usage CONSTANT)
    Q_PROPERTY(int usageType READ usageType CONSTANT)
public:
    explicit Secret(virSecretPtr secret, Connection *conn, QObject *parent = nullptr);

    QString uuid() const;
    QString usage() const;
    int usageType() const;

    bool setValue(const QString &value);

private:
    Connection *m_conn;
    virSecretPtr m_secret;
};

#endif // SECRET_H
