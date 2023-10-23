#include "Ultimate.hpp"

#include <QCryptographicHash>
#include <QString>
#include <QObject>

Ultimate Ultimate::ultimate;

Ultimate::Ultimate() :
    m_ultimate(false)
{
}

bool Ultimate::setKey(const std::string &key)
{
    if(m_ultimate)
        return true;
    QCryptographicHash hash(QCryptographicHash::Sha224);
    hash.addData(key.data(),key.size());
    const QByteArray &result=hash.result();
    if(!result.isEmpty() && result.at(0)==0x00 && result.at(1)==0x00)
        m_ultimate=true;
    return m_ultimate;
}

bool Ultimate::isUltimate() const
{
    return m_ultimate;
}

std::string Ultimate::buy()
{
    return QObject::tr("https://catchchallenger.herman-brule.com/#buy").toStdString();
}
