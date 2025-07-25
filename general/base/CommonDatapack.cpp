#include "CommonDatapack.hpp"
#include "GeneralVariable.hpp"
#include "../fight/FightLoader.hpp"
#include "DatapackGeneralLoader.hpp"
#ifndef CATCHCHALLENGER_CLASS_MASTER
#include "CommonSettingsServer.hpp"
#endif

#include <iostream>
#include <vector>

using namespace CatchChallenger;

CommonDatapack CommonDatapack::commonDatapack;

CommonDatapack::CommonDatapack()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    items.itemMaxId=0;
    layersOptions.zoom=0;
    #endif
    isParsed=false;
    parsing=false;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monsterRateApplied=false;
    #endif
    monstersMaxId=0;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    craftingRecipesMaxId=0;
    #endif
}

void CommonDatapack::parseDatapack(const std::string &datapackPath)
{
    if(isParsed)
        return;
    if(parsing)
        return;
    parsing=true;

    this->datapackPath=datapackPath;
    #ifndef BOTTESTCONNECT
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
    parseProfileList();
    parseLayersOptions();
    #endif

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
    items=DatapackGeneralLoader::loadItems(tempNameToItemId,datapackPath+DATAPACK_BASE_PATH_ITEM,monsterBuffs);
    std::cout << items.item.size() << " items(s) loaded" << std::endl;
    std::cout << items.trap.size() << " trap(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseCraftingRecipes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    craftingRecipesMaxId=0;
    std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+DATAPACK_BASE_PATH_CRAFTING+"recipes.xml",items.item,craftingRecipesMaxId);
    craftingRecipes=multipleVariables.first;
    itemToCraftingRecipes=multipleVariables.second;
    std::cout << craftingRecipes.size() << " crafting recipe(s) loaded" << std::endl;
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
    monsterBuffs=FightLoader::loadMonsterBuff(tempNameToBuffId,datapackPath+DATAPACK_BASE_PATH_BUFF);
    std::cout << monsterBuffs.size() << " monster buff(s) loaded" << std::endl;
    #endif
}

//have to be after buff to be able to resolve name to id
void CommonDatapack::parseSkills()
{
    monsterSkills=FightLoader::loadMonsterSkill(tempNameToSkillId,datapackPath+DATAPACK_BASE_PATH_SKILL
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

//have to be after skill and item to be able to resolve name to id
void CommonDatapack::parseMonsters()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    if(CommonSettingsServer::commonSettingsServer.rates_xp<=0 || static_cast<double>(CommonSettingsServer::commonSettingsServer.rates_xp)==1.0 ||
            CommonSettingsServer::commonSettingsServer.rates_xp_pow<=0 || static_cast<double>(CommonSettingsServer::commonSettingsServer.rates_xp_pow)==1.0)
    {
        monsterRateApplied=false;
        CommonSettingsServer::commonSettingsServer.rates_xp=1000;//*1000 of real rate
        CommonSettingsServer::commonSettingsServer.rates_xp_pow=1000;//*1000 of real rate
    }
    else
        monsterRateApplied=true;
    #endif
    monsters=FightLoader::loadMonster(tempNameToMonsterId,datapackPath+DATAPACK_BASE_PATH_MONSTERS,monsterSkills
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
    monstersCollisionTemp=DatapackGeneralLoader::loadMonstersCollision(datapackPath+DATAPACK_BASE_PATH_MAPBASE+"layers.xml",CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.events);
    for(const MonstersCollisionTemp &i : monstersCollisionTemp)
    {
        MonstersCollision value;
        value.item=i.item;
        value.type=i.type;
        monstersCollision.push_back(value);
    }
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
    craftingRecipes.clear();
    events.clear();
    craftingRecipesMaxId=0;
    #endif // CATCHCHALLENGER_CLASS_MASTER
    reputation.clear();
    monsters.clear();
    monsterSkills.clear();
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monsterBuffs.clear();
    itemToCraftingRecipes.clear();
    items.item.clear();
    items.trap.clear();
    items.monsterItemEffect.clear();
    items.evolutionItem.clear();
    items.repel.clear();
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
    tempNameToItemId.clear();
    tempNameToSkillId.clear();
    tempNameToBuffId.clear();
    tempNameToMonsterId.clear();

    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monsterRateApplied=false;
    #endif
    parsing=false;
    isParsed=false;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
const std::unordered_map<uint8_t,Plant> &CommonDatapack::get_plants() const
{
    return plants;
}

const std::unordered_map<uint16_t,CraftingRecipe> &CommonDatapack::get_craftingRecipes() const
{
    return craftingRecipes;
}

const std::unordered_map<uint16_t,uint16_t> &CommonDatapack::get_itemToCraftingRecipes() const
{
    return itemToCraftingRecipes;
}

const uint16_t &CommonDatapack::get_craftingRecipesMaxId() const
{
    return craftingRecipesMaxId;
}

const std::unordered_map<uint8_t,Buff> &CommonDatapack::get_monsterBuffs() const
{
    return monsterBuffs;
}

const ItemFull &CommonDatapack::get_items() const
{
    return items;
}

const LayersOptions &CommonDatapack::get_layersOptions() const
{
    return layersOptions;
}

const std::vector<Event> &CommonDatapack::get_events() const
{
    return events;
}

const std::unordered_map<std::string,CATCHCHALLENGER_TYPE_ITEM> CommonDatapack::get_tempNameToItemId() const
{
    return tempNameToItemId;
}

const std::unordered_map<std::string,CATCHCHALLENGER_TYPE_BUFF> CommonDatapack::get_tempNameToBuffId() const
{
    return tempNameToBuffId;
}

const std::unordered_map<std::string,CATCHCHALLENGER_TYPE_SKILL> CommonDatapack::get_tempNameToSkillId() const
{
    return tempNameToSkillId;
}

const std::unordered_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> CommonDatapack::get_tempNameToMonsterId() const
{
    return tempNameToMonsterId;
}

const bool &CommonDatapack::get_monsterRateApplied() const
{
    return monsterRateApplied;
}

void CommonDatapack::set_monsterRateApplied(const bool &v)
{
    monsterRateApplied=v;
}

//temp
const std::vector<MonstersCollision> &CommonDatapack::get_monstersCollision() const
{
    return monstersCollision;
}
//never more than 255
const std::vector<MonstersCollisionTemp> &CommonDatapack::get_monstersCollisionTemp() const
{
    return monstersCollisionTemp;
}
//never more than 255
const std::vector<Type> &CommonDatapack::get_types() const
{
    return types;
}

#endif
const std::vector<Reputation> &CommonDatapack::get_reputation() const
{
    return reputation;
}

std::vector<Reputation> &CommonDatapack::get_reputation_rw()
{
    return reputation;
}

const std::unordered_map<uint16_t,Monster> &CommonDatapack::get_monsters() const
{
    return monsters;
}

const uint16_t &CommonDatapack::get_monstersMaxId() const
{
    return monstersMaxId;
}

const std::unordered_map<uint16_t,Skill> &CommonDatapack::get_monsterSkills() const
{
    return monsterSkills;
}

const std::vector<Profile> &CommonDatapack::get_profileList() const
{
    return profileList;
}

const std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> &CommonDatapack::get_xmlLoadedFile() const
{
    return xmlLoadedFile;
}

std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> &CommonDatapack::get_xmlLoadedFile_rw()
{
    return xmlLoadedFile;
}

const std::vector<std::string > &CommonDatapack::get_skins() const
{
    return skins;
}
