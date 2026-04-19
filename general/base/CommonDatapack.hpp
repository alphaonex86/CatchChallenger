#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <string>
#include <unordered_map>

#include "GeneralStructures.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../tinyXML2/tinyxml2.hpp"
#endif
#include "lib.h"

namespace CatchChallenger {
class DLL_PUBLIC CommonDatapack
{
public:
    friend class DatapackGeneralLoader;
    friend class CommonDatapackServerSpec;//industries, see CommonDatapackServerSpec::parseIndustries()
    explicit CommonDatapack();
    static CommonDatapack commonDatapack;
public:
    void unload();
    #ifndef CATCHCHALLENGER_NOXML
    void parseDatapack(const std::string &datapackPath);
    #endif
    bool isParsedContent() const;
protected:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_PLANT,Plant> plants;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_CRAFTINGRECIPE,CraftingRecipe> craftingRecipes;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_CRAFTINGRECIPE> itemToCraftingRecipes;
    CATCHCHALLENGER_TYPE_CRAFTINGRECIPE craftingRecipesMaxId;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_BUFF,Buff> monsterBuffs;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffect> > monsterItemEffect;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffectOutOfFight> > monsterItemEffectOutOfFight;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER/*monster*/,CATCHCHALLENGER_TYPE_MONSTER/*evolveTo*/> > evolutionItem;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM/*item*/, catchchallenger_datapack_set<CATCHCHALLENGER_TYPE_MONSTER/*monster*/> > itemToLearn;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, uint32_t> repel;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> items;
    CATCHCHALLENGER_TYPE_ITEM itemMaxId;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Trap> trap;
    LayersOptions layersOptions;
    std::vector<Event> events;

    bool monsterRateApplied;

    //temp
    std::vector<MonstersCollision> monstersCollision;//never more than 255
    #ifdef CATCHCHALLENGER_CACHE_HPS
public:
    #endif
    std::vector<MonstersCollisionTemp> monstersCollisionTemp;//never more than 255
    #ifdef CATCHCHALLENGER_CACHE_HPS
protected:
    #endif
    std::vector<Type> types;
    #endif
    std::vector<Reputation> reputation;//Player_private_and_public_informations, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_REPUTATION,PlayerReputation> reputation;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> monsters;
    CATCHCHALLENGER_TYPE_MONSTER monstersMaxId;
    catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_SKILL,Skill> monsterSkills;
    std::vector<Profile> profileList;

