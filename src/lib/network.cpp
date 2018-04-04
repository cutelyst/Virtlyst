#include "network.h"

Network::Network(virNetworkPtr net, Connection *conn, QObject *parent) : QObject(parent)
  , m_net(net)
  , m_conn(conn)
{

}

Network::~Network()
{
    virNetworkFree(m_net);
}

QString Network::name() const
{
    return QString::fromUtf8(virNetworkGetName(m_net));
}

QString Network::bridgeName() const
{
    char *name = virNetworkGetBridgeName(m_net);
    const QString ret = QString::fromUtf8(name);
    return ret;
}
