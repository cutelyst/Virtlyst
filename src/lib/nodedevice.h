#ifndef NODEDEVICE_H
#define NODEDEVICE_H

#include <QObject>
#include <QDomDocument>
#include <libvirt/libvirt.h>

class Connection;
class NodeDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString capabilityInterface READ capabilityInterface CONSTANT)
public:
    explicit NodeDevice(virNodeDevicePtr nodeDev, Connection *conn, QObject *parent = nullptr);

    QString capabilityInterface();

private:
    QDomDocument xml();

    QDomDocument m_xml;
    virNodeDevicePtr m_node;
    Connection *m_conn;
};

#endif // NODEDEVICE_H
