#ifndef STORAGEVOL_H
#define STORAGEVOL_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class StorageVol : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString size READ size CONSTANT)
public:
    explicit StorageVol(virStorageVolPtr vol, QObject *parent = nullptr);

    QString name();
    QString type();
    QString size();

private:
    bool getInfo();
    QDomDocument xmlDoc();

    QDomDocument m_xml;
    virStorageVolInfo m_info;
    virStorageVolPtr m_vol;
    bool m_gotInfo = false;
};

#endif // STORAGEVOL_H
