#include "interface.h"

Interface::Interface(virInterfacePtr iface, Connection *conn, QObject *parent) : QObject(parent)
  , m_iface(iface)
  , m_conn(conn)
{

}

Interface::~Interface()
{
    virInterfaceFree(m_iface);
}

QString Interface::name() const
{
    return QString::fromUtf8(virInterfaceGetName(m_iface));
}

QString Interface::type() const
{
    return QStringLiteral("ethernet");
}

QString Interface::mac() const
{
    return QString::fromUtf8(virInterfaceGetMACString(m_iface));

}
