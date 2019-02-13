#include "ws.h"

#include "virtlyst.h"
#include "lib/connection.h"
#include "lib/domain.h"

#include <libvirt/libvirt.h>

#include <QTcpSocket>

#include <QLoggingCategory>
#include <QBuffer>

Q_LOGGING_CATEGORY(V_WS, "virtlyst.ws")

Ws::Ws(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Ws::index(Context *c, const QString &hostId, const QString &uuid)
{
    Connection *conn = m_virtlyst->connection(hostId, c);
    if (conn == nullptr) {
        qCWarning(V_WS) << "Host id not found or connection not active";
        c->response()->redirect(c->uriForAction(QStringLiteral("/index")));
        return;
    }

    Domain *dom = conn->getDomainByUuid(uuid, c);
    if (!dom) {
        qCDebug(V_WS) << "Domain not found: no domain with matching name '%1'" << uuid;
        return;
    }

    if (!c->response()->webSocketHandshake(QString(), QString(), QStringLiteral("binary"))) {
        qCWarning(V_WS) << "Failed to estabilish websocket handshake";
        return;
    }

    const quint16 port = dom->consolePort();
    QString host = dom->consoleListenAddress();

    // if the remote or local domain is listening on
    // all addresses better use the connection hostname
    if (host == QLatin1String("0.0.0.0")) {
        host = QUrl(conn->uri()).host();
        if (host.isEmpty()) {
            host = QStringLiteral("0.0.0.0");
        }
    }

    auto sock = new QTcpSocket(c);
    sock->connectToHost(host, port);
    qCDebug(V_WS) << "Connecting TCP socket to" << host << port;

    connect(sock, &QTcpSocket::readyRead, c, [=] {
        const QByteArray data = sock->readAll();
//        qCWarning(V_WS) << "Console Proxy socket data" << data.size();
        c->response()->webSocketBinaryMessage(data);
    });
    connect(sock, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            c, [=] (QAbstractSocket::SocketError error) {
        qCWarning(V_WS) << "Console Proxy error:" << error << sock->errorString();
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });
    connect(sock, &QTcpSocket::disconnected, c, [=] {
        qCWarning(V_WS) << "Console Proxy socket disconnected";
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection, sock->errorString());
    });

    auto buf = new QBuffer(c);
    connect(sock, &QTcpSocket::connected, c, [=] {
        qCWarning(V_WS) << "Console Proxy socket connected from" << host << port;
        if (buf->isOpen()) {
            sock->write(buf->readAll());
            sock->flush();
            buf->close();
        }
    });

    connect(c->request(), &Request::webSocketBinaryFrame, c, [=] (const QByteArray &message) {
//        qCWarning(V_WS) << "Console Proxy ws data" << message.size();
        if (sock->state() != QAbstractSocket::ConnectedState) {
            buf->open(QIODevice::ReadWrite);
            buf->write(message);
            return;
        }
        sock->write(message);
        sock->flush();
    });
}
