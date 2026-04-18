#include "CommonDatapackServerSpec.hpp"
#include "CommonDatapack.hpp"
#ifdef CATCHCHALLENGER_CLIENT
#include "CommonSettingsServer.hpp"
#endif
#include "GeneralVariable.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "Map_loader.hpp"
#include "../fight/FightLoader.hpp"
#include "DatapackGeneralLoader/DatapackGeneralLoader.hpp"
#endif
#include "../../general/base/FacilityLibGeneral.hpp"

#include <iostream>
#include <vector>
#include <cmath>

using namespace CatchChallenger;

CommonDatapackServerSpec CommonDatapackServerSpec::commonDatapackServerSpec;

CommonDatapackServerSpec::CommonDatapackServerSpec()
{
    isParsedSpec=false;
    parsingSpec=false;
}

#ifndef CATCHCHALLENGER_NOXML
void CommonDatapackServerSpec::parseDatapackAfterZoneAndMap(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &subDatapackCode, const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    if(!CommonDatapack::commonDatapack.isParsedContent())
    {
        std::cerr << "firstly you need call CommonDatapack::commonDatapack.parseDatapack(), load the map before call CommonDatapackServerSpec::parseDatapack()" << std::endl;
        return;
    }
    if(isParsedSpec)
        return;
    if(parsingSpec)
        return;
    parsingSpec=true;

    this->datapackPath=datapackPath;
    this->mainDatapackCode=mainDatapackCode;
    this->subDatapackCode=subDatapackCode;

    #ifndef BOTTESTCONNECT
    parseQuests(mapPathToId);
    parseServerProfileList(mapPathToId);
    #ifdef CATCHCHALLENGER_CLIENT
    applyMonstersRate();
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    parseMonstersDrop();
    #endif
    #endif

    #ifdef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif

    parsingSpec=false;
    isParsedSpec=true;
}
#endif

bool CommonDatapackServerSpec::isParsedContent() const
{
    return isParsedSpec;
}

#ifndef CATCHCHALLENGER_NOXML
void CommonDatapackServerSpec::parseQuests(const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    quests=DatapackGeneralLoader::loadQuests(datapackPath+DATAPACK_BASE_PATH_QUESTS1+mainDatapackCode+DATAPACK_BASE_PATH_QUESTS2,mapPathToId);
    std::cout << quests.size() << " quest(s) loaded" << std::endl;
}

void CommonDatapackServerSpec::parseServerProfileList(const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    std::string startFile=datapackPath+DATAPACK_BASE_PATH_PLAYERSPEC+"/"+mainDatapackCode+"/sub/"+subDatapackCode+"/start.xml";
    if(!CatchChallenger::FacilityLibGeneral::isFile(startFile))
        startFile=datapackPath+DATAPACK_BASE_PATH_PLAYERSPEC+"/"+mainDatapackCode+"/start.xml";
    serverProfileList=DatapackGeneralLoader::loadServerProfileList(datapackPath,mainDatapackCode,startFile,CommonDatapack::commonDatapack.get_profileList(),mapPathToId);
    std::cout << serverProfileList.size() << " server profile(s) loaded" << std::endl;
}
#endif

#ifdef CATCHCHALLENGER_CLIENT
void CommonDatapackServerSpec::applyMonstersRate()
{
    if(CommonDatapack::commonDatapack.get_monsterRateApplied())
        return;
    CommonDatapack::commonDatapack.set_monsterRateApplied(false);
    if(CommonSettingsServer::commonSettingsServer.rates_xp<=0)
    {
        std::cerr << "CommonSettingsServer::commonSettingsServer.rates_xp can't be null, are you connected to game server to have the rates?" << std::endl;
        abort();
    }
    for(const auto& n : CommonDatapack::commonDatapack.monsters) {
        CatchChallenger::Monster &monster=CommonDatapack::commonDatapack.monsters.at(n.first);

        /*prevent double call, double call on client, not solved for now:
         * Client open to lan 1 call, client connect on this server: 2 call */
        if(!monster.level_to_xp.empty())
        {
            std::cerr << "CommonDatapackServerSpec::applyMonstersRate() drop dual call" << std::endl;
            return;
        }

        const uint32_t &oldXP=monster.give_xp;
        monster.give_xp*=CommonSettingsServer::commonSettingsServer.rates_xp;
        //monster.give_xp/=1000;//why?
        if(monster.give_xp==0)
        {
            std::cerr << "CommonDatapackServerSpec::applyMonstersRate() monster.give_xp==0: " << oldXP << "*" << CommonSettingsServer::commonSettingsServer.rates_xp << "/1000" << std::endl;
            abort();
        }
        monster.powerVar*=static_cast<double>(CommonSettingsServer::commonSettingsServer.rates_xp_pow)/1000;
        monster.level_to_xp.clear();
        int index=0;
        while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            uint64_t xp_for_this_level=std::pow(index+1,monster.powerVar);
            uint64_t xp_for_max_level=monster.xp_for_max_level;
            uint64_t max_xp=std::pow(CATCHCHALLENGER_MONSTER_LEVEL_MAX,monster.powerVar);
            uint64_t tempXp=xp_for_this_level*xp_for_max_level/max_xp;
            if(tempXp<1)
                tempXp=1;
            monster.level_to_xp.push_back(static_cast<uint32_t>(tempXp));
            index++;
        }
    }
}
#endif

