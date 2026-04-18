#include "Ultimate.hpp"

#include "../../general/base/CatchChallenger_Hash.hpp"
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
    CatchChallenger::Hash hash;
    hash.update(reinterpret_cast<const unsigned char *>(key.data()),key.size());
    unsigned char result[CATCHCHALLENGER_HASH_SIZE];
    hash.final(result);
    if(result[0]==0x00 && result[1]==0x00)
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
