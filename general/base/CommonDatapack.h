#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <QObject>
#include <QMutex>
#include <unordered_map>
#include <string>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QDomDocument>
#endif

#include "GeneralStructures.h"

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
    std::unordered_map<quint8,Plant> plants;
    std::unordered_map<quint16,CrafingRecipe> crafingRecipes;
    std::unordered_map<quint16,quint16> itemToCrafingRecipes;
    #endif
    std::vector<Reputation> reputation;
    std::unordered_map<quint16,Monster> monsters;
    std::unordered_map<quint16,Skill> monsterSkills;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::unordered_map<quint8,Buff> monsterBuffs;
    ItemFull items;
    std::unordered_map<quint16,Industry> industries;
    std::unordered_map<quint16,IndustryLink> industriesLink;
    std::vector<MonstersCollision> monstersCollision;
    #endif
    std::vector<Profile> profileList;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    std::vector<Type> types;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    std::unordered_map<std::string/*file*/, QDomDocument> xmlLoadedFile;
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    LayersOptions layersOptions;
    std::vector<Event> events;
    #endif
    std::vector<std::string > skins;
private:
    QMutex inProgress;
    bool isParsed;
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
