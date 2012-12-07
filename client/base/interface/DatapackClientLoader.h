#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>
#include <QStringList>

#include "../../general/base/GeneralStructures.h"

#include "tileset.h"

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    void resetAll();

    //static items
    struct Item
    {
        QPixmap image;
        QString name;
        QString description;
    };
    QHash<quint32,Item> items;
    struct Plant
    {
        quint32 itemUsed;
        quint16 sprouted_seconds;
        quint16 taller_seconds;
        quint16 flowering_seconds;
        quint16 fruits_seconds;
        Tiled::Tileset * tileset;
    };
    QHash<quint8,Plant> plants;
    QHash<quint32,quint8> itemToPlants;
    QHash<quint32,Pokecraft::CrafingRecipe> crafingRecipes;
    QHash<quint32,quint32> itemToCrafingRecipes;

    QStringList maps;
    QPixmap defaultInventoryImage();
    static DatapackClientLoader datapackLoader;
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
};

#endif // DATAPACKCLIENTLOADER_H
