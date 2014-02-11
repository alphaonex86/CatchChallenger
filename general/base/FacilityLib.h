#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QDir>
#include <QDateTime>
#include "GeneralStructures.h"
#include "CommonSettings.h"

namespace CatchChallenger {
class FacilityLib
{
public:
    static QByteArray toUTF8(const QString &text);
    static QStringList listFolder(const QString& folder,const QString& suffix=QStringLiteral(""));
    static QString randomPassword(const QString& string,const quint8& length);
    static QStringList skinIdList(const QString& skinPath);
    static QString secondsToString(const quint64 &seconds);
    static PublicPlayerMonster playerMonsterToPublicPlayerMonster(const PlayerMonster &playerMonster);
    static QByteArray playerMonsterToBinary(const PlayerMonster &playerMonster);
    static QByteArray publicPlayerMonsterToBinary(const PublicPlayerMonster &publicPlayerMonster);
    static PlayerMonster botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster, const Monster::Stat &stat);
    static bool rectTouch(QRect r1,QRect r2);
    static QString genderToString(const Gender &gender);
    static QString allowToString(const QSet<ActionAllow> &allowList);
    static QSet<ActionAllow> StringToAllow(const QString &string);
    static QDateTime nextCaptureTime(const City &city);
    static QByteArray privateMonsterToBinary(const PlayerMonster &monster);
    static bool rmpath(const QDir &dir);
    static IndustryStatus industryStatusWithCurrentTime(const IndustryStatus &industryStatus, const Industry &industry);
    static quint32 getFactoryResourcePrice(const quint32 &quantityInStock,const Industry::Resource &resource,const Industry &industry);
    static quint32 getFactoryProductPrice(const quint32 &quantityInStock,const Industry::Product &product,const Industry &industry);
    static IndustryStatus factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry);
    static bool factoryProductionStarted(const IndustryStatus &industryStatus,const Industry &industry);
    static QString timeToString(const quint32 &time);
private:
    static QByteArray UTF8EmptyData;
    static QString text_slash;
    static QString text_male;
    static QString text_female;
    static QString text_unknown;
    static QString text_clan;
    static QString text_dotcomma;
};
}

#endif
