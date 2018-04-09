#include "storagevol.h"

#include "virtlyst.h"

#include <QLoggingCategory>

StorageVol::StorageVol(virStorageVolPtr vol, QObject *parent) : QObject(parent)
  , m_vol(vol)
{

}

QString StorageVol::name()
{
    return QString::fromUtf8(virStorageVolGetName(m_vol));
}

QString StorageVol::type()
{
    return xmlDoc()
            .documentElement()
            .firstChildElement(QStringLiteral("target"))
            .firstChildElement(QStringLiteral("format"))
            .attribute(QStringLiteral("type"));
}

QString StorageVol::size()
{
    if (getInfo()) {
        return Virtlyst::prettyKibiBytes(m_info.capacity / 1024);
    }
    return QString();
}

bool StorageVol::getInfo()
{
    if (!m_gotInfo && virStorageVolGetInfo(m_vol, &m_info) == 0) {
        m_gotInfo = true;
    }
    return m_gotInfo;
}

QDomDocument StorageVol::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virStorageVolGetXMLDesc(m_vol, 0);
        const QString xmlString = QString::fromUtf8(xml);
        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from interface" << error;
        }
        free(xml);
    }
    return m_xml;
}
