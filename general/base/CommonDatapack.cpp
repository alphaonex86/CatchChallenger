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
    types=FightLoader::loadTypes(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("type.xml"));
    qDebug() << QStringLiteral("%1 type(s) loaded").arg(types.size());
    #endif
}

void CommonDatapack::parseSkins()
{
    skins=DatapackGeneralLoader::loadSkins(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKIN));
    qDebug() << QStringLiteral("%1 skin(s) loaded").arg(skins.size());
}

void CommonDatapack::parseItems()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    items=DatapackGeneralLoader::loadItems(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM),monsterBuffs);
    qDebug() << QStringLiteral("%1 items(s) loaded").arg(items.item.size());
    qDebug() << QStringLiteral("%1 trap(s) loaded").arg(items.trap.size());
    #endif
}

void CommonDatapack::parseIndustries()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    industries=DatapackGeneralLoader::loadIndustries(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_INDUSTRIESBASE),items.item);
    industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_INDUSTRIESBASE)+QStringLiteral("list.xml"),industries);
    qDebug() << QStringLiteral("%1 industries loaded").arg(industries.size());
    qDebug() << QStringLiteral("%1 industries link loaded").arg(industriesLink.size());
    #endif
}

void CommonDatapack::parseCraftingRecipes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_CRAFTING)+QStringLiteral("recipes.xml"),items.item);
    crafingRecipes=multipleVariables.first;
    itemToCrafingRecipes=multipleVariables.second;
    qDebug() << QStringLiteral("%1 crafting recipe(s) loaded").arg(crafingRecipes.size());
    #endif
}

void CommonDatapack::parsePlants()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    plants=DatapackGeneralLoader::loadPlants(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLANTS)+QStringLiteral("plants.xml"));
    qDebug() << QStringLiteral("%1 plant(s) loaded").arg(plants.size());
    #endif
}

void CommonDatapack::parseReputation()
{
    reputation=DatapackGeneralLoader::loadReputation(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+QStringLiteral("reputation.xml"));
    qDebug() << QStringLiteral("%1 reputation(s) loaded").arg(reputation.size());
}

void CommonDatapack::parseBuff()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    monsterBuffs=FightLoader::loadMonsterBuff(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_BUFF));
    qDebug() << QStringLiteral("%1 monster buff(s) loaded").arg(monsterBuffs.size());
    #endif
}

void CommonDatapack::parseSkills()
{
    monsterSkills=FightLoader::loadMonsterSkill(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKILL)
                                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                ,monsterBuffs,types
                                                #endif // CATCHCHALLENGER_CLASS_MASTER
                                                );
    qDebug() << QStringLiteral("%1 monster skill(s) loaded").arg(monsterSkills.size());
}

void CommonDatapack::parseEvents()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    events=DatapackGeneralLoader::loadEvents(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+QStringLiteral("event.xml"));
    qDebug() << QStringLiteral("%1 event(s) loaded").arg(events.size());
    #endif
}

void CommonDatapack::parseMonsters()
{
    monsters=FightLoader::loadMonster(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS),monsterSkills
                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                      ,types,items.item
                                      #endif
                                      );
    qDebug() << QStringLiteral("%1 monster(s) loaded").arg(monsters.size());
}

void CommonDatapack::parseMonstersCollision()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollision=DatapackGeneralLoader::loadMonstersCollision(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+QStringLiteral("layers.xml"),CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.events);
    qDebug() << QStringLiteral("%1 monster(s) collisions loaded").arg(monstersCollision.size());
    #endif
}

void CommonDatapack::parseMonstersEvolutionItems()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.evolutionItem=FightLoader::loadMonsterEvolutionItems(monsters);
    qDebug() << QStringLiteral("%1 monster evolution items(s) loaded").arg(items.evolutionItem.size());
    #endif
}

void CommonDatapack::parseMonstersItemToLearn()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.itemToLearn=FightLoader::loadMonsterItemToLearn(monsters,items.evolutionItem);
    qDebug() << QStringLiteral("%1 monster items(s) to learn loaded").arg(items.itemToLearn.size());
    #endif
}

void CommonDatapack::parseProfileList()
{
    profileList=DatapackGeneralLoader::loadProfileList(datapackPath,datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYERBASE)+QStringLiteral("start.xml")
                                                       #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                       ,items.item
                                                       #endif // CATCHCHALLENGER_CLASS_MASTER
                                                       ,monsters,reputation).second;
    qDebug() << QStringLiteral("%1 profile(s) loaded").arg(profileList.size());
}

void CommonDatapack::parseLayersOptions()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    layersOptions=DatapackGeneralLoader::loadLayersOptions(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPBASE)+QStringLiteral("layers.xml"));
    qDebug() << QStringLiteral("layers options parsed");
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
