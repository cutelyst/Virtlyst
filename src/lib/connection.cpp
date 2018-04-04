#include "connection.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(VIRT_CONN, "virt.connection")

Connection::Connection(const QString &name, QObject *parent) : QObject(parent)
  , m_uri(name)
{
    m_conn = virConnectOpen(name.toUtf8().constData());
    if (m_conn == NULL) {
        qCWarning(VIRT_CONN) << "Failed to open connection to" << name;
        return;
    }
}

Connection::~Connection()
{
    virConnectClose(m_conn);
}

QString Connection::hostname() const
{
    QString ret;
    if (m_conn) {
        char *host = virConnectGetHostname(m_conn);
        ret = QString::fromUtf8(host);
        free(host);
    }
    return ret;
}

quint64 Connection::freeMemoryBytes() const
{
    if (m_conn) {
        return virNodeGetFreeMemory(m_conn);
    }
    return 0;
}

int Connection::maxVcpus() const
{
    return virConnectGetMaxVcpus(m_conn, NULL);
}

bool Connection::domainDefineXml(const QString &xml)
{
    virDomainPtr dom = virDomainDefineXML(m_conn, xml.toUtf8().constData());
    virDomainFree(dom);
    return dom != NULL;
}

virConnectPtr Connection::raw() const
{
    return m_conn;
}
