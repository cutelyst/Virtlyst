#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <libvirt/libvirt.h>

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(const QString &uri, QObject *parent = nullptr);
    ~Connection();

    QString hostname() const;
    quint64 freeMemoryBytes() const;
    int maxVcpus() const;

    bool domainDefineXml(const QString &xml);

    virConnectPtr raw() const;

private:
    virConnectPtr m_conn;
    QString m_uri;
};

#endif // CONNECTION_H
