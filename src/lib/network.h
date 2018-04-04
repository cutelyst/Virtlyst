#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <libvirt/libvirt.h>

class Connection;
class Network : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString bridgeName READ bridgeName CONSTANT)
public:
    explicit Network(virNetworkPtr net, Connection *conn, QObject *parent = nullptr);
    ~Network();

    QString name() const;
    QString bridgeName() const;

private:
    virNetworkPtr m_net;
    Connection *m_conn;
};

#endif // NETWORK_H
