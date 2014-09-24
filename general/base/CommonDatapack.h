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
    QHash<quint16,BotFight> botFights;
    QHash<quint8,Plant> plants;
    QHash<quint16,CrafingRecipe> crafingRecipes;
    QHash<quint16,quint16> itemToCrafingRecipes;
    QList<Reputation> reputation;
    QHash<quint16,Quest> quests;
    QHash<quint16,Monster> monsters;
    QHash<quint16,Skill> monsterSkills;
    QHash<quint8,Buff> monsterBuffs;
    ItemFull items;
    QHash<quint16,Industry> industries;
    QHash<quint16,IndustryLink> industriesLink;
    QList<Profile> profileList;
    QList<Type> types;
    QHash<QString/*file*/, QDomDocument> xmlLoadedFile;
    QHash<QString/*file*/, QHash<quint32/*id*/,QDomElement> > teleportConditionsUnparsed;
    QList<MonstersCollision> monstersCollision;
    LayersOptions layersOptions;
    QList<Event> events;
    QList<QString> skins;
    QHash<quint32,Shop> shops;
private:
    QMutex inProgress;
    bool isParsed;
    QString datapackPath;
private:
    void parseTypes();
    void parseItems();
    void parseIndustries();
    void parseQuests();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseEvents();
    void parseMonsters();
    void parseMonstersEvolutionItems();
    void parseMonstersItemToLearn();
    void parseReputation();
    void parseBotFights();
    void parseProfileList();
    void parseMonstersCollision();
    void parseLayersOptions();
    void parseSkins();
    void parseShop();
};
}


#endif // COMMONDATAPACK_H
