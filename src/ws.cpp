#include "ws.h"

#include "lib/connection.h"
#include "lib/domain.h"
#include "virtlyst.h"

#include <libvirt/libvirt.h>

#include <QBuffer>
#include <QLoggingCategory>
#include <QProcess>
#include <QTcpSocket>

Q_LOGGING_CATEGORY(V_WS, "virtlyst.ws")

Ws::Ws(Virtlyst *parent)
    : Controller(parent)
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

    if (!c->response()->webSocketHandshake({}, {}, "binary"_qba)) {
        qCWarning(V_WS) << "Failed to estabilish websocket handshake";
        return;
    }

    const quint16 port = dom->consolePort();
    QString host       = dom->consoleListenAddress();
    auto sock          = new QTcpSocket(c);

    // if the remote or local domain is listening on
    // all addresses better use the connection hostname
    if (host == u"0.0.0.0") {
        host = QUrl(conn->uri()).host();
        if (host.isEmpty()) {
            host = QStringLiteral("0.0.0.0");
        }
    } else if (host == u"127.0.0.1" && conn->uri().contains(u"ssh://")) {
        createSshTunnel(sock, QUrl(conn->uri()), port);
    }

    sock->connectToHost(host, port);
    qCDebug(V_WS) << "Connecting TCP socket to" << host << port;

    connect(sock, &QTcpSocket::readyRead, c, [=] {
        const QByteArray data = sock->readAll();
        //        qCWarning(V_WS) << "Console Proxy socket data" << data.size();
        c->response()->webSocketBinaryMessage(data);
    });
    connect(sock, &QTcpSocket::errorOccurred, c, [=](QAbstractSocket::SocketError error) {
        qCWarning(V_WS) << "Console Proxy error:" << error << sock->errorString();
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection,
                                      sock->errorString());
    });
    connect(sock, &QTcpSocket::disconnected, c, [=] {
        qCWarning(V_WS) << "Console Proxy socket disconnected";
        c->response()->webSocketClose(Response::CloseCodeAbnormalDisconnection,
                                      sock->errorString());
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

    connect(c->request(), &Request::webSocketBinaryFrame, c, [=](const QByteArray &message) {
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

void Ws::createSshTunnel(QAbstractSocket *sock, const QUrl &url, quint16 port)
{
    qCDebug(V_WS) << "Create SSH tunnel to" << url.host() << "as" << url.userName();

    QStringList args;
    args << u"-TCv"_qs;
    args << u"-L"_qs << QStringLiteral("127.0.0.1:%1:127.0.0.1:%1").arg(port);
    args << u"-l"_qs << url.userName();
    args << url.host();

    auto process = new QProcess(sock);
    connect(sock, &QAbstractSocket::disconnected, process, [process] {
        if (process->state() == QProcess::Running) {
            process->terminate();
            qCDebug(V_WS) << "Shutdown ssh tunnel:" << process->waitForFinished();
        }
    });

    process->setProgram(u"ssh"_qs);
    process->setArguments(args);
    process->setReadChannel(QProcess::StandardError);
    process->start();

    if (process->waitForStarted()) {
        QByteArrayList outputVerbose;
        if (process->state() == QProcess::Running) {
            while (process->waitForReadyRead()) {
                const auto &output = process->readAllStandardError();
                if (output.contains("Local forwarding listening on")) {
                    qCDebug(V_WS) << "SSH tunnel established";
                    return;
                }
                outputVerbose << output;
            }
            qCWarning(V_WS) << "Cannot create ssh tunnel:" << outputVerbose.join('\n');
            process->terminate();
            return;
        }
    }

    qCWarning(V_WS) << "Cannot start ssh:" << process->readAllStandardError();
}