    #ifndef CATCHCHALLENGER_NOXML
    std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> xmlLoadedFile;//keep for Map_loader::getXmlCondition(), need to be deleted later
    #endif
    std::vector<std::string > skins;//I think it's clean after use, database have only number
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    //plants
    size_t get_plants_size() const;
    const Plant &get_plant(const CATCHCHALLENGER_TYPE_PLANT &key) const;
    bool has_plant(const CATCHCHALLENGER_TYPE_PLANT &key) const;
    //craftingRecipes
    size_t get_craftingRecipes_size() const;
    const CraftingRecipe &get_craftingRecipe(const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &key) const;
    bool has_craftingRecipe(const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &key) const;
    //itemToCraftingRecipes
    size_t get_itemToCraftingRecipes_size() const;
    const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &get_itemToCraftingRecipe(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_itemToCraftingRecipe(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //craftingRecipesMaxId
    const CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &get_craftingRecipesMaxId() const;
    //monsterBuffs
    size_t get_monsterBuffs_size() const;
    const Buff &get_monsterBuff(const CATCHCHALLENGER_TYPE_BUFF &key) const;
    bool has_monsterBuff(const CATCHCHALLENGER_TYPE_BUFF &key) const;
    //monsterItemEffect
    size_t get_monsterItemEffect_size() const;
    const std::vector<MonsterItemEffect> &get_monsterItemEffect(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_monsterItemEffect(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //monsterItemEffectOutOfFight
    size_t get_monsterItemEffectOutOfFight_size() const;
    const std::vector<MonsterItemEffectOutOfFight> &get_monsterItemEffectOutOfFight(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_monsterItemEffectOutOfFight(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //evolutionItem (nested map: item -> monster -> evolveTo)
    size_t get_evolutionItem_size() const;
    bool has_evolutionItem(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_evolutionItemForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const;
    CATCHCHALLENGER_TYPE_MONSTER get_evolutionItemForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const;
    //itemToLearn (nested map/set: item -> set<monster>)
    size_t get_itemToLearn_size() const;
    bool has_itemToLearn(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_itemToLearnForMonster(const CATCHCHALLENGER_TYPE_ITEM &itemKey, const CATCHCHALLENGER_TYPE_MONSTER &monsterKey) const;
    //repel
    size_t get_repel_size() const;
    uint32_t get_repel(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_repel(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //items
    size_t get_items_size() const;
    const Item &get_item(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_item(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //itemMaxId
    const CATCHCHALLENGER_TYPE_ITEM &get_itemMaxId() const;
    //trap
    size_t get_trap_size() const;
    const Trap &get_trap(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    bool has_trap(const CATCHCHALLENGER_TYPE_ITEM &key) const;
    //layersOptions
    const LayersOptions &get_layersOptions() const;
    //events
    const std::vector<Event> &get_events() const;
    const bool &get_monsterRateApplied() const;
    void set_monsterRateApplied(const bool &v);

    //temp
    const std::vector<MonstersCollision> &get_monstersCollision() const;//never more than 255
    const std::vector<MonstersCollisionTemp> &get_monstersCollisionTemp() const;//never more than 255
    const std::vector<Type> &get_types() const;
    #endif
    //tempNameToItemId
    size_t get_tempNameToItemId_size() const;
    CATCHCHALLENGER_TYPE_ITEM get_tempNameToItemId(const std::string &name) const;
    bool has_tempNameToItemId(const std::string &name) const;
    void set_tempNameToItemId(const std::string &name, const CATCHCHALLENGER_TYPE_ITEM &id);
    //tempNameToSkillId
    size_t get_tempNameToSkillId_size() const;
    CATCHCHALLENGER_TYPE_SKILL get_tempNameToSkillId(const std::string &name) const;
    bool has_tempNameToSkillId(const std::string &name) const;
    //tempNameToBuffId
    size_t get_tempNameToBuffId_size() const;
    CATCHCHALLENGER_TYPE_BUFF get_tempNameToBuffId(const std::string &name) const;
    bool has_tempNameToBuffId(const std::string &name) const;
    //tempNameToMonsterId
    size_t get_tempNameToMonsterId_size() const;
    CATCHCHALLENGER_TYPE_MONSTER get_tempNameToMonsterId(const std::string &name) const;
    bool has_tempNameToMonsterId(const std::string &name) const;
    void set_tempNameToMonsterId(const std::string &name, const CATCHCHALLENGER_TYPE_MONSTER &id);
    const std::vector<Reputation> &get_reputation() const;//Player_private_and_public_informations, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_REPUTATION,PlayerReputation> reputation;
    std::vector<Reputation> &get_reputation_rw();
    //monsters
    size_t get_monsters_size() const;
    const Monster &get_monster(const CATCHCHALLENGER_TYPE_MONSTER &key) const;
    bool has_monster(const CATCHCHALLENGER_TYPE_MONSTER &key) const;
    //monstersMaxId
    const CATCHCHALLENGER_TYPE_MONSTER &get_monstersMaxId() const;
    //monsterSkills
    size_t get_monsterSkills_size() const;
    const Skill &get_monsterSkill(const CATCHCHALLENGER_TYPE_SKILL &key) const;
    bool has_monsterSkill(const CATCHCHALLENGER_TYPE_SKILL &key) const;
    //profileList
    const std::vector<Profile> &get_profileList() const;
public:
    #ifndef CATCHCHALLENGER_NOXML
    //xmlLoadedFile
    size_t get_xmlLoadedFile_size() const;
    const tinyxml2::XMLDocument &get_xmlLoadedFile(const std::string &file) const;
    bool has_xmlLoadedFile(const std::string &file) const;
    tinyxml2::XMLDocument &get_xmlLoadedFile_rw(const std::string &file);
    void clear_xmlLoadedFile();
    #endif
    const std::vector<std::string > &get_skins() const;//I think it's clean after use, database have only number
    #ifdef CATCHCHALLENGER_CACHE_HPS
    template <class B>
    void serialize(B& buf) const {
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        buf << plants;
        buf << craftingRecipes;
        buf << itemToCraftingRecipes;
        buf << craftingRecipesMaxId;
        buf << monsterBuffs;
        buf << monsterItemEffect << monsterItemEffectOutOfFight << evolutionItem << itemToLearn << repel << items << itemMaxId << trap;
        #ifdef CATCHCHALLENGER_CLIENT
        buf << monstersCollision;
        buf << monstersCollisionTemp;
        buf << isParsed;
        #endif
        //CatchChallenger::CommonDatapack::commonDatapack.types for typeDefinition.multiplicator
        buf << types;
        buf << layersOptions;
        buf << events;
        buf << monstersCollision;
        buf << monsterRateApplied;
        #endif
        buf << reputation;
        buf << monsters;
        buf << monstersMaxId;
        buf << monsterSkills;
        buf << profileList;
        buf << skins;
        buf << tempNameToItemId;
        buf << tempNameToMonsterId;
    }
    template <class B>
    void parse(B& buf) {
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        buf >> plants;
        buf >> craftingRecipes;
        buf >> itemToCraftingRecipes;
        buf >> craftingRecipesMaxId;
        buf >> monsterBuffs;
        buf >> monsterItemEffect >> monsterItemEffectOutOfFight >> evolutionItem >> itemToLearn >> repel >> items >> itemMaxId >> trap;
        #ifdef CATCHCHALLENGER_CLIENT
        buf >> monstersCollision;
        buf >> monstersCollisionTemp;
        buf >> isParsed;
        #endif
        //CatchChallenger::CommonDatapack::commonDatapack.types for typeDefinition.multiplicator
        buf >> types;
        buf >> layersOptions;
        buf >> events;
        buf >> monstersCollision;
        buf >> monsterRateApplied;
        #endif
        buf >> reputation;
        buf >> monsters;
        buf >> monstersMaxId;
        buf >> monsterSkills;
        buf >> profileList;
        buf >> skins;
        buf >> tempNameToItemId;
        buf >> tempNameToMonsterId;
    }
    #endif
private:
    bool isParsed;
    bool parsing;
    std::string datapackPath;
    //to fill, used to resolv into file the name and not the id, more easy to edit the xml file, all in lowercase
    catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_ITEM> tempNameToItemId;
    catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_BUFF> tempNameToBuffId;
    catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_SKILL> tempNameToSkillId;
    catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> tempNameToMonsterId;
private:
    #ifndef CATCHCHALLENGER_NOXML
    void parseTypes();
    void parseItems();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    //have to be after buff to be able to resolve name to id
    void parseSkills();
    void parseEvents();
    //have to be after skill and item to be able to resolve name to id
    void parseMonsters();
    void parseMonstersCollision();
    void parseMonstersEvolutionItems();
    void parseMonstersItemToLearn();
    void parseReputation();
    void parseProfileList();
    void parseLayersOptions();
    void parseSkins();
    #endif
};
}


#endif // COMMONDATAPACK_H
