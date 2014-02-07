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

void CommonDatapack::parseDatapack(const QString &datapackPath)
{
    QMutexLocker mutexLocker(&inProgress);
    if(isParsed)
        return;
    this->datapackPath=datapackPath;
    parseBuff();
    parseTypes();
    parseItems();
    parsePlants();
    parseCraftingRecipes();
    parseSkills();
    parseMonsters();
    parseMonstersEvolutionItems();
    parseMonstersItemToLearn();
    parseReputation();
    parseQuests();
    parseBotFights();
    parseIndustries();
    parseProfileList();
    isParsed=true;
}


void CommonDatapack::parseTypes()
{
    types=FightLoader::loadTypes(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("type.xml"));
    qDebug() << QStringLiteral("%1 type(s) loaded").arg(types.size());
}

void CommonDatapack::parseItems()
{
    items=DatapackGeneralLoader::loadItems(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ITEM),monsterBuffs);
    qDebug() << QStringLiteral("%1 items(s) loaded").arg(items.item.size());
    qDebug() << QStringLiteral("%1 trap(s) loaded").arg(items.trap.size());
}

void CommonDatapack::parseIndustries()
{
    industries=DatapackGeneralLoader::loadIndustries(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_INDUSTRIES),items.item);
    industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_INDUSTRIES)+QStringLiteral("list.xml"),industries);
    qDebug() << QStringLiteral("%1 industries(s) loaded").arg(industries.size());
    qDebug() << QStringLiteral("%1 industries(s) link loaded").arg(industriesLink.size());
}

void CommonDatapack::parseCraftingRecipes()
{
    QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_CRAFTING)+QStringLiteral("recipes.xml"),items.item);
    crafingRecipes=multipleVariables.first;
    itemToCrafingRecipes=multipleVariables.second;
    qDebug() << QStringLiteral("%1 crafting recipe(s) loaded").arg(crafingRecipes.size());
}

void CommonDatapack::parsePlants()
{
    plants=DatapackGeneralLoader::loadPlants(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLANTS)+QStringLiteral("plants.xml"));
    qDebug() << QStringLiteral("%1 plant(s) loaded").arg(plants.size());
}

void CommonDatapack::parseQuests()
{
    quests=DatapackGeneralLoader::loadQuests(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS));
    qDebug() << QStringLiteral("%1 quest(s) loaded").arg(quests.size());
}

void CommonDatapack::parseReputation()
{
    reputation=DatapackGeneralLoader::loadReputation(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYER)+QStringLiteral("reputation.xml"));
    qDebug() << QStringLiteral("%1 reputation(s) loaded").arg(reputation.size());
}

void CommonDatapack::parseBuff()
{
    monsterBuffs=FightLoader::loadMonsterBuff(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("buff.xml"));
    qDebug() << QStringLiteral("%1 monster buff(s) loaded").arg(monsterBuffs.size());
}

void CommonDatapack::parseSkills()
{
    monsterSkills=FightLoader::loadMonsterSkill(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("skill.xml"),monsterBuffs,types);
    qDebug() << QStringLiteral("%1 monster skill(s) loaded").arg(monsterSkills.size());
}

void CommonDatapack::parseMonsters()
{
    monsters=FightLoader::loadMonster(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("monster.xml"),monsterSkills,types,items.item);
    qDebug() << QStringLiteral("%1 monster(s) loaded").arg(monsters.size());
}

void CommonDatapack::parseMonstersEvolutionItems()
{
    items.evolutionItem=FightLoader::loadMonsterEvolutionItems(monsters);
    qDebug() << QStringLiteral("%1 monster evolution items(s) loaded").arg(items.evolutionItem.size());
}

void CommonDatapack::parseMonstersItemToLearn()
{
    items.itemToLearn=FightLoader::loadMonsterItemToLearn(monsters,items.evolutionItem);
    qDebug() << QStringLiteral("%1 monster items(s) to learn loaded").arg(items.itemToLearn.size());
}

void CommonDatapack::parseBotFights()
{
    botFights=FightLoader::loadFight(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_FIGHT), monsters, monsterSkills);
    qDebug() << QStringLiteral("%1 bot fight(s) loaded").arg(botFights.size());
}

void CommonDatapack::parseProfileList()
{
    profileList=DatapackGeneralLoader::loadProfileList(datapackPath,datapackPath+QStringLiteral(DATAPACK_BASE_PATH_PLAYER)+QStringLiteral("start.xml"),items.item,monsters,reputation).second;
    qDebug() << QStringLiteral("%1 profile(s) loaded").arg(profileList.size());
}

void CommonDatapack::unload()
{
    QMutexLocker mutexLocker(&inProgress);
    if(!isParsed)
        return;
    botFights.clear();
    plants.clear();
    crafingRecipes.clear();
    reputation.clear();
    quests.clear();
    monsters.clear();
    monsterSkills.clear();
    monsterBuffs.clear();
    itemToCrafingRecipes.clear();
    items.item.clear();
    items.trap.clear();
    items.monsterItemEffect.clear();
    items.evolutionItem.clear();
    items.repel.clear();
    industries.clear();
    profileList.clear();
    types.clear();
    xmlLoadedFile.clear();
    teleportConditionsUnparsed.clear();
    isParsed=false;
}

