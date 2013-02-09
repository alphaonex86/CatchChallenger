#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>
#include <QStringList>

#include "../../general/base/GeneralStructures.h"
#include "../../fight/interface/FightEngine.h"

#include "tileset.h"

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    static DatapackClientLoader datapackLoader;
    void resetAll();

    //static items
    struct Item
    {
        QPixmap image;
        QString name;
        QString description;
        quint32 price;
    };
    QHash<quint32,Item> items;
    struct Plant
    {
        quint32 itemUsed;
        quint16 sprouted_seconds;
        quint16 taller_seconds;
        quint16 flowering_seconds;
        quint16 fruits_seconds;
        float quantity;
        Tiled::Tileset * tileset;
    };
    QHash<quint8,Plant> plants;
    QHash<quint32,quint8> itemToPlants;
    QHash<quint32,quint32> itemToCrafingRecipes;
    QHash<quint32,CatchChallenger::CrafingRecipe> crafingRecipes;
    QStringList maps;
    QPixmap defaultInventoryImage();
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
signals:
    void datapackParsed();
private:
    QString datapackPath;
    QPixmap *mDefaultInventoryImage;
    explicit DatapackClientLoader();
    ~DatapackClientLoader();
private slots:
    void parseItems();
    void parseMaps();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseMonsters();
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseSkillsExtra();
};

#endif // DATAPACKCLIENTLOADER_H
