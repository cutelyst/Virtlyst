#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <QDomDocument>

#include <libvirt/libvirt.h>

class Connection;
class Domain : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString xml READ xml CONSTANT)
    Q_PROPERTY(QString uuid READ uuid CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
public:
    explicit Domain(virDomainPtr domain, Connection *parent = nullptr);

    bool load(bool secure = true);
    bool save();

    QString xml() const;

    QString uuid() const;

    QString title() const;
    void setTitle(const QString &title);

    QString description() const;
    void setDescription(const QString &description);

    int currentVcpu() const;
    void setCurrentVcpu(int number);

    int vcpu() const;
    void setVcpu(int number);

    quint64 memory() const;
    void setMemory(quint64 kBytes);

    quint64 currentMemory() const;
    void setCurrentMemory(quint64 kBytes);

private:
    QString dataFromSimpleNode(const QString &element) const;
    void setDataToSimpleNode(const QString &element, const QString &data);

    Connection *m_conn;
    virDomainPtr m_domain;
    QDomDocument m_xmlDoc;
};

#endif // DOMAIN_H
