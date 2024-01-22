#ifndef WS_H
#define WS_H

#include <Cutelyst/Controller>

#include <QAbstractSocket>

using namespace Cutelyst;

class Virtlyst;
class Ws : public Controller
{
    Q_OBJECT
public:
    explicit Ws(Virtlyst *parent = nullptr);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c, const QString &hostId, const QString &uuid);

private Q_SLOTS:
    void End(Context *c) { Q_UNUSED(c); }

private:
    Virtlyst *m_virtlyst;

    void createSshTunnel(QAbstractSocket *sock, const QUrl &url, quint16 port);
};

#endif // WS_H
