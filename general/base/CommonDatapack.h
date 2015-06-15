#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QMultiHash>
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
    void parseDatapack(const QString &datapackPath);
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    QHash<quint8,Plant> plants;
    QHash<quint16,CrafingRecipe> crafingRecipes;
    QHash<quint16,quint16> itemToCrafingRecipes;
    #endif
    QList<Reputation> reputation;
    QHash<quint16,Monster> monsters;
    QHash<quint16,Skill> monsterSkills;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    QHash<quint8,Buff> monsterBuffs;
    ItemFull items;
    QHash<quint16,Industry> industries;
    QHash<quint16,IndustryLink> industriesLink;
    QList<MonstersCollision> monstersCollision;
    #endif
    QList<Profile> profileList;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    QList<Type> types;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QHash<QString/*file*/, QDomDocument> xmlLoadedFile;
    #endif
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    LayersOptions layersOptions;
    QList<Event> events;
    #endif
    QList<QString> skins;
private:
    QMutex inProgress;
    bool isParsed;
    QString datapackPath;
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
