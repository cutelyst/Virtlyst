#ifndef STORAGEVOL_H
#define STORAGEVOL_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class StoragePool;
class StorageVol : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString size READ size CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)
public:
    explicit StorageVol(virStorageVolPtr vol, virStoragePoolPtr pool, QObject *parent = nullptr);

    QString name();
    QString type();
    QString size();
    QString path();

    bool undefine();
    StorageVol *clone(const QString &name, const QString &format, int flags);

    StoragePool *pool();

private:
    bool getInfo();
    QDomDocument xmlDoc();
    virStoragePoolPtr poolPtr();

    QDomDocument m_xml;
    virStoragePoolPtr m_pool;
    virStorageVolInfo m_info;
    virStorageVolPtr m_vol;
    bool m_gotInfo = false;
};

#endif // STORAGEVOL_H