#ifndef CATCHCHALLENGER_NOXML
#ifndef CATCHCHALLENGER_CLASS_MASTER
void CommonDatapackServerSpec::parseMonstersDrop()
{
    monsterDrops=DatapackGeneralLoader::loadMonsterDrop(datapackPath+DATAPACK_BASE_PATH_MONSTERS,
                                                       CommonDatapack::commonDatapack.items,
                                                       CommonDatapack::commonDatapack.monsters);
    std::cout << monsterDrops.size() << " monters drop(s) loaded" << std::endl;
}
#endif
#endif

void CommonDatapackServerSpec::unload()
{
    if(!isParsedSpec)
        return;
    if(parsingSpec)
        return;
    parsingSpec=true;
    quests.clear();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    #ifndef CATCHCHALLENGER_NOXML
    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monsterDrops.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    serverProfileList.clear();
    CommonDatapack::commonDatapack.unload();

    for(const auto& n : CommonDatapack::commonDatapack.monsters) {
        CatchChallenger::Monster &monster=CommonDatapack::commonDatapack.monsters.at(n.first);
        monster.level_to_xp.clear();
    }

    parsingSpec=false;
    isParsedSpec=false;
}

//quests
size_t CommonDatapackServerSpec::get_quests_size() const { return quests.size(); }
const Quest &CommonDatapackServerSpec::get_quest(const CATCHCHALLENGER_TYPE_QUEST &key) const { return quests.at(key); }
bool CommonDatapackServerSpec::has_quest(const CATCHCHALLENGER_TYPE_QUEST &key) const { return quests.find(key)!=quests.cend(); }
//serverProfileList
const std::vector<ServerSpecProfile> &CommonDatapackServerSpec::get_serverProfileList() const { return serverProfileList; }
std::vector<ServerSpecProfile> &CommonDatapackServerSpec::get_serverProfileList_rw() { return serverProfileList; }
//monsterDrops
size_t CommonDatapackServerSpec::get_monsterDrops_size() const { return monsterDrops.size(); }
const std::vector<MonsterDrops> &CommonDatapackServerSpec::get_monsterDrop(const CATCHCHALLENGER_TYPE_MAPID &key) const { return monsterDrops.at(key); }
bool CommonDatapackServerSpec::has_monsterDrop(const CATCHCHALLENGER_TYPE_MAPID &key) const { return monsterDrops.find(key)!=monsterDrops.cend(); }
//zoneToId
size_t CommonDatapackServerSpec::get_zoneToId_size() const { return zoneToId.size(); }
ZONE_TYPE CommonDatapackServerSpec::get_zoneToId(const std::string &zone) const { return zoneToId.at(zone); }
bool CommonDatapackServerSpec::has_zoneToId(const std::string &zone) const { return zoneToId.find(zone)!=zoneToId.cend(); }
void CommonDatapackServerSpec::set_zoneToId(const std::string &zone, const ZONE_TYPE &id) { zoneToId[zone]=id; }
//idToZone
const std::vector<std::string> &CommonDatapackServerSpec::get_idToZone() const { return idToZone; }
std::vector<std::string> &CommonDatapackServerSpec::get_idToZone_rw() { return idToZone; }

