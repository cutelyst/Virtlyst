#include "domain.h"
#include "connection.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(VIRT_DOM, "virt.domain")

Domain::Domain(virDomainPtr domain, Connection *parent) : QObject(parent)
  , m_conn(parent)
  , m_domain(domain)
{

}

bool Domain::load(bool secure)
{
    char *xml = virDomainGetXMLDesc(m_domain, secure ? VIR_DOMAIN_XML_SECURE : 0);
    const QString xmlString = QString::fromUtf8(xml);
    free(xml);

    QString errorString;
    if (!m_xmlDoc.setContent(xmlString, &errorString)) {
        qCCritical(VIRT_DOM) << "error" << m_xmlDoc.isNull() << m_xmlDoc.toString();
        return false;
    }
    //    qCCritical(VIRT_DOM) << "LOADED" << m_xmlDoc.isNull() << m_xmlDoc.toString();
    qCCritical(VIRT_DOM) << "LOADED" << memory() << currentMemory();

    return true;
}

bool Domain::save()
{
    qCCritical(VIRT_DOM) << m_xmlDoc.toString();
    return m_conn->domainDefineXml(m_xmlDoc.toString(0));
}

QString Domain::xml() const
{
    return m_xmlDoc.toString(2);
}

QString Domain::uuid() const
{
    return dataFromSimpleNode(QStringLiteral("uuid"));
}

QString Domain::title() const
{
    return dataFromSimpleNode(QStringLiteral("title"));
}

void Domain::setTitle(const QString &title)
{
    setDataToSimpleNode(QStringLiteral("title"), title);
}

QString Domain::description() const
{
    return dataFromSimpleNode(QStringLiteral("description"));
}

void Domain::setDescription(const QString &description)
{
    setDataToSimpleNode(QStringLiteral("description"), description);
}

int Domain::currentVcpu() const
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    if (node.hasAttribute(QStringLiteral("current"))) {
        return node.attribute(QStringLiteral("current")).toInt();
    }
    QDomNode data = node.firstChild();
    return data.nodeValue().toInt();
}

void Domain::setCurrentVcpu(int number)
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    node.setAttribute(QStringLiteral("current"), number);
}

int Domain::vcpu() const
{
    return dataFromSimpleNode(QStringLiteral("vcpu")).toULongLong();
}

void Domain::setVcpu(int number)
{
    setDataToSimpleNode(QStringLiteral("vcpu"), QString::number(number));
}

quint64 Domain::memory() const
{
    return dataFromSimpleNode(QStringLiteral("memory")).toULongLong();
}

void Domain::setMemory(quint64 kBytes)
{
    setDataToSimpleNode(QStringLiteral("memory"), QString::number(kBytes));
}

quint64 Domain::currentMemory() const
{
    return dataFromSimpleNode(QStringLiteral("currentMemory")).toULongLong();
}

void Domain::setCurrentMemory(quint64 kBytes)
{
    setDataToSimpleNode(QStringLiteral("currentMemory"), QString::number(kBytes));
}

QString Domain::dataFromSimpleNode(const QString &element) const
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(element);
    QDomNode data = node.firstChild();
    return data.nodeValue();
}

void Domain::setDataToSimpleNode(const QString &element, const QString &data)
{
    QDomElement docElem = m_xmlDoc.documentElement();
    QDomElement node = docElem.firstChildElement(element);
    QDomNode nodeData = node.firstChild();
    nodeData.setNodeValue(data);
}
