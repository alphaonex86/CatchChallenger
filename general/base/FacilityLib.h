#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QDir>
#include <QDateTime>
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
    static QByteArray playerMonsterToBinary(const PlayerMonster &playerMonster);
    static QByteArray publicPlayerMonsterToBinary(const PublicPlayerMonster &publicPlayerMonster);
    static PlayerMonster botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster, const Monster::Stat &stat);
    static bool rectTouch(QRect r1,QRect r2);
    static QString genderToString(const Gender &gender);
    static QString allowToQString(const QSet<ActionAllow> &allowList);
    static QSet<ActionAllow> QStringToAllow(const QString &string);
    static QDateTime nextCaptureTime(const City &city);
    static QByteArray privateMonsterToBinary(const PlayerMonster &monster);
    static bool rmpath(const QDir &dir);
private:
    static QByteArray UTF8EmptyData;
};
}

#endif
