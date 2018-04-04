#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <libvirt/libvirt.h>

class Connection;
class Interface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString mac READ mac CONSTANT)
public:
    explicit Interface(virInterfacePtr iface, Connection *conn, QObject *parent = nullptr);
    ~Interface();

    QString name() const;
    QString type() const;
    QString mac() const;

private:
    virInterfacePtr m_iface;
    Connection *m_conn;
};

#endif // INTERFACE_H
