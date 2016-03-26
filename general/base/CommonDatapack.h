#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <unordered_map>
#include <string>

#include "GeneralStructures.h"
#include "tinyXML/tinyxml.h"

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QDomDocument>
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
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::unordered_map<uint8_t,Plant> plants;
    std::unordered_map<uint16_t,CrafingRecipe> crafingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCrafingRecipes;
    #endif
    std::vector<Reputation> reputation;
    std::unordered_map<uint16_t,Monster> monsters;
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

    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::unordered_map<std::string/*file*/, TiXmlDocument> xmlLoadedFile;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::unordered_map<std::string/*file*/, QDomDocument> xmlLoadedFileQt;
    #endif
    #endif

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
