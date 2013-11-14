#ifndef COMMONDATAPACK_H
#define COMMONDATAPACK_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QMultiHash>

#include "GeneralStructures.h"

namespace CatchChallenger {
class CommonDatapack : public QObject
{
    Q_OBJECT
public:
    explicit CommonDatapack();
    static CommonDatapack commonDatapack;
public slots:
    void unload();
    void parseDatapack(const QString &datapackPath);
public:
    QHash<quint32,BotFight> botFights;
    QHash<quint8,Plant> plants;
    QHash<quint32,CrafingRecipe> crafingRecipes;
    QHash<quint32,quint32> itemToCrafingRecipes;
    QHash<QString,Reputation> reputation;
    QHash<quint32,Quest> quests;
    QHash<quint32,Monster> monsters;
    QHash<quint32,Skill> monsterSkills;
    QHash<quint32,Buff> monsterBuffs;
    ItemFull items;
    QHash<quint32,Industry> industries;
    QHash<quint32,quint32> industriesLink;
    QList<Profile> profileList;
    QList<Type> types;
private:
    QMutex inProgress;
    bool isParsed;
    QString datapackPath;
private slots:
    void parseTypes();
    void parseItems();
    void parseIndustries();
    void parseQuests();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseMonsters();
    void parseReputation();
    void parseBotFights();
    void parseProfileList();
};
}


#endif // COMMONDATAPACK_H
