#include "CommonDatapack.hpp"
#include "GeneralVariable.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../fight/FightLoader.hpp"
#include "DatapackGeneralLoader/DatapackGeneralLoader.hpp"
#ifndef CATCHCHALLENGER_CLASS_MASTER
#include "CommonSettingsServer.hpp"
#endif
#endif

#include <iostream>
#include <vector>

using namespace CatchChallenger;

CommonDatapack CommonDatapack::commonDatapack;

CommonDatapack::CommonDatapack()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    itemMaxId=0;
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

#ifndef CATCHCHALLENGER_NOXML
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
    #else
    parseSkins();
    parseReputation();
    parseBuff();
    parseTypes();
    parseItems();
    parseSkills();
    parseMonsters();
    parseProfileList();
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
    ItemFull itemFull=DatapackGeneralLoader::loadItems(tempNameToItemId,datapackPath+DATAPACK_BASE_PATH_ITEM,monsterBuffs);
    monsterItemEffect=std::move(itemFull.monsterItemEffect);
    monsterItemEffectOutOfFight=std::move(itemFull.monsterItemEffectOutOfFight);
    evolutionItem=std::move(itemFull.evolutionItem);
    itemToLearn=std::move(itemFull.itemToLearn);
    repel=std::move(itemFull.repel);
    items=std::move(itemFull.item);
    itemMaxId=itemFull.itemMaxId;
    trap=std::move(itemFull.trap);
    std::cout << items.size() << " items(s) loaded" << std::endl;
    std::cout << trap.size() << " trap(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseCraftingRecipes()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    craftingRecipesMaxId=0;
    std::pair<catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_CRAFTINGRECIPE,CraftingRecipe>,catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_CRAFTINGRECIPE> > multipleVariables=DatapackGeneralLoader::loadCraftingRecipes(datapackPath+DATAPACK_BASE_PATH_CRAFTING+"recipes.xml",items,craftingRecipesMaxId);
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
                                      ,types,items
                                      ,monstersMaxId
                                      #endif
                                      );
    std::cout << monsters.size() << " monster(s) loaded" << std::endl;
}

void CommonDatapack::parseMonstersCollision()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollisionTemp=DatapackGeneralLoader::loadMonstersCollision(datapackPath+DATAPACK_BASE_PATH_MAPBASE+"layers.xml",CommonDatapack::commonDatapack.items,CommonDatapack::commonDatapack.events);
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
    evolutionItem=FightLoader::loadMonsterEvolutionItems(monsters);
    std::cout << evolutionItem.size() << " monster evolution items(s) loaded" << std::endl;
    #endif
}

void CommonDatapack::parseMonstersItemToLearn()
{
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    itemToLearn=FightLoader::loadMonsterItemToEvolution(monsters,evolutionItem);
    std::cout << itemToLearn.size() << " monster items(s) to learn loaded" << std::endl;
    #endif
}

void CommonDatapack::parseProfileList()
{
    profileList=DatapackGeneralLoader::loadProfileList(datapackPath,datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"start.xml"
                                                       ,reputation).second;
    std::cout << profileList.size() << " profile(s) loaded" << std::endl;
}

