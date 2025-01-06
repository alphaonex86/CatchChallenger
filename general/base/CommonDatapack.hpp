#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <unordered_map>
#include <string>

#include "GeneralStructures.hpp"
#include "../tinyXML2/tinyxml2.hpp"
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
    void parseDatapack(const std::string &datapackPath);
    bool isParsedContent() const;
protected:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::unordered_map<uint8_t,Plant> plants;
    std::unordered_map<uint16_t,CraftingRecipe> craftingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCraftingRecipes;
    uint16_t craftingRecipesMaxId;
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    ItemFull items;
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
    std::vector<Reputation> reputation;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::unordered_map<uint16_t,Monster> monsters;
    uint16_t monstersMaxId;
    std::unordered_map<uint16_t,Skill> monsterSkills;
    std::vector<Profile> profileList;

    std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> xmlLoadedFile;//keep for Map_loader::getXmlCondition(), need to be deleted later
    std::vector<std::string > skins;//I think it's clean after use, database have only number
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    const std::unordered_map<uint8_t,Plant> &get_plants() const;
    const std::unordered_map<uint16_t,CraftingRecipe> &get_craftingRecipes() const;
    const std::unordered_map<uint16_t,uint16_t> &get_itemToCraftingRecipes() const;
    const uint16_t &get_craftingRecipesMaxId() const;
    const std::unordered_map<uint8_t,Buff> &get_monsterBuffs() const;
    const ItemFull &get_items() const;
    const LayersOptions &get_layersOptions() const;
    const std::vector<Event> &get_events() const;

    const bool &get_monsterRateApplied() const;
    void set_monsterRateApplied(const bool &v);

    //temp
    const std::vector<MonstersCollision> &get_monstersCollision() const;//never more than 255
    const std::vector<MonstersCollisionTemp> &get_monstersCollisionTemp() const;//never more than 255
    const std::vector<Type> &get_types() const;
    #endif
    const std::vector<Reputation> &get_reputation() const;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::vector<Reputation> &get_reputation_rw();
    const std::unordered_map<uint16_t,Monster> &get_monsters() const;
    const uint16_t &get_monstersMaxId() const;
    const std::unordered_map<uint16_t,Skill> &get_monsterSkills() const;
    const std::vector<Profile> &get_profileList() const;

    const std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> &get_xmlLoadedFile() const;//keep for Map_loader::getXmlCondition(), need to be deleted later
    std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> &get_xmlLoadedFile_rw();
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
        buf << items;
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
    }
    template <class B>
    void parse(B& buf) {
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        buf >> plants;
        buf >> craftingRecipes;
        buf >> itemToCraftingRecipes;
        buf >> craftingRecipesMaxId;
        buf >> monsterBuffs;
        buf >> items;
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
    }
    #endif
private:
    bool isParsed;
    bool parsing;
    std::string datapackPath;
private:
    void parseTypes();
    void parseItems();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseEvents();
    void parseMonsters();
    void parseMonstersCollision();
    void parseMonstersEvolutionItems();
    void parseMonstersItemToLearn();
    void parseReputation();
    void parseProfileList();
    void parseLayersOptions();
    void parseSkins();
};
}


#endif // COMMONDATAPACK_H
