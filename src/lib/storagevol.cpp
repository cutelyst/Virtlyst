#include "storagevol.h"

#include "virtlyst.h"

#include <QXmlStreamWriter>
#include <QLoggingCategory>

StorageVol::StorageVol(virStorageVolPtr vol, virStoragePoolPtr pool, QObject *parent) : QObject(parent)
  , m_vol(vol)
  , m_pool(pool)
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

void StorageVol::undefine()
{
    virStorageVolDelete(m_vol, 0);
}

bool StorageVol::clone(const QString &name, const QString &format, int flags)
{
    if (m_pool == nullptr) {
        return false;
    }

    QByteArray output;
    QXmlStreamWriter stream(&output);

    QString localFormat = format;
    if (localFormat.isEmpty()) {
        localFormat = type();
    }

    stream.writeStartElement(QStringLiteral("volume"));
    stream.writeTextElement(QStringLiteral("name"), name);
    stream.writeTextElement(QStringLiteral("capacity"), QStringLiteral("0"));
    stream.writeTextElement(QStringLiteral("allocation"), QStringLiteral("0"));

    stream.writeStartElement(QStringLiteral("target"));
    stream.writeStartElement(QStringLiteral("format"));
    stream.writeAttribute(QStringLiteral("type"), localFormat);
    stream.writeEndElement(); // format
    stream.writeEndElement(); // target

    stream.writeEndElement(); // volume
    qDebug() << "XML output" << output;

    virStorageVolPtr vol = virStorageVolCreateXMLFrom(m_pool, output.constData(), m_vol, flags);
    if (vol) {
        virStorageVolFree(vol);
        return true;
    }
    return false;
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