void CommonDatapack::parseLayersOptions()
{
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    layersOptions=DatapackGeneralLoader::loadLayersOptions(datapackPath+DATAPACK_BASE_PATH_MAPBASE+"layers.xml");
    std::cout << "layers options parsed" << std::endl;
    #endif
}
#endif

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
    monsterItemEffect.clear();
    monsterItemEffectOutOfFight.clear();
    evolutionItem.clear();
    itemToLearn.clear();
    repel.clear();
    items.clear();
    itemMaxId=0;
    trap.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    profileList.clear();
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    monstersCollision.clear();
    types.clear();
    #endif // CATCHCHALLENGER_CLASS_MASTER
    #ifndef EPOLLCATCHCHALLENGERSERVER
    #ifndef CATCHCHALLENGER_NOXML
    xmlLoadedFile.clear();
    #endif
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
//plants
size_t CommonDatapack::get_plants_size() const { return plants.size(); }
const Plant &CommonDatapack::get_plant(const CATCHCHALLENGER_TYPE_PLANT &key) const { return plants.at(key); }
bool CommonDatapack::has_plant(const CATCHCHALLENGER_TYPE_PLANT &key) const { return plants.find(key)!=plants.cend(); }
//craftingRecipes
size_t CommonDatapack::get_craftingRecipes_size() const { return craftingRecipes.size(); }
const CraftingRecipe &CommonDatapack::get_craftingRecipe(const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &key) const { return craftingRecipes.at(key); }
bool CommonDatapack::has_craftingRecipe(const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &key) const { return craftingRecipes.find(key)!=craftingRecipes.cend(); }
//itemToCraftingRecipes
size_t CommonDatapack::get_itemToCraftingRecipes_size() const { return itemToCraftingRecipes.size(); }
const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &CommonDatapack::get_itemToCraftingRecipe(const CATCHCHALLENGER_TYPE_ITEM &key) const { return itemToCraftingRecipes.at(key); }
bool CommonDatapack::has_itemToCraftingRecipe(const CATCHCHALLENGER_TYPE_ITEM &key) const { return itemToCraftingRecipes.find(key)!=itemToCraftingRecipes.cend(); }
//craftingRecipesMaxId
const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &CommonDatapack::get_craftingRecipesMaxId() const { return craftingRecipesMaxId; }
//monsterBuffs
size_t CommonDatapack::get_monsterBuffs_size() const { return monsterBuffs.size(); }
const Buff &CommonDatapack::get_monsterBuff(const CATCHCHALLENGER_TYPE_BUFF &key) const { return monsterBuffs.at(key); }
bool CommonDatapack::has_monsterBuff(const CATCHCHALLENGER_TYPE_BUFF &key) const { return monsterBuffs.find(key)!=monsterBuffs.cend(); }
//monsterItemEffect
size_t CommonDatapack::get_monsterItemEffect_size() const { return monsterItemEffect.size(); }
const std::vector<MonsterItemEffect> &CommonDatapack::get_monsterItemEffect(const CATCHCHALLENGER_TYPE_ITEM &key) const { return monsterItemEffect.at(key); }
bool CommonDatapack::has_monsterItemEffect(const CATCHCHALLENGER_TYPE_ITEM &key) const { return monsterItemEffect.find(key)!=monsterItemEffect.cend(); }
//monsterItemEffectOutOfFight
size_t CommonDatapack::get_monsterItemEffectOutOfFight_size() const { return monsterItemEffectOutOfFight.size(); }
const std::vector<MonsterItemEffectOutOfFight> &CommonDatapack::get_monsterItemEffectOutOfFight(const CATCHCHALLENGER_TYPE_ITEM &key) const { return monsterItemEffectOutOfFight.at(key); }
bool CommonDatapack::has_monsterItemEffectOutOfFight(const CATCHCHALLENGER_TYPE_ITEM &key) const { return monsterItemEffectOutOfFight.find(key)!=monsterItemEffectOutOfFight.cend(); }
//evolutionItem
size_t CommonDatapack::get_evolutionItem_size() const { return evolutionItem.size(); }
bool CommonDatapack::has_evolutionItem(const CATCHCHALLENGER_TYPE_ITEM &key) const { return evolutionItem.find(key)!=evolutionItem.cend(); }
bool CommonDatapack::has_evolutionItemForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const
{
    const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,CATCHCHALLENGER_TYPE_MONSTER> >::const_iterator i=evolutionItem.find(itemKey);
    if(i==evolutionItem.cend()) return false;
    return i->second.find(monsterKey)!=i->second.cend();
}
CATCHCHALLENGER_TYPE_MONSTER CommonDatapack::get_evolutionItemForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const { return evolutionItem.at(itemKey).at(monsterKey); }
//itemToLearn
size_t CommonDatapack::get_itemToLearn_size() const { return itemToLearn.size(); }
bool CommonDatapack::has_itemToLearn(const CATCHCHALLENGER_TYPE_ITEM &key) const { return itemToLearn.find(key)!=itemToLearn.cend(); }
bool CommonDatapack::has_itemToLearnForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const
{
    const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, catchchallenger_datapack_set<CATCHCHALLENGER_TYPE_MONSTER> >::const_iterator i=itemToLearn.find(itemKey);
    if(i==itemToLearn.cend()) return false;
    return i->second.find(monsterKey)!=i->second.cend();
}
//repel
size_t CommonDatapack::get_repel_size() const { return repel.size(); }
uint32_t CommonDatapack::get_repel(const CATCHCHALLENGER_TYPE_ITEM &key) const { return repel.at(key); }
bool CommonDatapack::has_repel(const CATCHCHALLENGER_TYPE_ITEM &key) const { return repel.find(key)!=repel.cend(); }
//items
size_t CommonDatapack::get_items_size() const { return items.size(); }
const Item &CommonDatapack::get_item(const CATCHCHALLENGER_TYPE_ITEM &key) const { return items.at(key); }
bool CommonDatapack::has_item(const CATCHCHALLENGER_TYPE_ITEM &key) const { return items.find(key)!=items.cend(); }
//itemMaxId
const CATCHCHALLENGER_TYPE_ITEM &CommonDatapack::get_itemMaxId() const { return itemMaxId; }
//trap
size_t CommonDatapack::get_trap_size() const { return trap.size(); }
const Trap &CommonDatapack::get_trap(const CATCHCHALLENGER_TYPE_ITEM &key) const { return trap.at(key); }
bool CommonDatapack::has_trap(const CATCHCHALLENGER_TYPE_ITEM &key) const { return trap.find(key)!=trap.cend(); }
//layersOptions
const LayersOptions &CommonDatapack::get_layersOptions() const { return layersOptions; }
//events
const std::vector<Event> &CommonDatapack::get_events() const { return events; }
const bool &CommonDatapack::get_monsterRateApplied() const { return monsterRateApplied; }
void CommonDatapack::set_monsterRateApplied(const bool &v) { monsterRateApplied=v; }

