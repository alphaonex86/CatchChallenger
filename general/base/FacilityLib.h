#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <vector>
#include <string>
#include "GeneralStructures.h"

namespace CatchChallenger {
class FacilityLib
{
public:
    static PublicPlayerMonster playerMonsterToPublicPlayerMonster(const PlayerMonster &playerMonster);
    static uint16_t playerMonsterToBinary(char *data,const PlayerMonster &playerMonster);
    static uint16_t publicPlayerMonsterToBinary(char *data,const PublicPlayerMonster &publicPlayerMonster);
    static PlayerMonster botFightMonsterToPlayerMonster(const BotFight::BotFightMonster &botFightMonster, const Monster::Stat &stat);
    static std::string genderToString(const Gender &gender);
    static uint64_t nextCaptureTime(const City &city);
    static uint32_t privateMonsterToBinary(char *data, const PlayerMonster &monster);
    static IndustryStatus industryStatusWithCurrentTime(const IndustryStatus &industryStatus, const Industry &industry);
    static uint32_t getFactoryResourcePrice(const uint32_t &quantityInStock,const Industry::Resource &resource,const Industry &industry);
    static uint32_t getFactoryProductPrice(const uint32_t &quantityInStock,const Industry::Product &product,const Industry &industry);
    static IndustryStatus factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry);
    static bool factoryProductionStarted(const IndustryStatus &industryStatus,const Industry &industry);
    static void appendReputationPoint(PlayerReputation *playerReputation, const int32_t &point, const Reputation &reputation);
private:
    static std::vector<char> UTF8EmptyData;
    static std::string text_slash;
    static std::string text_male;
    static std::string text_female;
    static std::string text_unknown;
    static std::string text_clan;
    static std::string text_dotcomma;
};
}

#endif
