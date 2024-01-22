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
#include "secret.h"

#include <QLoggingCategory>

Secret::Secret(virSecretPtr secret, Connection *conn, QObject *parent)
    : QObject(parent)
    , m_secret(secret)
    , m_conn(conn)
{
}

QString Secret::uuid() const
{
    char buf[VIR_UUID_STRING_BUFLEN];
    if (virSecretGetUUIDString(m_secret, buf) < 0) {
        return QString();
    }
    return QString::fromUtf8(buf);
}

QString Secret::usage() const
{
    return QString::fromUtf8(virSecretGetUsageID(m_secret));
}

int Secret::usageType() const
{
    return virSecretGetUsageType(m_secret);
}

bool Secret::setValue(const QString &value)
{
    const QByteArray data = value.toUtf8();
    return virSecretSetValue(m_secret,
                             reinterpret_cast<const unsigned char *>(data.constData()),
                             data.size(),
                             0) == 0;
}
