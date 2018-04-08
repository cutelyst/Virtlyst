#include "nodedevice.h"

#include <QLoggingCategory>

NodeDevice::NodeDevice(virNodeDevicePtr nodeDev, Connection *conn, QObject *parent) : QObject(parent)
  , m_node(nodeDev)
  , m_conn(conn)
{
}

QString NodeDevice::capabilityInterface()
{
    return xml()
            .documentElement()
            .firstChildElement(QStringLiteral("capability"))
            .firstChildElement(QStringLiteral("interface"))
            .firstChild()
            .nodeValue();
}

QDomDocument NodeDevice::xml()
{
    if (m_xml.isNull()) {
        char *xml = virNodeDeviceGetXMLDesc(m_node, 0);
        const QString xmlString = QString::fromUtf8(xml);
//        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from interface" << error;
        }
        free(xml);
    }
    return m_xml;
}
