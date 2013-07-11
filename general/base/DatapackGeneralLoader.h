#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <QHash>
#include <QString>
#include "GeneralStructures.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
public:
    static QHash<QString, Reputation> loadReputation(const QString &file);
    static QHash<quint32, Quest> loadQuests(const QString &folder);
    static QHash<quint8,Plant> loadPlants(const QString &file);
    static QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > loadCraftingRecipes(const QString &file, const QHash<quint32, Item> &items);
    static QHash<quint32,Item> loadItems(const QString &folder);
};
}

#endif // DATAPACKGENERALLOADER_H
