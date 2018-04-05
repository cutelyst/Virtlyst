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
#include "console.h"

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/domain.h"

#include <libvirt/libvirt.h>

#include <QLoggingCategory>

#include <QEventLoop>
#include <QTcpSocket>

Console::Console(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Console::index(Context *c, const QString &hostId, const QString &uuid)
{
    QStringList errors;
    c->setStash(QStringLiteral("template"), QStringLiteral("console-vnc.html"));
    c->setStash(QStringLiteral("host_id"), hostId);
    c->setStash(QStringLiteral("ws_host"), QStringLiteral("localhost"));
    c->setStash(QStringLiteral("ws_port"), QStringLiteral("3000"));
    const QString path = QLatin1String("console/ws_vnc/") + hostId + QLatin1Char('/') + uuid;
    c->setStash(QStringLiteral("ws_path"), path);
    c->setStash(QStringLiteral("time_refresh"), 8000);

    Connection *conn = m_virtlyst->connection(hostId);
    if (conn == nullptr) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    virDomainPtr domain = virDomainLookupByUUIDString(conn->raw(), uuid.toUtf8().constData());
    if (!domain) {
        errors.append(QStringLiteral("Domain not found: no domain with matching name '%1'").arg(uuid));
        c->setStash(QStringLiteral("errors"), errors);
        return;
    }
    auto dom = new Domain(domain, conn, c);
    dom->loadXml();
}

void Console::ws_vnc(Context *c, const QString &hostId, const QString &uuid)
{
    if (!c->response()->webSocketHandshake(QString(), QString(), QStringLiteral("binary"))) {
        qWarning() << "Failed to estabilish websocket handshake";
        return;
    }

    auto sock = new QTcpSocket;
    sock->connectToHost(QStringLiteral("localhost"), 5900);
    connect(sock, &QTcpSocket::readyRead, c, [=] {
        const QByteArray data = sock->readAll();
        qWarning() << "TCP VNC socket in" << data.size();
        c->response()->webSocketBinaryMessage(data);
    });
    connect(sock, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            c, [=] (QAbstractSocket::SocketError error) {
        qWarning() << "TCP VNC socket error:" << error << sock->errorString();
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });
    connect(sock, &QTcpSocket::disconnected, c, [=] {
        qWarning() << "TCP VNC socket disconnected";
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });

    connect(c->request(), &Request::webSocketBinaryMessage, c, [=] (const QByteArray &message) {
        qWarning() << "TCP VNC socket out" << message.size();
        sock->write(message);
        sock->flush();
    });

}
