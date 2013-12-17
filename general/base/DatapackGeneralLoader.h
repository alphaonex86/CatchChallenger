#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <QHash>
#include <QString>
#include <QPair>
#include <QList>
#include <QDomElement>
#include <QDomDocument>
#include "GeneralStructures.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
public:
    static QHash<QString, Reputation> loadReputation(const QString &file);
    static QHash<quint32, Quest> loadQuests(const QString &folder);
    static QPair<bool,Quest> loadSingleQuest(const QString &file);
    static QHash<quint8,Plant> loadPlants(const QString &file);
    static QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > loadCraftingRecipes(const QString &file, const QHash<quint32, Item> &items);
    static ItemFull loadItems(const QString &folder, const QHash<quint32, Buff> &monsterBuffs);
    static QHash<quint32,Industry> loadIndustries(const QString &folder,const QHash<quint32, Item> &items);
    static QHash<quint32,quint32> loadIndustriesLink(const QString &file,const QHash<quint32,Industry> &industries);
    static QPair<QList<QDomElement>, QList<Profile> > loadProfileList(const QString &datapackPath, const QString &file);
    static QHash<QString, QDomDocument> xmlLoadedFile;
};
}

#endif // DATAPACKGENERALLOADER_H
