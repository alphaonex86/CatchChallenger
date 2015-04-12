#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QDir>
#include <QDateTime>
#include "GeneralStructures.h"
#include "CommonSettingsCommon.h"

namespace CatchChallenger {
class FacilityLib
{
public:
    static PublicPlayerMonster playerMonsterToPublicPlayerMonster(const PlayerMonster &playerMonster);
    static QByteArray playerMonsterToBinary(const PlayerMonster &playerMonster);
    static QByteArray publicPlayerMonsterToBinary(const PublicPlayerMonster &publicPlayerMonster);
    static PlayerMonster botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster, const Monster::Stat &stat);
    static QString genderToString(const Gender &gender);
    static QDateTime nextCaptureTime(const City &city);
    static QByteArray privateMonsterToBinary(const PlayerMonster &monster);
    static IndustryStatus industryStatusWithCurrentTime(const IndustryStatus &industryStatus, const Industry &industry);
    static quint32 getFactoryResourcePrice(const quint32 &quantityInStock,const Industry::Resource &resource,const Industry &industry);
    static quint32 getFactoryProductPrice(const quint32 &quantityInStock,const Industry::Product &product,const Industry &industry);
    static IndustryStatus factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry);
    static bool factoryProductionStarted(const IndustryStatus &industryStatus,const Industry &industry);
    static void appendReputationPoint(PlayerReputation *playerReputation, const qint32 &point, const Reputation &reputation);
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
