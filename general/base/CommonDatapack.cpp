#include "CommonDatapack.h"
#include "GeneralVariable.h"
#include "FacilityLib.h"
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

CommonDatapack CommonDatapack::commonDatapack;

CommonDatapack::CommonDatapack()
{
    isParsed=false;
}

void CommonDatapack::parseDatapack(const std::string &datapackPath)
{
    if(isParsed)
        return;
    QMutexLocker mutexLocker(&inProgress);

    this->datapackPath=datapackPath;
    parseSkins();
    parseReputation();
    parseBuff();
    parseTypes();
    parseItems();
    parsePlants();
    parseCraftingRecipes();
    parseSkills();
    parseEvents();
    parseMonsters();
    parseMonstersCollision();
    parseMonstersEvolutionItems();
    parseMonstersItemToLearn();
    parseIndustries();
    parseProfileList();
    parseLayersOptions();

    isParsed=true;
}


void CommonDatapack::parseTypes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    types=FightLoader::loadTypes(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_MONSTERS)+std::stringLiteral("type.xml"));
    qDebug() << std::stringLiteral("%1 type(s) loaded").arg(types.size());
    #endif
}

void CommonDatapack::parseSkins()
{
    skins=DatapackGeneralLoader::loadSkins(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_SKIN));
    qDebug() << std::stringLiteral("%1 skin(s) loaded").arg(skins.size());
}

void CommonDatapack::parseItems()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    items=DatapackGeneralLoader::loadItems(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_ITEM),monsterBuffs);
    qDebug() << std::stringLiteral("%1 items(s) loaded").arg(items.item.size());
    qDebug() << std::stringLiteral("%1 trap(s) loaded").arg(items.trap.size());
    #endif
}

void CommonDatapack::parseIndustries()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    industries=DatapackGeneralLoader::loadIndustries(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_INDUSTRIESBASE),items.item);
    industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_INDUSTRIESBASE)+std::stringLiteral("list.xml"),industries);
    qDebug() << std::stringLiteral("%1 industries loaded").arg(industries.size());
    qDebug() << std::stringLiteral("%1 industries link loaded").arg(industriesLink.size());
    #endif
}

void CommonDatapack::parseCraftingRecipes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_CRAFTING)+std::stringLiteral("recipes.xml"),items.item);
    crafingRecipes=multipleVariables.first;
    itemToCrafingRecipes=multipleVariables.second;
    qDebug() << std::stringLiteral("%1 crafting recipe(s) loaded").arg(crafingRecipes.size());
    #endif
}

void CommonDatapack::parsePlants()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    plants=DatapackGeneralLoader::loadPlants(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_PLANTS)+std::stringLiteral("plants.xml"));
    qDebug() << std::stringLiteral("%1 plant(s) loaded").arg(plants.size());
    #endif
}

void CommonDatapack::parseReputation()
{
    reputation=DatapackGeneralLoader::loadReputation(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+std::stringLiteral("reputation.xml"));
    qDebug() << std::stringLiteral("%1 reputation(s) loaded").arg(reputation.size());
}

void CommonDatapack::parseBuff()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    monsterBuffs=FightLoader::loadMonsterBuff(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_BUFF));
    qDebug() << std::stringLiteral("%1 monster buff(s) loaded").arg(monsterBuffs.size());
    #endif
}

void CommonDatapack::parseSkills()
{
    monsterSkills=FightLoader::loadMonsterSkill(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_SKILL)
                                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                ,monsterBuffs,types
                                                #endif // CATCHCHALLENGER_CLASS_MASTER
                                                );
    qDebug() << std::stringLiteral("%1 monster skill(s) loaded").arg(monsterSkills.size());
}

void CommonDatapack::parseEvents()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    events=DatapackGeneralLoader::loadEvents(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+std::stringLiteral("event.xml"));
    qDebug() << std::stringLiteral("%1 event(s) loaded").arg(events.size());
    #endif
}

void CommonDatapack::parseMonsters()
{
    monsters=FightLoader::loadMonster(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_MONSTERS),monsterSkills
                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                      ,types,items.item
                                      #endif
                                      );
    qDebug() << std::stringLiteral("%1 monster(s) loaded").arg(monsters.size());
}

void CommonDatapack::parseMonstersCollision()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollision=DatapackGeneralLoader::loadMonstersCollision(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_MAPBASE)+std::stringLiteral("layers.xml"),CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.events);
    qDebug() << std::stringLiteral("%1 monster(s) collisions loaded").arg(monstersCollision.size());
    #endif
}

void CommonDatapack::parseMonstersEvolutionItems()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.evolutionItem=FightLoader::loadMonsterEvolutionItems(monsters);
    qDebug() << std::stringLiteral("%1 monster evolution items(s) loaded").arg(items.evolutionItem.size());
    #endif
}

void CommonDatapack::parseMonstersItemToLearn()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.itemToLearn=FightLoader::loadMonsterItemToLearn(monsters,items.evolutionItem);
    qDebug() << std::stringLiteral("%1 monster items(s) to learn loaded").arg(items.itemToLearn.size());
    #endif
}

void CommonDatapack::parseProfileList()
{
    profileList=DatapackGeneralLoader::loadProfileList(datapackPath,datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+std::stringLiteral("start.xml")
                                                       #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                       ,items.item
                                                       #endif // CATCHCHALLENGER_CLASS_MASTER
                                                       ,monsters,reputation).second;
    qDebug() << std::stringLiteral("%1 profile(s) loaded").arg(profileList.size());
}

void CommonDatapack::parseLayersOptions()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    layersOptions=DatapackGeneralLoader::loadLayersOptions(datapackPath+std::stringLiteral(DATAPACK_BASE_PATH_MAPBASE)+std::stringLiteral("layers.xml"));
    qDebug() << std::stringLiteral("layers options parsed");
    #endif
}

void CommonDatapack::unload()
{
    QMutexLocker mutexLocker(&inProgress);
    if(!isParsed)
        return;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    plants.clear();
    crafingRecipes.clear();
    events.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    reputation.clear();
    monsters.clear();
    monsterSkills.clear();
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monsterBuffs.clear();
    itemToCrafingRecipes.clear();
    items.item.clear();
    items.trap.clear();
    items.monsterItemEffect.clear();
    items.evolutionItem.clear();
    items.repel.clear();
    industries.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    profileList.clear();
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollision.clear();
    types.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    #ifndef EPOLLCATCHCHALLENGERSERVER
    xmlLoadedFile.clear();
    #endif
    skins.clear();
    isParsed=false;
}
