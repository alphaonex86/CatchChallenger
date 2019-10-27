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
    std::unordered_map<uint16_t,CrafingRecipe> crafingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCrafingRecipes;
    uint16_t crafingRecipesMaxId;
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    ItemFull items;
    std::unordered_map<uint16_t,Industry> industries;
    std::unordered_map<uint16_t,IndustryLink> industriesLink;
    std::vector<MonstersCollision> monstersCollision;//never more than 255
    std::vector<Type> types;
    LayersOptions layersOptions;
    std::vector<Event> events;

    bool monsterRateApplied;
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
        buf
                #ifndef CATCHCHALLENGER_CLASS_MASTER
                << plants << crafingRecipes << itemToCrafingRecipes << crafingRecipesMaxId << monsterBuffs << items << industries << industriesLink << monstersCollision
                << types << layersOptions << events << monsterRateApplied
                #endif
                << reputation << monsters << monstersMaxId << monsterSkills << profileList
               ;
    }
    template <class B>
    void parse(B& buf) {
        buf
                #ifndef CATCHCHALLENGER_CLASS_MASTER
                >> plants >> crafingRecipes >> itemToCrafingRecipes >> crafingRecipesMaxId >> monsterBuffs >> items >> industries >> industriesLink >> monstersCollision
                >> types >> layersOptions >> events >> monsterRateApplied
                #endif
                >> reputation >> monsters >> monstersMaxId >> monsterSkills >> profileList
               ;
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