//temp
const std::vector<MonstersCollision> &CommonDatapack::get_monstersCollision() const { return monstersCollision; }
const std::vector<MonstersCollisionTemp> &CommonDatapack::get_monstersCollisionTemp() const { return monstersCollisionTemp; }
const std::vector<Type> &CommonDatapack::get_types() const { return types; }

#endif
//tempNameToItemId
size_t CommonDatapack::get_tempNameToItemId_size() const { return tempNameToItemId.size(); }
CATCHCHALLENGER_TYPE_ITEM CommonDatapack::get_tempNameToItemId(const std::string &name) const { return tempNameToItemId.at(name); }
bool CommonDatapack::has_tempNameToItemId(const std::string &name) const { return tempNameToItemId.find(name)!=tempNameToItemId.cend(); }
void CommonDatapack::set_tempNameToItemId(const std::string &name, const CATCHCHALLENGER_TYPE_ITEM &id) { tempNameToItemId[name]=id; }
//tempNameToBuffId
size_t CommonDatapack::get_tempNameToBuffId_size() const { return tempNameToBuffId.size(); }
CATCHCHALLENGER_TYPE_BUFF CommonDatapack::get_tempNameToBuffId(const std::string &name) const { return tempNameToBuffId.at(name); }
bool CommonDatapack::has_tempNameToBuffId(const std::string &name) const { return tempNameToBuffId.find(name)!=tempNameToBuffId.cend(); }
//tempNameToSkillId
size_t CommonDatapack::get_tempNameToSkillId_size() const { return tempNameToSkillId.size(); }
CATCHCHALLENGER_TYPE_SKILL CommonDatapack::get_tempNameToSkillId(const std::string &name) const { return tempNameToSkillId.at(name); }
bool CommonDatapack::has_tempNameToSkillId(const std::string &name) const { return tempNameToSkillId.find(name)!=tempNameToSkillId.cend(); }
//tempNameToMonsterId
size_t CommonDatapack::get_tempNameToMonsterId_size() const { return tempNameToMonsterId.size(); }
CATCHCHALLENGER_TYPE_MONSTER CommonDatapack::get_tempNameToMonsterId(const std::string &name) const { return tempNameToMonsterId.at(name); }
bool CommonDatapack::has_tempNameToMonsterId(const std::string &name) const { return tempNameToMonsterId.find(name)!=tempNameToMonsterId.cend(); }
void CommonDatapack::set_tempNameToMonsterId(const std::string &name, const CATCHCHALLENGER_TYPE_MONSTER &id) { tempNameToMonsterId[name]=id; }
const std::vector<Reputation> &CommonDatapack::get_reputation() const { return reputation; }
std::vector<Reputation> &CommonDatapack::get_reputation_rw() { return reputation; }

//monsters
size_t CommonDatapack::get_monsters_size() const { return monsters.size(); }
const Monster &CommonDatapack::get_monster(const CATCHCHALLENGER_TYPE_MONSTER &key) const { return monsters.at(key); }
bool CommonDatapack::has_monster(const CATCHCHALLENGER_TYPE_MONSTER &key) const { return monsters.find(key)!=monsters.cend(); }
//monstersMaxId
const CATCHCHALLENGER_TYPE_MONSTER &CommonDatapack::get_monstersMaxId() const { return monstersMaxId; }
//monsterSkills
size_t CommonDatapack::get_monsterSkills_size() const { return monsterSkills.size(); }
const Skill &CommonDatapack::get_monsterSkill(const CATCHCHALLENGER_TYPE_SKILL &key) const { return monsterSkills.at(key); }
bool CommonDatapack::has_monsterSkill(const CATCHCHALLENGER_TYPE_SKILL &key) const { return monsterSkills.find(key)!=monsterSkills.cend(); }
//profileList
const std::vector<Profile> &CommonDatapack::get_profileList() const { return profileList; }

#ifndef CATCHCHALLENGER_NOXML
//xmlLoadedFile
size_t CommonDatapack::get_xmlLoadedFile_size() const { return xmlLoadedFile.size(); }
const tinyxml2::XMLDocument &CommonDatapack::get_xmlLoadedFile(const std::string &file) const { return xmlLoadedFile.at(file); }
bool CommonDatapack::has_xmlLoadedFile(const std::string &file) const { return xmlLoadedFile.find(file)!=xmlLoadedFile.cend(); }
tinyxml2::XMLDocument &CommonDatapack::get_xmlLoadedFile_rw(const std::string &file) { return xmlLoadedFile[file]; }
void CommonDatapack::clear_xmlLoadedFile() { xmlLoadedFile.clear(); }
#endif

const std::vector<std::string > &CommonDatapack::get_skins() const
{
    return skins;
}
