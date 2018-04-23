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

Q_LOGGING_CATEGORY(V_CONSOLE, "virtlyst.console")

Console::Console(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Console::index(Context *c, const QString &hostId, const QString &uuid)
{
    QStringList errors;
    c->setStash(QStringLiteral("host_id"), hostId);

    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qCWarning(V_CONSOLE) << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }
    c->setStash(QStringLiteral("host"), QVariant::fromValue(conn));

    Domain *dom = conn->getDomainByUuid(uuid, c);
    if (!dom) {
        errors.append(QStringLiteral("Domain not found: no domain with matching name '%1'").arg(uuid));
        c->setStash(QStringLiteral("errors"), errors);
        return;
    }

    const QString type = dom->consoleType();
    if (type == QLatin1String("spice")) {
        c->setStash(QStringLiteral("template"), QStringLiteral("console-spice.html"));
    } else if (type == QLatin1String("vnc")) {
        c->setStash(QStringLiteral("template"), QStringLiteral("console-vnc.html"));
    } else {
        qCDebug(V_CONSOLE) << "Console type not known for domain" << uuid;
        return;
    }
    const QUrl uri = c->request()->uri();
    c->setStash(QStringLiteral("domain"), QVariant::fromValue(dom));
    c->setStash(QStringLiteral("ws_host"), uri.host());
    c->setStash(QStringLiteral("ws_port"), uri.port(c->request()->secure() ? 443 : 80));
    c->setStash(QStringLiteral("console_passwd"), dom->consolePassword());
    const QString path = QLatin1String("console/ws/") + hostId + QLatin1Char('/') + uuid;
    c->setStash(QStringLiteral("ws_path"), path);
}

void Console::ws(Context *c, const QString &hostId, const QString &uuid)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qCWarning(V_CONSOLE) << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    Domain *dom = conn->getDomainByUuid(uuid, c);
    if (!dom) {
        qCDebug(V_CONSOLE) << "Domain not found: no domain with matching name '%1'" << uuid;
        return;
    }

    if (!c->response()->webSocketHandshake(QString(), QString(), QStringLiteral("binary"))) {
        qCWarning(V_CONSOLE) << "Failed to estabilish websocket handshake";
        return;
    }

    const QString host = dom->consoleListenAddress();
    const quint32 port = dom->consolePort();

    auto sock = new QTcpSocket(c);
    sock->connectToHost(host, port);
    qCDebug(V_CONSOLE) << "Connecting TCP socket to" << host << port;

    connect(sock, &QTcpSocket::readyRead, c, [=] {
        const QByteArray data = sock->readAll();
//        qCWarning(V_CONSOLE) << "Console Proxy socket data" << data.size();
        c->response()->webSocketBinaryMessage(data);
    });
    connect(sock, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            c, [=] (QAbstractSocket::SocketError error) {
        qCWarning(V_CONSOLE) << "Console Proxy error:" << error << sock->errorString();
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });
    connect(sock, &QTcpSocket::disconnected, c, [=] {
        qCWarning(V_CONSOLE) << "Console Proxy socket disconnected";
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });
    auto buf = new QByteArray;//this will leak
    connect(sock, &QTcpSocket::connected, c, [=] {
        qCWarning(V_CONSOLE) << "Console Proxy socket connected from" << host << port;
        if (!buf->isNull()) {
            sock->write(*buf);
            sock->flush();
        }
    });

    connect(c->request(), &Request::webSocketBinaryFrame, c, [=] (const QByteArray &message) {
//        qCWarning(V_CONSOLE) << "Console Proxy ws data" << message.size();
        if (sock->state() != QAbstractSocket::ConnectedState) {
            buf->append(message);
            return;
        }
        sock->write(message);
        sock->flush();
    });

}
