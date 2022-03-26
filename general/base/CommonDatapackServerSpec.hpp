#ifndef COMMONDATAPACK_ServerSpec_H
#define COMMONDATAPACK_ServerSpec_H

#include <unordered_map>
#include <string>

#include "GeneralStructures.hpp"
#include "CommonDatapack.hpp"
#include "lib.h"

namespace CatchChallenger {
class DLL_PUBLIC CommonDatapackServerSpec
{
public:
    explicit CommonDatapackServerSpec();
public:
    void unload();
    void parseDatapack(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &subDatapackCode);
    bool isParsedContent() const;
    static CommonDatapackServerSpec commonDatapackServerSpec;

    const std::unordered_map<uint16_t,BotFight> &get_botFights() const;
    const uint16_t &get_botFightsMaxId() const;
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST,Quest> &get_quests() const;
    const std::unordered_map<SHOP_TYPE,Shop> &get_shops() const;
    const std::vector<ServerSpecProfile> &get_serverProfileList() const;
    std::vector<ServerSpecProfile> &get_serverProfileList_rw();
    const std::unordered_map<uint16_t,std::vector<MonsterDrops> > &get_monsterDrops() const;
    const std::unordered_map<std::string,ZONE_TYPE> &get_zoneToId() const;
    std::unordered_map<std::string,ZONE_TYPE> &get_zoneToId_rw();
    const std::vector<std::string> &get_idToZone() const;
    std::vector<std::string> &get_idToZone_rw();
protected:
    std::unordered_map<uint16_t,BotFight> botFights;
    uint16_t botFightsMaxId;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST,Quest> quests;
    std::unordered_map<SHOP_TYPE,Shop> shops;/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    std::vector<ServerSpecProfile> serverProfileList;
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > monsterDrops;//to prevent send network packet for item when luck is 100%
    std::unordered_map<std::string,ZONE_TYPE> zoneToId;//tempory var to load zone
    std::vector<std::string> idToZone;//to write to db: GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_city.asyncWrite({clan->capturedCity});
public:
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        buf << botFights;
        buf << botFightsMaxId;
        buf << quests;
        buf << shops;
        buf << serverProfileList;
        buf << monsterDrops;
        buf << idToZone;
    }
    template <class B>
    void parse(B& buf) {
        buf >> botFights;
        buf >> botFightsMaxId;
        buf >> quests;
        buf >> shops;
        buf >> serverProfileList;
        buf >> monsterDrops;
        buf >> idToZone;
    }
    #endif
private:
    bool isParsedSpec;
    bool parsingSpec;
    std::string datapackPath;
    std::string mainDatapackCode;
    std::string subDatapackCode;
private:
    void parseQuests();
    void parseBotFights();//gold/item variables by server
    void parseShop();
    void parseServerProfileList();
    void parseIndustries();
    #ifdef CATCHCHALLENGER_CLIENT
    void applyMonstersRate();//xp,sp variable by server, only have this second pass on client, take care
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    void parseMonstersDrop();//drop rate variable by server
    #endif
};
}


#endif // COMMONDATAPACK_ServerSpec_H
