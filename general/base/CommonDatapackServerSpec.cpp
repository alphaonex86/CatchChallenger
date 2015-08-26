#include "CommonDatapackServerSpec.h"
#include "CommonDatapack.h"
#include "GeneralVariable.h"
#include "FacilityLib.h"
#include "Map_loader.h"
#include "../fight/FightLoader.h"
#include "DatapackGeneralLoader.h"

#include <QDebug>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QMutexLocker>

using namespace CatchChallenger;

CommonDatapackServerSpec CommonDatapackServerSpec::commonDatapackServerSpec;

CommonDatapackServerSpec::CommonDatapackServerSpec()
{
}

void CommonDatapackServerSpec::parseDatapack(const std::string &datapackPath,const std::string &mainDatapackCode)
{
    if(isParsedSpec)
        return;
    QMutexLocker mutexLocker(&inProgressSpec);

    this->datapackPath=datapackPath;
    this->mainDatapackCode=mainDatapackCode;

    CommonDatapack::commonDatapack.parseDatapack(datapackPath);

    parseQuests();
    parseBotFights();
    parseShop();
    parseServerProfileList();
    parseIndustries();

    #ifdef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif

    isParsedSpec=true;
}

void CommonDatapackServerSpec::parseQuests()
{
    quests=DatapackGeneralLoader::loadQuests(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_QUESTS).arg(mainDatapackCode));
    qDebug() << std::stringLiteral("%1 quest(s) loaded").arg(quests.size());
}

void CommonDatapackServerSpec::parseShop()
{
    shops=DatapackGeneralLoader::preload_shop(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_SHOP).arg(mainDatapackCode)+std::stringLiteral("shop.xml"),CommonDatapack::commonDatapack.items.item);
    qDebug() << std::stringLiteral("%1 monster items(s) to learn loaded").arg(shops.size());
}

void CommonDatapackServerSpec::parseBotFights()
{
    botFights=FightLoader::loadFight(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_FIGHT).arg(mainDatapackCode),CommonDatapack::commonDatapack.monsters,CommonDatapack::commonDatapack.monsterSkills,CommonDatapack::commonDatapack.items.item);
    qDebug() << std::stringLiteral("%1 bot fight(s) loaded").arg(botFights.size());
}

void CommonDatapackServerSpec::parseServerProfileList()
{
    serverProfileList=DatapackGeneralLoader::loadServerProfileList(datapackPath,mainDatapackCode,datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_PLAYERSPEC).arg(mainDatapackCode)+std::stringLiteral("start.xml"),CommonDatapack::commonDatapack.profileList);
    qDebug() << std::stringLiteral("%1 server profile(s) loaded").arg(serverProfileList.size());
}

void CommonDatapackServerSpec::parseIndustries()
{
    std::unordered_map<uint16_t,Industry> industriesBase=CommonDatapack::commonDatapack.industries;
    std::unordered_map<uint16_t,IndustryLink> industriesLinkBase=CommonDatapack::commonDatapack.industriesLink;
    CommonDatapack::commonDatapack.industries=DatapackGeneralLoader::loadIndustries(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_INDUSTRIESSPEC).arg(mainDatapackCode),CommonDatapack::commonDatapack.items.item);
    qDebug() << std::stringLiteral("%1 industries loaded (spec industries %2)").arg(industriesBase.size()).arg(CommonDatapack::commonDatapack.industries.size());
    {
        std::unordered_mapIterator<uint16_t,Industry> i(industriesBase);
        while (i.hasNext()) {
            i.next();
            if(!CommonDatapack::commonDatapack.industries.contains(i.key()))
                CommonDatapack::commonDatapack.industries[i.key()]=i.value();
        }
    }
    CommonDatapack::commonDatapack.industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_INDUSTRIESSPEC).arg(mainDatapackCode)+std::stringLiteral("list.xml"),CommonDatapack::commonDatapack.industries);
    qDebug() << std::stringLiteral("%1 industries link loaded (spec industries link %2)").arg(industriesLinkBase.size()).arg(CommonDatapack::commonDatapack.industriesLink.size());
    {
        std::unordered_mapIterator<uint16_t,IndustryLink> i(industriesLinkBase);
        while (i.hasNext()) {
            i.next();
            if(!CommonDatapack::commonDatapack.industriesLink.contains(i.key()))
                CommonDatapack::commonDatapack.industriesLink[i.key()]=i.value();
        }
    }
}

void CommonDatapackServerSpec::unload()
{
    QMutexLocker mutexLocker(&inProgressSpec);
    if(!isParsedSpec)
        return;
    botFights.clear();
    quests.clear();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    shops.clear();
    serverProfileList.clear();
    CommonDatapack::commonDatapack.unload();
    isParsedSpec=false;
}
