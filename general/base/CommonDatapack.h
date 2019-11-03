#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <unordered_map>
#include <string>

#include "GeneralStructures.h"
#include "GeneralVariable.h"
#include "tinyXML2/tinyxml2.h"

#ifndef EPOLLCATCHCHALLENGERSERVER
#include "tinyXML2/tinyxml2.h"
#endif

namespace CatchChallenger {
class CommonDatapack
{
public:
    explicit CommonDatapack();
    static CommonDatapack commonDatapack;
public:
    void unload();
    void parseDatapack(const std::string &datapackPath);
    bool isParsedContent() const;
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::unordered_map<uint8_t,Plant> plants;
    std::unordered_map<uint16_t,CraftingRecipe> craftingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCraftingRecipes;
    uint16_t craftingRecipesMaxId;
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    ItemFull items;
    std::unordered_map<uint16_t,Industry> industries;
    std::unordered_map<uint16_t,IndustryLink> industriesLink;
    LayersOptions layersOptions;
    std::vector<Event> events;

    bool monsterRateApplied;

    //temp
    std::vector<MonstersCollision> monstersCollision;//never more than 255
    std::vector<MonstersCollisionTemp> monstersCollisionTemp;//never more than 255
    std::vector<Type> types;
    #endif
    std::vector<Reputation> reputation;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::unordered_map<uint16_t,Monster> monsters;
    uint16_t monstersMaxId;
    std::unordered_map<uint16_t,Skill> monsterSkills;
    std::vector<Profile> profileList;

    std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> xmlLoadedFile;//keep for Map_loader::getXmlCondition(), need to be deleted later
    std::vector<std::string > skins;//I think it's clean after use, database have only number

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
        buf << industries;
        buf << industriesLink;
        #ifdef CATCHCHALLENGER_CLIENT
        buf << monstersCollision;
        #endif
        //<< types
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
        buf >> industries;
        buf >> industriesLink;
        #ifdef CATCHCHALLENGER_CLIENT
        buf >> monstersCollision;
        #endif
        //<< types
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
    virtual void parseIndustries();
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
