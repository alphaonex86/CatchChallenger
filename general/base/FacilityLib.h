#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include "GeneralStructures.h"

namespace CatchChallenger {
class FacilityLib
{
public:
    static QByteArray toUTF8(const QString &text);
    static QStringList listFolder(const QString& folder,const QString& suffix="");
    static QString randomPassword(const QString& string,const quint8& length);
    static QStringList skinIdList(const QString& skinPath);
    static QString secondsToString(const quint64 &seconds);
    static PublicPlayerMonster playerMonsterToPublicPlayerMonster(const PlayerMonster &playerMonster);
    static QByteArray publicPlayerMonsterToBinary(const PublicPlayerMonster &publicPlayerMonster);
private:
    static QByteArray UTF8EmptyData;
};
}

#endif
