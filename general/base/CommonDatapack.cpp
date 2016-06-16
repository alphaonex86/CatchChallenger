#include "CommonDatapack.h"
#include "GeneralVariable.h"
#include "FacilityLib.h"
#include "../fight/FightLoader.h"
#include "DatapackGeneralLoader.h"

#include <iostream>
#include <vector>

using namespace CatchChallenger;

CommonDatapack CommonDatapack::commonDatapack;

CommonDatapack::CommonDatapack()
{
    isParsed=false;
    parsing=false;
    monstersMaxId=0;
    crafingRecipesMaxId=0;
}

void CommonDatapack::parseDatapack(const std::string &datapackPath)
{
    if(isParsed)
        return;
    if(parsing)
        return;
    parsing=true;

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

    parsing=false;
    isParsed=true;
}


void CommonDatapack::parseTypes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    types=FightLoader::loadTypes(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"type.xml");
    std::cout << types.size() << " type(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseSkins()
{
    skins=DatapackGeneralLoader::loadSkins(datapackPath+DATAPACK_BASE_PATH_SKIN);
    std::cout << skins.size() << " skin(s) loaded" << std::endl;
}

void CommonDatapack::parseItems()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    items=DatapackGeneralLoader::loadItems(datapackPath+DATAPACK_BASE_PATH_ITEM,monsterBuffs);
    std::cout << items.item.size() << " items(s) loaded" << std::endl;
    std::cout << items.trap.size() << " trap(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseIndustries()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    industries=DatapackGeneralLoader::loadIndustries(datapackPath+DATAPACK_BASE_PATH_INDUSTRIESBASE,items.item);
    industriesLink=DatapackGeneralLoader::loadIndustriesLink(datapackPath+DATAPACK_BASE_PATH_INDUSTRIESBASE+"list.xml",industries);
    std::cout << industries.size() << " industries loaded" << std::endl;
    std::cout << industriesLink.size() << " industries link loaded" << std::endl;
    #endif
}

void CommonDatapack::parseCraftingRecipes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    crafingRecipesMaxId=0;
    std::pair<std::unordered_map<uint16_t,CrafingRecipe>,std::unordered_map<uint16_t,uint16_t> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+DATAPACK_BASE_PATH_CRAFTING+"recipes.xml",items.item,crafingRecipesMaxId);
    crafingRecipes=multipleVariables.first;
    itemToCrafingRecipes=multipleVariables.second;
    std::cout << crafingRecipes.size() << " crafting recipe(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parsePlants()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    plants=DatapackGeneralLoader::loadPlants(datapackPath+DATAPACK_BASE_PATH_PLANTS+"plants.xml");
    std::cout << plants.size() << " plant(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseReputation()
{
    reputation=DatapackGeneralLoader::loadReputation(datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"reputation.xml");
    std::cout << reputation.size() << " reputation(s) loaded" << std::endl;
}

void CommonDatapack::parseBuff()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    monsterBuffs=FightLoader::loadMonsterBuff(datapackPath+DATAPACK_BASE_PATH_BUFF);
    std::cout << monsterBuffs.size() << " monster buff(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseSkills()
{
    monsterSkills=FightLoader::loadMonsterSkill(datapackPath+DATAPACK_BASE_PATH_SKILL
                                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                ,monsterBuffs,types
                                                #endif // CATCHCHALLENGER_CLASS_MASTER
                                                );
    std::cout << monsterSkills.size() << " monster skill(s) loaded" << std::endl;
}

void CommonDatapack::parseEvents()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    events=DatapackGeneralLoader::loadEvents(datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"event.xml");
    std::cout << events.size() << " event(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseMonsters()
{
    monsters=FightLoader::loadMonster(datapackPath+DATAPACK_BASE_PATH_MONSTERS,monsterSkills
                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                      ,types,items.item
                                      ,monstersMaxId
                                      #endif
                                      );
    std::cout << monsters.size() << " monster(s) loaded" << std::endl;
}

void CommonDatapack::parseMonstersCollision()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollision=DatapackGeneralLoader::loadMonstersCollision(datapackPath+DATAPACK_BASE_PATH_MAPBASE+"layers.xml",CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.events);
    std::cout << monstersCollision.size() << " monster(s) collisions loaded" << std::endl;
    #endif
}

void CommonDatapack::parseMonstersEvolutionItems()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.evolutionItem=FightLoader::loadMonsterEvolutionItems(monsters);
    std::cout << items.evolutionItem.size() << " monster evolution items(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseMonstersItemToLearn()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    items.itemToLearn=FightLoader::loadMonsterItemToLearn(monsters,items.evolutionItem);
    std::cout << items.itemToLearn.size() << " monster items(s) to learn loaded" << std::endl;
    #endif
}

void CommonDatapack::parseProfileList()
{
    profileList=DatapackGeneralLoader::loadProfileList(datapackPath,datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"start.xml"
                                                       #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                       ,items.item
                                                       #endif // CATCHCHALLENGER_CLASS_MASTER
                                                       ,monsters,reputation).second;
    std::cout << profileList.size() << " profile(s) loaded" << std::endl;
}

void CommonDatapack::parseLayersOptions()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    layersOptions=DatapackGeneralLoader::loadLayersOptions(datapackPath+DATAPACK_BASE_PATH_MAPBASE+"layers.xml");
    std::cout << "layers options parsed" << std::endl;
    #endif
}

bool CommonDatapack::isParsedContent() const
{
    return isParsed;
}

void CommonDatapack::unload()
{
    if(!isParsed)
        return;
    if(parsing)
        return;
    parsing=true;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    plants.clear();
    crafingRecipes.clear();
    events.clear();
    crafingRecipesMaxId=0;
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
    parsing=false;
    isParsed=false;
}
