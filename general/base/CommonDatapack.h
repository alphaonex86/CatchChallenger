#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QMultiHash>

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
    QHash<quint8,Plant> plants;
    QHash<quint16,CrafingRecipe> crafingRecipes;
    QHash<quint16,quint16> itemToCrafingRecipes;
    QList<Reputation> reputation;
    QHash<quint16,Monster> monsters;
    QHash<quint16,Skill> monsterSkills;
    QHash<quint8,Buff> monsterBuffs;
    ItemFull items;
    QHash<quint16,Industry> industries;
    QHash<quint16,IndustryLink> industriesLink;
    QList<Profile> profileList;
    QList<Type> types;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QHash<QString/*file*/, QDomDocument> xmlLoadedFile;
    #endif
    LayersOptions layersOptions;
    QList<Event> events;
    QList<QString> skins;
private:
    QMutex inProgress;
    bool isParsed;
    QString datapackPath;
private:
    void parseTypes();
    void parseItems();
    void parseIndustries();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseEvents();
    void parseMonsters();
    void parseMonstersEvolutionItems();
    void parseMonstersItemToLearn();
    void parseReputation();
    void parseProfileList();
    void parseLayersOptions();
    void parseSkins();
};
}


#endif // COMMONDATAPACK_H
