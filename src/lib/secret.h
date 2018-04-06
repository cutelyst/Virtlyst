#ifndef SECRET_H
#define SECRET_H

#include <QObject>
#include <libvirt/libvirt.h>

class Connection;
class Secret : public QObject
{
    Q_OBJECT
public:
    explicit Secret(virSecretPtr secret, Connection *conn, QObject *parent = nullptr);

private:
    Connection *m_conn;
    virSecretPtr m_secret;
};

#endif // SECRET_H
