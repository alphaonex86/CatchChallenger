#include "CommonDatapackServerSpec.h"
#include "CommonDatapack.h"
#include "GeneralVariable.h"
#include "FacilityLib.h"
#include "Map_loader.h"
#include "../fight/FightLoader.h"
#include "DatapackGeneralLoader.h"

#include <iostream>
#include <vector>

using namespace CatchChallenger;

CommonDatapackServerSpec CommonDatapackServerSpec::commonDatapackServerSpec;

CommonDatapackServerSpec::CommonDatapackServerSpec()
{
    isParsedSpec=false;
    parsingSpec=false;
    botFightsMaxId=0;
}

void CommonDatapackServerSpec::parseDatapack(const std::string &datapackPath,const std::string &mainDatapackCode)
{
    if(isParsedSpec)
        return;
    if(parsingSpec)
        return;
    parsingSpec=true;

    this->datapackPath=datapackPath;
    this->mainDatapackCode=mainDatapackCode;

    #ifndef BOTTESTCONNECT
    CommonDatapack::commonDatapack.parseDatapack(datapackPath);

    parseQuests();
    parseBotFights();
    parseShop();
    parseServerProfileList();
    parseIndustries();
    #endif

    #ifdef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif

    parsingSpec=false;
    isParsedSpec=true;
}

bool CommonDatapackServerSpec::isParsedContent() const
{
    return isParsedSpec;
}

void CommonDatapackServerSpec::parseQuests()
{
    quests=DatapackGeneralLoader::loadQuests(datapackPath+DATAPACK_BASE_PATH_QUESTS1+mainDatapackCode+DATAPACK_BASE_PATH_QUESTS2);
    std::cout << quests.size() << " quest(s) loaded" << std::endl;
}

void CommonDatapackServerSpec::parseShop()
{
    shops=DatapackGeneralLoader::preload_shop(datapackPath+DATAPACK_BASE_PATH_SHOP1+mainDatapackCode+DATAPACK_BASE_PATH_SHOP2+"shop.xml",CommonDatapack::commonDatapack.items.item);
    std::cout << shops.size() << " monster items(s) to learn loaded" << std::endl;
}

void CommonDatapackServerSpec::parseBotFights()
{
    botFightsMaxId=0;
    botFights=FightLoader::loadFight(datapackPath+DATAPACK_BASE_PATH_FIGHT1+mainDatapackCode+DATAPACK_BASE_PATH_FIGHT2,CommonDatapack::commonDatapack.monsters,CommonDatapack::commonDatapack.monsterSkills,CommonDatapack::commonDatapack.items.item,botFightsMaxId);
    std::cout << botFights.size() << " bot fight(s) loaded" << std::endl;
}

void CommonDatapackServerSpec::parseServerProfileList()
{
    serverProfileList=DatapackGeneralLoader::loadServerProfileList(datapackPath,mainDatapackCode,datapackPath+DATAPACK_BASE_PATH_PLAYERSPEC+"/"+mainDatapackCode+"/start.xml",CommonDatapack::commonDatapack.profileList);
    std::cout << serverProfileList.size() << " server profile(s) loaded" << std::endl;
}

void CommonDatapackServerSpec::parseIndustries()
{
    std::unordered_map<uint16_t,Industry> industriesBase=CommonDatapack::commonDatapack.industries;
    std::unordered_map<uint16_t,IndustryLink> industriesLinkBase=CommonDatapack::commonDatapack.industriesLink;
    CommonDatapack::commonDatapack.industries=DatapackGeneralLoader::loadIndustries(datapackPath+DATAPACK_BASE_PATH_INDUSTRIESSPEC1+mainDatapackCode+DATAPACK_BASE_PATH_INDUSTRIESSPEC2,CommonDatapack::commonDatapack.items.item);
    std::cout << industriesBase.size() << " industries loaded (spec industries " << CommonDatapack::commonDatapack.industries.size() << ")" << std::endl;
    {
        auto i=industriesBase.begin();
        while(i!=industriesBase.cend())
        {
            if(CommonDatapack::commonDatapack.industries.find(i->first)==CommonDatapack::commonDatapack.industries.cend())
                CommonDatapack::commonDatapack.industries[i->first]=i->second;
            ++i;
        }
    }
    CommonDatapack::commonDatapack.industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+DATAPACK_BASE_PATH_INDUSTRIESSPEC1+mainDatapackCode+DATAPACK_BASE_PATH_INDUSTRIESSPEC2+"list.xml",CommonDatapack::commonDatapack.industries);
    std::cout << industriesLinkBase.size() << " industries link loaded (spec industries link " << CommonDatapack::commonDatapack.industriesLink.size() << ")" << std::endl;
    {
        auto i=industriesLinkBase.begin();
        while(i!=industriesLinkBase.cend())
        {
            if(CommonDatapack::commonDatapack.industriesLink.find(i->first)==CommonDatapack::commonDatapack.industriesLink.cend())
                CommonDatapack::commonDatapack.industriesLink[i->first]=i->second;
            ++i;
        }
    }
}

void CommonDatapackServerSpec::unload()
{
    if(!isParsedSpec)
        return;
    if(parsingSpec)
        return;
    parsingSpec=true;
    botFights.clear();
    quests.clear();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    shops.clear();
    serverProfileList.clear();
    CommonDatapack::commonDatapack.unload();

    parsingSpec=false;
    isParsedSpec=false;
}
