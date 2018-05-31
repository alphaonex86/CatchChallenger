#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <unordered_map>
#include <string>

#include "GeneralStructures.h"
#include "GeneralVariable.h"

#ifndef EPOLLCATCHCHALLENGERSERVER
#include "tinyXML2/tinyxml2.h"
#include <QObject>
#endif

namespace CatchChallenger {
class CommonDatapack
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
#ifndef EPOLLCATCHCHALLENGERSERVER
Q_OBJECT
#endif
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
    #endif
    std::vector<Reputation> reputation;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::unordered_map<uint16_t,Monster> monsters;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    bool monsterRateApplied;
    #endif
    uint16_t monstersMaxId;
    std::unordered_map<uint16_t,Skill> monsterSkills;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    ItemFull items;
    std::unordered_map<uint16_t,Industry> industries;
    std::unordered_map<uint16_t,IndustryLink> industriesLink;
    std::vector<MonstersCollision> monstersCollision;
    #endif
    std::vector<Profile> profileList;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::vector<Type> types;
    #endif

    std::unordered_map<std::string/*file*/,tinyxml2::XMLDocument> xmlLoadedFile;//keep for Map_loader::getXmlCondition(), need to be deleted later

    #ifndef CATCHCHALLENGER_CLASS_MASTER
    LayersOptions layersOptions;
    std::vector<Event> events;
    #endif
    std::vector<std::string > skins;
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
