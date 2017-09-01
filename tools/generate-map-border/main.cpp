#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QDomDocument>
#include <QDomElement>
#include <iostream>
#include <QDirIterator>

#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_layer.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_tile.h"

#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/base/LanguagesSelect.h"

#include "../../general/base/CommonDatapack.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/GeneralType.h"
#include "../../general/base/GeneralStructures.h"

/*pokemium or pokenet npc file structure:
[npc]
NULL//name, 0
down//orientation, 1
157//skin, 2
15//x, 3
11//y, 4
NULL//possible Pokemons, 5
0//PartySize, 6
-1//Badge, 7
1//all speech, 8
true//healer, 9
false//box, 10
false//shop, 11, false = 0 (none), true = 1, 2, 3, ...
[/npc]
[warp]
setX(Integer.parseInt(reader.nextLine()));
setY(Integer.parseInt(reader.nextLine()));
setWarpX(Integer.parseInt(reader.nextLine()) * 32);
setWarpY(Integer.parseInt(reader.nextLine()) * 32 - 8);
setWarpMapX(Integer.parseInt(reader.nextLine()));
setWarpMapY(Integer.parseInt(reader.nextLine()));
setBadgeRequirement(Integer.parseInt(reader.nextLine()));
[/warp]
[hmobject]
setName(reader.nextLine());
setType(HMObject.parseHMObject(hmObject.getName()));//CUT_TREE (Only this for now), HEADBUTT_TREE, ROCKSMASH_ROCK, STRENGTH_BOULDER, WHIRLPOOL
setX(Integer.parseInt(reader.nextLine()) * 32);
setOriginalX(hmObject.getX());
setY(Integer.parseInt(reader.nextLine()) * 32 - 8);
setOriginalY(hmObject.getY());
[/hmobject]

Todo:
not supported by catchchallenger for now:
[trade]
t.setName(reader.nextLine());
Direction
t.setSprite(Integer.parseInt(reader.nextLine()));
t.setX(Integer.parseInt(reader.nextLine()) * 32);
t.setY(Integer.parseInt(reader.nextLine()) * 32 - 8);
t.setRequestedPokemon(reader.nextLine(), Integer.parseInt(reader.nextLine()), reader.nextLine());
t.setOfferedSpecies(reader.nextLine(), Integer.parseInt(reader.nextLine()));
[/trade]
*/

//todo: use zopfli to improve the layer compression
//load the shop content via itemdex.xml
//split map part delimited with empty tile and adapt the tp (scan all the square)
//drop not used tileset

QHash<QString,int> mapWidth,mapHeight;
QHash<QString,int> xOffsetModifier,yOffsetModifier;
QHash<QString,int> mapX,mapY;
QHash<QString,int> monsterNameToMonsterId;
int botId=0;
int fightid=0;
QSet<QString> mapWithHeal;
unsigned int npcinicountValid=0;
QHash<QString,Tiled::Map *> mapLoaded;//keep in memory for intermap link management
Tiled::Tileset *tilesetinvisible=NULL;
Tiled::Tileset *tilesetnormal1=NULL;

struct BotDescriptor
{
    QString name;
    QString orientation;
    QString skin;
    int x;
    int y;
    QList<QMap<QString,QString> > text;
    QList<int> fightMonsterId;
    QList<int> fightMonsterLevel;
    unsigned int shopId;
};

struct FightDescriptor
{
    quint32 id;
    QMap<QString,QString> start;
    QMap<QString,QString> win;
    QList<int> fightMonsterId;
    QList<int> fightMonsterLevel;
};

enum WarpType
{
    WarpType_Door,
    WarpType_TpOnIt,
    WarpType_TpOnPush
};

struct WarpDescriptor
{
    int x;
    int y;
    int destX;
    int destY;
    QString destMap;
    WarpType type;
};

struct Shop
{
    QList<int> itemsId;
};
QMap<unsigned int,Shop> shopList;

QHash<QString,unsigned int> itemNameToId;

QStringList loadNPCText(const QString &npcFile,const QString &language);

bool haveBorderHole(const QString &file,const CatchChallenger::Orientation &orientation)
{
    const Tiled::Map * const map=mapLoaded.value(file);
    //detect the layer
    std::vector<unsigned int> indexLayerWalkable;
    int indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
        {
            Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
            if(tileLayer->name()=="Walkable")
                indexLayerWalkable.push_back(indexLayer);
        }
        indexLayer++;
    }
    if(indexLayerWalkable.empty())
    {
        indexLayer=0;
        while(indexLayer<map->layerCount())
        {
            if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
            {
                Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
                if(tileLayer->name()=="Grass")
                    indexLayerWalkable.push_back(indexLayer);
            }
            indexLayer++;
        }
    }
    indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
        {
            Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
            if(tileLayer->name()=="Water")
                indexLayerWalkable.push_back(indexLayer);
        }
        indexLayer++;
    }
    if(indexLayerWalkable.empty())
        return false;
    //detect the collisions
    std::vector<unsigned int> indexLayerCollisions;
    indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="Collisions")
            indexLayerCollisions.push_back(indexLayer);
        indexLayer++;
    }
    //test the tile line
    switch (orientation) {
    case CatchChallenger::Orientation_bottom:
    {
        const unsigned int y=map->height()-1;
        unsigned int x=0;
        while(x<(unsigned int)map->width())
        {
            unsigned int indexLayer=0;
            while(indexLayer<indexLayerWalkable.size())
            {
                if(!static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerWalkable.at(indexLayer)))->cellAt(x,y).isEmpty())
                    break;
                indexLayer++;
            }
            if(indexLayer<indexLayerWalkable.size())
            {
                indexLayer=0;
                while(indexLayer<indexLayerCollisions.size())
                {
                    const Tiled::TileLayer * const tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerCollisions.at(indexLayer)));
                    const Tiled::Cell &cell=tileLayer->cellAt(x,y);
                    if(!cell.isEmpty())
                        break;
                    indexLayer++;
                }
                if(indexLayer>=indexLayerCollisions.size())
                    return true;
            }
            x++;
        }
    }
    break;
    case CatchChallenger::Orientation_top:
    {
        const unsigned int y=0;
        unsigned int x=0;
        while(x<(unsigned int)map->width())
        {
            unsigned int indexLayer=0;
            while(indexLayer<indexLayerWalkable.size())
            {
                if(!static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerWalkable.at(indexLayer)))->cellAt(x,y).isEmpty())
                    break;
                indexLayer++;
            }
            if(indexLayer<indexLayerWalkable.size())
            {
                indexLayer=0;
                while(indexLayer<indexLayerCollisions.size())
                {
                    const Tiled::TileLayer * const tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerCollisions.at(indexLayer)));
                    const Tiled::Cell &cell=tileLayer->cellAt(x,y);
                    if(!cell.isEmpty())
                        break;
                    indexLayer++;
                }
                if(indexLayer>=indexLayerCollisions.size())
                    return true;
            }
            x++;
        }
    }
    break;
    case CatchChallenger::Orientation_left:
    {
        const unsigned int x=0;
        unsigned int y=0;
        while(y<(unsigned int)map->height())
        {
            unsigned int indexLayer=0;
            while(indexLayer<indexLayerWalkable.size())
            {
                if(!static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerWalkable.at(indexLayer)))->cellAt(x,y).isEmpty())
                    break;
                indexLayer++;
            }
            if(indexLayer<indexLayerWalkable.size())
            {
                indexLayer=0;
                while(indexLayer<indexLayerCollisions.size())
                {
                    const Tiled::TileLayer * const tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerCollisions.at(indexLayer)));
                    const Tiled::Cell &cell=tileLayer->cellAt(x,y);
                    if(!cell.isEmpty())
                        break;
                    indexLayer++;
                }
                if(indexLayer>=indexLayerCollisions.size())
                    return true;
            }
            y++;
        }
    }
    break;
    case CatchChallenger::Orientation_right:
    {
        const unsigned int x=map->width()-1;
        unsigned int y=0;
        while(y<(unsigned int)map->height())
        {
            unsigned int indexLayer=0;
            while(indexLayer<indexLayerWalkable.size())
            {
                if(!static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerWalkable.at(indexLayer)))->cellAt(x,y).isEmpty())
                    break;
                indexLayer++;
            }
            if(indexLayer<indexLayerWalkable.size())
            {
                indexLayer=0;
                while(indexLayer<indexLayerCollisions.size())
                {
                    const Tiled::TileLayer * const tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerCollisions.at(indexLayer)));
                    const Tiled::Cell &cell=tileLayer->cellAt(x,y);
                    if(!cell.isEmpty())
                        break;
                    indexLayer++;
                }
                if(indexLayer>=indexLayerCollisions.size())
                    return true;
            }
            y++;
        }
    }
    break;
    default:
        return false;
        break;
    }
    return false;
}

QString simplifyItemName(QString name)
{
    name.remove(' ');
    name.remove('-');
    name.remove('_');
    name=name.toUpper();
    return name;
}

//get the x/y offset modifier, heal point
int readMap(QString file)
{
    if(!QFile(file).exists())
        return 0;
    Tiled::MapReader reader;
    Tiled::Map *map=reader.readMap(file);
    if(map->layerCount()==0)
        abort();
    if(map==NULL)
    {
        qDebug() << "Can't read" << file << reader.errorString();
        return 86;
    }
    mapWidth[file]=map->width();
    mapHeight[file]=map->height();
    xOffsetModifier[file]=map->property("xOffsetModifier").toInt()/32;//*map->tileWidth(), 32 I don't known why, get from pokenet, mabe you have +-1 offset*/
    yOffsetModifier[file]=map->property("yOffsetModifier").toInt()/32;//map->tileHeight(), 32 I don't known why, get from pokenet, mabe you have +-1 offset*/

    bool healDetected=false;
    //detect if is heal map by map name
    {
        QString metaDataPath=file;
        metaDataPath.replace(".tmx",".xml");
        QFile metaDataFile(metaDataPath);
        if(metaDataFile.open(QIODevice::ReadOnly))
        {
            QString content=QString::fromUtf8(metaDataFile.readAll());
            if(content.contains("<name>Pokemon Center</name>"))
                healDetected=true;
            metaDataFile.close();
        }
        else
        {
            std::cerr << "You need start generate-xml-meta-data.php before to generated the metadata (" << metaDataPath.toStdString() << ")" << std::endl;
            abort();
        }
    }

    //detect if is heal map by bot text
    if(!healDetected)
    {
        QStringList textListEN;
        {
            QString npcFile=file;
            npcFile.replace(".tmx",".txt");
            textListEN=loadNPCText(npcFile,"english");

            unsigned int index=0;
            while(index<(unsigned int)textListEN.size())
            {
                const QString &englishText=textListEN.at(index);
                if(englishText.contains("Let me heal your Pokemon for you",Qt::CaseInsensitive))
                {
                    healDetected=true;
                    break;
                }
                index++;
            }
        }
        bool ok;
        if(!healDetected)
        {
            QString npcFile=file;
            npcFile.replace(".tmx",".txt");
            npcFile="../npc/"+npcFile;
            QFile tempFile(npcFile);
            if(tempFile.open(QIODevice::ReadOnly))
            {
                QStringList values;
                QString balise,baliseEnd;
                while (!tempFile.atEnd()) {
                    QString line=QString::fromUtf8(tempFile.readLine());
                    line.replace("\n","");
                    line.replace("\r","");
                    line.replace("\t","");
                    if(!baliseEnd.isEmpty() && baliseEnd==line)
                    {
                        if(balise=="[npc]")
                        {
                            if(values.count()==12)
                            {
                                BotDescriptor botDescriptor;
                                botDescriptor.name=values.at(0);
                                botDescriptor.orientation=values.at(1);
                                botDescriptor.skin=values.at(2);
                                botDescriptor.x=values.at(3).toInt(&ok);

                                if(botDescriptor.name=="Nurse Joy")
                                    healDetected=true;
                            }
                        }
                    }
                }
            }
        }
    }
    if(healDetected)
    {
        QString cleanFileName=file;
        cleanFileName.remove(".tmx");
        mapWithHeal.insert(cleanFileName);
    }

    mapLoaded[file]=map;
    {
        QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        Tiled::Tileset::preloadedTileset.clear();
    }
    return 0;
}

QStringList loadNPCText(const QString &npcFile,const QString &language)
{
    QStringList textList;
    QString filePath="../language/"+language+"/NPC/"+npcFile;
    QFile tempFile(filePath);
    if(tempFile.open(QIODevice::ReadOnly))
    {
        while (!tempFile.atEnd()) {
            QString line=QString::fromUtf8(tempFile.readLine());
            line.replace("\n","");
            line.replace("\r","");
            line.replace("\t","");
            //line.replace("Route ","Road ");
            line.replace(QRegularExpression(" +$"),"");
            textList << line;
        }
        tempFile.close();
    }
    /*else
        std::cerr << "File not found to open NPC: " << filePath.toStdString() << std::endl;*/
    return textList;
}

bool haveContent(const Tiled::Map * const map,const std::vector<unsigned int> &layerIndexes,const int x,const int y)
{
    unsigned int index=0;
    while(index<layerIndexes.size())
    {
        const Tiled::TileLayer * const tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(layerIndexes.at(index)));
        if(x<0 || x>=map->width())
            return false;
        if(y<0 || y>=map->height())
            return false;
        if(!tileLayer->cellAt(x,y).isEmpty())
            return true;
        index++;
    }
    return false;
}

int createBorder(QString file,const bool addOneToY)
{
    if(monsterNameToMonsterId.empty())
    {
        std::cerr << "monsterNameToMonsterId.empty()" << std::endl;
        abort();
    }

    unsigned int offsetToY=0;
    if(addOneToY)
        offsetToY=1;
    if(!QFile(file).exists())
    {
        std::cout << file.toStdString() << " file not exists" << std::endl;
        //return 0;
        abort();
    }

    Tiled::MapWriter write;
    Tiled::Map *map=mapLoaded.value(file);
    if(map==NULL)
    {
        qDebug() << "Can't open to create the border" << file;
        abort();//return 86;
    }
    QString xString=file;
    QString yString=file;
    xString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\1");
    yString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\2");
    int x=xString.toInt();
    int y=yString.toInt();

    {
        int index=0;
        while(index<map->layerCount())
        {
            if(map->layerAt(index)->isTileLayer())
                if(map->layerAt(index)->asTileLayer()->width()!=map->width() || map->layerAt(index)->asTileLayer()->height()!=map->height())
                    map->layerAt(index)->asTileLayer()->resize(QSize(map->width(),map->height()),QPoint(0,0));
            index++;
        }
    }
    //add the tileset if needed
    int indexTileset=0;
    while(indexTileset<map->tilesetCount())
    {
        const QString &imageSource=map->tilesetAt(indexTileset)->imageSource();
        if(imageSource.endsWith("invisible.tsx") || map->tilesetAt(indexTileset)->name()=="invisible.tsx")
            break;
        indexTileset++;
    }
    if(indexTileset>=map->tilesetCount())
    {
        indexTileset=map->tilesetCount();
        map->addTileset(tilesetinvisible);
        /*Tiled::Tileset *tileset=new Tiled::Tileset("invisible",map->tileWidth(),map->tileHeight());
        tileset->setFileName("invisible.tsx");*/
    }
    {
        const Tiled::Tile * const tile=map->tilesetAt(indexTileset)->tileAt(3);
        if(tile==NULL)
        {
            qDebug() << "Tile not found (" << __LINE__ << "), it's base tile, can't be missing";
            abort();
        }
    }

    //add the move layer if needed
    bool hadMovingLayer=false;
    unsigned int indexLayerMoving=0;
    while(indexLayerMoving<(unsigned int)map->layerCount())
    {
        if(map->layerAt(indexLayerMoving)->isObjectGroup() && map->layerAt(indexLayerMoving)->name()=="Moving")
        {
            hadMovingLayer=true;
            break;
        }
        indexLayerMoving++;
    }
    if(indexLayerMoving>=(unsigned int)map->layerCount())
    {
        indexLayerMoving=map->layerCount();
        Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Moving",0,0,map->width(),map->height());
        map->addLayer(objectGroup);
    }

    //locate grass layer
    std::vector<unsigned int> indexLayerWalkable;
    int indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="Walkable")
            indexLayerWalkable.push_back(indexLayer);
        indexLayer++;
    }
    if(indexLayerWalkable.empty())
    {
        int indexLayer=0;
        while(indexLayer<map->layerCount())
        {
            if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="Grass")
            {
                indexLayerWalkable.push_back(indexLayer);
                map->layerAt(indexLayer)->setName("Walkable");
            }
            indexLayer++;
        }
        if(indexLayerWalkable.empty())
        {
            int indexLayer=0;
            while(indexLayer<map->layerCount())
            {
                if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="Water")
                    indexLayerWalkable.push_back(indexLayer);
                indexLayer++;
            }
            if(indexLayerWalkable.empty())
                abort();
        }
    }
    std::vector<unsigned int> indexLayerCollisions;
    indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="Collisions")
            indexLayerCollisions.push_back(indexLayer);
        indexLayer++;
    }
    std::vector<unsigned int> indexLayerWalkBehind;
    indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->isTileLayer() && map->layerAt(indexLayer)->name()=="WalkBehind")
            indexLayerWalkBehind.push_back(indexLayer);
        indexLayer++;
    }

    if(!hadMovingLayer)
        if(x<=30 && x>=-30 && y<=30 && y>=-30)
        {
            QString mapBorderFile;
            //check the left map
            mapBorderFile=QStringLiteral("%1.%2.tmx").arg(x-1).arg(y);
            if(mapHeight.contains(mapBorderFile))
            {
                if(false && mapX.contains(file) && mapX.contains(mapBorderFile))
                {
                }
                else
                {
                    int topBorder=0;
                    int bottomBorder=0;
                    const int offset=yOffsetModifier.value(mapBorderFile)-yOffsetModifier.value(file);
                    const int mapBorderHeight=mapHeight.value(mapBorderFile);
                    if(offset<0)
                    {
                        if(-offset<=mapBorderHeight)
                        {
                            topBorder=0;
                            if((mapBorderHeight-(-offset))<=map->height())
                                bottomBorder=mapBorderHeight-(-offset);
                            else
                                bottomBorder=map->height();
                        }
                    }
                    else
                    {
                        if(offset<map->height())
                        {
                            topBorder=offset;
                            if((map->height()-offset)<=mapBorderHeight)
                                bottomBorder=map->height();
                            else
                                bottomBorder=mapBorderHeight+offset;
                        }
                    }
                    if(topBorder!=0 || bottomBorder!=0)
                    {
                        QPointF point(0,(bottomBorder-topBorder)/2+topBorder+offsetToY);
                        if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        {
                            if(haveBorderHole(file,CatchChallenger::Orientation_left) && haveBorderHole(mapBorderFile,CatchChallenger::Orientation_right))
                            {
                                Tiled::MapObject *mapObject=new Tiled::MapObject("","border-left",point,QSizeF(1,1));
                                QString mapBorderFileClean=mapBorderFile;
                                mapBorderFileClean.remove(".tmx");
                                mapObject->setProperty("map",mapBorderFileClean);
                                Tiled::Cell cell=mapObject->cell();
                                cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                                if(cell.tile==NULL)
                                    qDebug() << "Tile not found (" << __LINE__ << ")";
                                mapObject->setCell(cell);
                                map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                            }
                        }
                        else
                            std::cerr << "For " << file.toStdString() << " border left is out of range" << std::endl;
                    }
                    else
                        std::cerr << "For " << file.toStdString() << " border left detected is out of range" << std::endl;
                }
            }
            //check the right map
            mapBorderFile=QStringLiteral("%1.%2.tmx").arg(x+1).arg(y);
            if(mapHeight.contains(mapBorderFile))
            {
                if(false && mapX.contains(file) && mapX.contains(mapBorderFile))
                {
                }
                else
                {
                    int topBorder=0;
                    int bottomBorder=0;
                    const int offset=yOffsetModifier.value(mapBorderFile)-yOffsetModifier.value(file);
                    const int mapBorderHeight=mapHeight.value(mapBorderFile);
                    if(offset<0)
                    {
                        if(-offset<=mapBorderHeight)
                        {
                            topBorder=0;
                            if((mapBorderHeight-(-offset))<=map->height())
                                bottomBorder=mapBorderHeight-(-offset);
                            else
                                bottomBorder=map->height();
                        }
                    }
                    else
                    {
                        if(offset<map->height())
                        {
                            topBorder=offset;
                            if((mapBorderHeight+offset)<=map->height())
                                bottomBorder=mapBorderHeight+offset;
                            else
                                bottomBorder=map->height();
                        }
                    }
                    if(topBorder!=0 || bottomBorder!=0)
                    {
                        QPointF point(map->width()-1,(bottomBorder-topBorder)/2+topBorder+offsetToY);
                        if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        {
                            if(haveBorderHole(file,CatchChallenger::Orientation_right) && haveBorderHole(mapBorderFile,CatchChallenger::Orientation_left))
                            {
                                Tiled::MapObject *mapObject=new Tiled::MapObject("","border-right",point,QSizeF(1,1));
                                QString mapBorderFileClean=mapBorderFile;
                                mapBorderFileClean.remove(".tmx");
                                mapObject->setProperty("map",mapBorderFileClean);
                                Tiled::Cell cell=mapObject->cell();
                                cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                                if(cell.tile==NULL)
                                    qDebug() << "Tile not found (" << __LINE__ << ")";
                                mapObject->setCell(cell);
                                map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                            }
                        }
                        else
                            std::cerr << "For " << file.toStdString() << " border right is out of range" << std::endl;
                    }
                    else
                        std::cerr << "For " << file.toStdString() << " border right detected is out of range" << std::endl;
                }
            }
            //check the top map
            mapBorderFile=QStringLiteral("%1.%2.tmx").arg(x).arg(y-1);
            if(mapWidth.contains(mapBorderFile))
            {
                if(false && mapX.contains(file) && mapX.contains(mapBorderFile))
                {
                }
                else
                {
                    int leftBorder=0;
                    int rightBorder=0;
                    const int offset=xOffsetModifier.value(file)-xOffsetModifier.value(mapBorderFile);
                    const int mapBorderWidth=mapWidth.value(mapBorderFile);
                    if(offset<0)
                    {
                        if(-offset<=map->width())
                        {
                            leftBorder=-offset;
                            if((-offset+mapBorderWidth)<=map->width())
                                rightBorder=mapBorderWidth+(-offset);
                            else
                                rightBorder=map->width();
                        }
                    }
                    else
                    {
                        if(offset<mapBorderWidth)
                        {
                            leftBorder=0;
                            if((mapBorderWidth-offset)<=map->width())
                                rightBorder=mapBorderWidth-offset;
                            else
                                rightBorder=map->width();
                        }
                    }
                    if(leftBorder!=0 || rightBorder!=0)
                    {
                        QPointF point((rightBorder-leftBorder)/2+leftBorder,0+offsetToY);
                        if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        {
                            if(haveBorderHole(file,CatchChallenger::Orientation_top) && haveBorderHole(mapBorderFile,CatchChallenger::Orientation_bottom))
                            {
                                Tiled::MapObject *mapObject=new Tiled::MapObject("","border-top",point,QSizeF(1,1));
                                QString mapBorderFileClean=mapBorderFile;
                                mapBorderFileClean.remove(".tmx");
                                mapObject->setProperty("map",mapBorderFileClean);
                                Tiled::Cell cell=mapObject->cell();
                                cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                                if(cell.tile==NULL)
                                    qDebug() << "Tile not found (" << __LINE__ << ")";
                                mapObject->setCell(cell);
                                map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                            }
                        }
                        else
                            std::cerr << "For " << file.toStdString() << " border top is out of range" << std::endl;
                    }
                    else
                        std::cerr << "For " << file.toStdString() << " border top is detected out of range" << std::endl;
                }
            }
            //check the bottom map
            mapBorderFile=QStringLiteral("%1.%2.tmx").arg(x).arg(y+1);
            if(mapWidth.contains(mapBorderFile))
            {
                if(false && mapX.contains(file) && mapX.contains(mapBorderFile))
                {
                }
                else
                {
                    int leftBorder=0;
                    int rightBorder=0;
                    const int offset=xOffsetModifier.value(file)-xOffsetModifier.value(mapBorderFile);
                    const int mapBorderWidth=mapWidth.value(mapBorderFile);
                    if(offset<0)
                    {
                        if(-offset<=map->width())
                        {
                            leftBorder=-offset;
                            if((-offset+mapBorderWidth)<=map->width())
                                rightBorder=mapBorderWidth+(-offset);
                            else
                                rightBorder=map->width();
                        }
                    }
                    else
                    {
                        if(offset<mapBorderWidth)
                        {
                            leftBorder=0;
                            if((map->width()+offset)<=map->width())
                                rightBorder=map->width()+offset;
                            else
                                rightBorder=map->width();
                        }
                    }
                    if(leftBorder!=0 || rightBorder!=0)
                    {
                        QPointF point((rightBorder-leftBorder)/2+leftBorder,map->height()-1+offsetToY);
                        if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        {
                            if(haveBorderHole(file,CatchChallenger::Orientation_bottom) && haveBorderHole(mapBorderFile,CatchChallenger::Orientation_top))
                            {
                                Tiled::MapObject *mapObject=new Tiled::MapObject("","border-bottom",point,QSizeF(1,1));
                                QString mapBorderFileClean=mapBorderFile;
                                mapBorderFileClean.remove(".tmx");
                                mapObject->setProperty("map",mapBorderFileClean);
                                Tiled::Cell cell=mapObject->cell();
                                cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                                if(cell.tile==NULL)
                                    qDebug() << "Tile not found (" << __LINE__ << ")";
                                mapObject->setCell(cell);
                                map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                            }
                        }
                        else
                            std::cerr << "For " << file.toStdString() << " border bottom is out of range" << std::endl;
                    }
                    else
                        std::cerr << "For " << file.toStdString() << " border bottom is detected out of range" << std::endl;
                }
            }
        }

    QList<BotDescriptor> botList;
    QMap<QPair<int,int>,WarpDescriptor> warpList;
    QList<FightDescriptor> fightList;
    QStringList textListNL,textListEN,textListFI,textListFR,textListDE,textListIT,textListPT,textListES;
    {
        QString npcFile=file;
        npcFile.replace(".tmx",".txt");
        textListNL=loadNPCText(npcFile,"dutch");
        textListEN=loadNPCText(npcFile,"english");
        textListFI=loadNPCText(npcFile,"finnish");
        textListFR=loadNPCText(npcFile,"french");
        textListDE=loadNPCText(npcFile,"german");
        textListIT=loadNPCText(npcFile,"italian");
        textListPT=loadNPCText(npcFile,"portuguese");
        textListES=loadNPCText(npcFile,"spanish");

        if(textListNL.size()!=textListEN.size())
        {
            //std::cerr << "NL size not match for translation, unload" << std::endl;
            textListNL=textListEN;
        }
        if(textListFI.size()!=textListEN.size())
        {
            //std::cerr << "FI size not match for translation, unload" << std::endl;
            textListFI=textListEN;
        }
        if(textListFR.size()!=textListEN.size())
        {
            //std::cerr << "FR size not match for translation, unload" << std::endl;
            textListFR=textListEN;
        }
        if(textListDE.size()!=textListEN.size())
        {
            //std::cerr << "DE size not match for translation, unload" << std::endl;
            textListDE=textListEN;
        }
        if(textListIT.size()!=textListEN.size())
        {
            //std::cerr << "IT size not match for translation, unload" << std::endl;
            textListIT=textListEN;
        }
        if(textListPT.size()!=textListEN.size())
        {
            //std::cerr << "PT size not match for translation, unload" << std::endl;
            textListPT=textListEN;
        }
        if(textListES.size()!=textListEN.size())
        {
            //std::cerr << "NL size not match for translation, unload" << std::endl;
            textListES=textListEN;
        }
    }
    bool ok;
    QSet<QPair<unsigned int,unsigned int> > dropBot;
    {
        QString npcFile=file;
        npcFile.replace(".tmx",".txt");
        npcFile="../npc/"+npcFile;
        QFile tempFile(npcFile);
        if(tempFile.open(QIODevice::ReadOnly))
        {
            //load raw bot on coord
            QHash<QPair<unsigned int,unsigned int>,BotDescriptor> rawBotDescriptorbyCoord;
            {
                QStringList values;
                QString balise,baliseEnd;
                while (!tempFile.atEnd()) {
                    QString line=QString::fromUtf8(tempFile.readLine());
                    line.replace("\n","");
                    line.replace("\r","");
                    line.replace("\t","");
                    if(!baliseEnd.isEmpty() && baliseEnd==line)
                    {
                        if(balise=="[warp]")
                        {}
                        else if(balise=="[hmobject]")
                        {}
                        else if(balise=="[npc]")
                        {
                            if(values.count()==12)
                            {
                                BotDescriptor botDescriptor;
                                botDescriptor.name=values.at(0);
                                botDescriptor.orientation=values.at(1);
                                botDescriptor.skin=values.at(2);
                                botDescriptor.x=values.at(3).toInt(&ok);

                                if(!ok)
                                {
                                    continue;
                                    values.clear();
                                }
                                botDescriptor.y=values.at(4).toInt(&ok);
                                if(!ok)
                                {
                                    continue;
                                    values.clear();
                                }
                                QStringList textIndex=values.at(8).split(",");
                                int index=0;
                                while(index<textIndex.size())
                                {
                                    bool ok;
                                    quint32 tempIndex=textIndex.at(index).toUInt(&ok);
                                    if(!ok)
                                        break;
                                    if((int)tempIndex>=textListEN.size())
                                        break;
                                    QMap<QString,QString> fullTextList;
                                    fullTextList["nl"]=textListNL.at(tempIndex);
                                    fullTextList["en"]=textListEN.at(tempIndex);
                                    fullTextList["fi"]=textListFI.at(tempIndex);
                                    fullTextList["fr"]=textListFR.at(tempIndex);
                                    fullTextList["de"]=textListDE.at(tempIndex);
                                    fullTextList["it"]=textListIT.at(tempIndex);
                                    fullTextList["pt"]=textListPT.at(tempIndex);
                                    fullTextList["es"]=textListES.at(tempIndex);
                                    botDescriptor.text << fullTextList;
                                    index++;
                                }

                                botDescriptor.shopId=0;
                                if(values.at(11).toUpper()=="TRUE")
                                    botDescriptor.shopId=1;
                                else
                                {
                                    unsigned int tempShopId=values.at(11).toUInt(&ok);
                                    if(ok)
                                        botDescriptor.shopId=tempShopId;
                                }

                                rawBotDescriptorbyCoord[QPair<unsigned int,unsigned int>(botDescriptor.x,botDescriptor.y)]=botDescriptor;
                            }
                            else
                                qDebug() << file << "wrong npc count values: " << values.count() << ": " << values.join(", ");
                        }
                        values.clear();
                        balise.clear();
                        baliseEnd.clear();
                    }
                    else if(!balise.isEmpty())
                        values << line;
                    else if(line.contains(QRegularExpression("^\\[[a-z]+\\]$")))
                    {
                        balise=line;
                        baliseEnd=balise;
                        baliseEnd.replace("[","[/");
                    }
                }
            }

            //load the final values
            tempFile.seek(0);
            QStringList values;
            QString balise,baliseEnd;
            while (!tempFile.atEnd()) {
                QString line=QString::fromUtf8(tempFile.readLine());
                line.replace("\n","");
                line.replace("\r","");
                line.replace("\t","");
                line.replace("] ","]");
                if(!baliseEnd.isEmpty() && baliseEnd==line)
                {
                    if(balise=="[npc]")
                    {
                        if(values.count()==12)
                        {
                            BotDescriptor botDescriptor;
                            botDescriptor.name=values.at(0);
                            botDescriptor.orientation=values.at(1);
                            botDescriptor.skin=values.at(2);
                            botDescriptor.shopId=0;
                            botDescriptor.x=values.at(3).toInt(&ok);

                            if(!ok)
                            {
                                continue;
                                values.clear();
                            }
                            botDescriptor.y=values.at(4).toInt(&ok);
                            if(!ok)
                            {
                                continue;
                                values.clear();
                            }
                            QStringList textIndex=values.at(8).split(",");
                            int index=0;
                            while(index<textIndex.size())
                            {
                                bool ok;
                                quint32 tempIndex=textIndex.at(index).toUInt(&ok);
                                if(!ok)
                                    break;
                                if((int)tempIndex>=textListEN.size())
                                    break;
                                QMap<QString,QString> fullTextList;
                                fullTextList["nl"]=textListNL.at(tempIndex);
                                fullTextList["en"]=textListEN.at(tempIndex);
                                fullTextList["fi"]=textListFI.at(tempIndex);
                                fullTextList["fr"]=textListFR.at(tempIndex);
                                fullTextList["de"]=textListDE.at(tempIndex);
                                fullTextList["it"]=textListIT.at(tempIndex);
                                fullTextList["pt"]=textListPT.at(tempIndex);
                                fullTextList["es"]=textListES.at(tempIndex);
                                botDescriptor.text << fullTextList;
                                index++;
                            }

                            //try detect one near valid bot and clone some information
                            if(!botDescriptor.text.isEmpty() && botDescriptor.text.at(0).value("en")=="Please report this bug!")
                            {
                                if(botDescriptor.x>0)
                                {
                                    const unsigned int newX=botDescriptor.x-1;
                                    const unsigned int newY=botDescriptor.y;
                                    if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                    {
                                        const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                        botDescriptor.text=nearBotDescriptor.text;
                                        if(botDescriptor.name.isEmpty() && !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            botDescriptor.name=nearBotDescriptor.name;
                                        if(nearBotDescriptor.shopId!=0)
                                            botDescriptor.shopId=nearBotDescriptor.shopId;
                                        if(botDescriptor.name.contains("Shop Keeper") && botDescriptor.shopId==0)
                                            std::cerr << "1) botDescriptor.name.contains(\"Shop Keeper\") && botDescriptor.shopId==0" << std::endl;
                                        if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                        {}
                                        else
                                            dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                    }
                                }
                                if(botDescriptor.y>0)
                                {
                                    const unsigned int newX=botDescriptor.x;
                                    const unsigned int newY=botDescriptor.y-1;
                                    if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                    {
                                        const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                        botDescriptor.text=nearBotDescriptor.text;
                                        if(botDescriptor.name.isEmpty() && !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            botDescriptor.name=nearBotDescriptor.name;
                                        if(nearBotDescriptor.shopId!=0)
                                            botDescriptor.shopId=nearBotDescriptor.shopId;
                                        if(botDescriptor.name.contains("Shop Keeper") && botDescriptor.shopId==0)
                                            std::cerr << "1) botDescriptor.name.contains(\"Shop Keeper\") && botDescriptor.shopId==0" << std::endl;
                                        if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                        {}
                                        else
                                            dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                    }
                                }
                                if(botDescriptor.x<(map->width()-1))
                                {
                                    const unsigned int newX=botDescriptor.x+1;
                                    const unsigned int newY=botDescriptor.y;
                                    if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                    {
                                        const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                        botDescriptor.text=nearBotDescriptor.text;
                                        if(botDescriptor.name.isEmpty() && !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            botDescriptor.name=nearBotDescriptor.name;
                                        if(nearBotDescriptor.shopId!=0)
                                            botDescriptor.shopId=nearBotDescriptor.shopId;
                                        if(botDescriptor.name.contains("Shop Keeper") && botDescriptor.shopId==0)
                                            std::cerr << "1) botDescriptor.name.contains(\"Shop Keeper\") && botDescriptor.shopId==0" << std::endl;
                                        if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                        {}
                                        else
                                            dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                    }
                                }
                                if(botDescriptor.y<(map->height()-1))
                                {
                                    const unsigned int newX=botDescriptor.x;
                                    const unsigned int newY=botDescriptor.y+1;
                                    if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                    {
                                        const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                        botDescriptor.text=nearBotDescriptor.text;
                                        if(botDescriptor.name.isEmpty() && !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            botDescriptor.name=nearBotDescriptor.name;
                                        if(nearBotDescriptor.shopId!=0)
                                            botDescriptor.shopId=nearBotDescriptor.shopId;
                                        if(botDescriptor.name.contains("Shop Keeper") && botDescriptor.shopId==0)
                                            std::cerr << "1) botDescriptor.name.contains(\"Shop Keeper\") && botDescriptor.shopId==0" << std::endl;
                                        if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                        {}
                                        else
                                            dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                    }
                                }
                            }

                            //clean neightbors
                            if(botDescriptor.x>0)
                            {
                                const unsigned int newX=botDescriptor.x-1;
                                const unsigned int newY=botDescriptor.y;
                                if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                {
                                    const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                    if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                            !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                    {}
                                    else
                                        dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                }
                            }
                            if(botDescriptor.y>0)
                            {
                                const unsigned int newX=botDescriptor.x;
                                const unsigned int newY=botDescriptor.y-1;
                                if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                {
                                    const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                    if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                            !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                    {}
                                    else
                                        dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                }
                            }
                            if(botDescriptor.x<(map->width()-1))
                            {
                                const unsigned int newX=botDescriptor.x+1;
                                const unsigned int newY=botDescriptor.y;
                                if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                {
                                    const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                    if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                            !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                    {}
                                    else
                                        dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                }
                            }
                            if(botDescriptor.y<(map->height()-1))
                            {
                                const unsigned int newX=botDescriptor.x;
                                const unsigned int newY=botDescriptor.y+1;
                                if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                {
                                    const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                    if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                            !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                    {}
                                    else
                                        dropBot.insert(QPair<unsigned int,unsigned int>(newX,newY));
                                }
                            }

                            //add heal, warehouse step, shop
                            {
                                QMap<QString,QString> fullTextListHeal;
                                fullTextListHeal["nl"]="heal";
                                fullTextListHeal["en"]="heal";
                                fullTextListHeal["fi"]="heal";
                                fullTextListHeal["fr"]="heal";
                                fullTextListHeal["de"]="heal";
                                fullTextListHeal["it"]="heal";
                                fullTextListHeal["pt"]="heal";
                                fullTextListHeal["es"]="heal";
                                QMap<QString,QString> fullTextListWarehouse;
                                fullTextListWarehouse["nl"]="warehouse";
                                fullTextListWarehouse["en"]="warehouse";
                                fullTextListWarehouse["fi"]="warehouse";
                                fullTextListWarehouse["fr"]="warehouse";
                                fullTextListWarehouse["de"]="warehouse";
                                fullTextListWarehouse["it"]="warehouse";
                                fullTextListWarehouse["pt"]="warehouse";
                                fullTextListWarehouse["es"]="warehouse";

                                bool healAdded=false,warehouseAdded=false;
                                unsigned int index=0;
                                while(index<(unsigned int)botDescriptor.text.size())
                                {
                                    const QString englishText=botDescriptor.text.at(index).value("en");
                                    if(englishText.contains("Let me heal your Pokemon for you",Qt::CaseInsensitive))
                                    {
                                        botDescriptor.text.insert(index+1,fullTextListHeal);
                                        healAdded=true;
                                        index++;
                                    }
                                    if(englishText.contains("Welcome to the Pokemon storage system",Qt::CaseInsensitive))
                                    {
                                        botDescriptor.text.insert(index+1,fullTextListWarehouse);
                                        warehouseAdded=true;
                                        index++;
                                    }
                                    index++;
                                }
                                if(!healAdded && botDescriptor.name=="Nurse Joy" && !botDescriptor.text.empty())
                                {
                                    botDescriptor.text.insert(index+1,fullTextListHeal);
                                    healAdded=true;
                                }
                                if(!warehouseAdded && botDescriptor.name=="Pokemon storage system" && !botDescriptor.text.empty())
                                {
                                    botDescriptor.text.insert(index+1,fullTextListWarehouse);
                                    warehouseAdded=true;
                                }
                                if(!healAdded && values.at(9).toUpper()=="TRUE")
                                {
                                    botDescriptor.text.insert(index+1,fullTextListHeal);
                                    healAdded=true;
                                }

                                botDescriptor.shopId=0;
                                if(values.at(11).toUpper()=="TRUE")
                                    botDescriptor.shopId=1;
                                else
                                {
                                    unsigned int tempShopId=values.at(11).toUInt(&ok);
                                    if(ok)
                                        botDescriptor.shopId=tempShopId;
                                }

                                if(botDescriptor.shopId==0)
                                {
                                    if(botDescriptor.x>0)
                                    {
                                        const unsigned int newX=botDescriptor.x-1;
                                        const unsigned int newY=botDescriptor.y;
                                        if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                        {
                                            const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                            if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                    !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            {}
                                            else
                                                if(nearBotDescriptor.shopId!=0)
                                                    botDescriptor.shopId=nearBotDescriptor.shopId;
                                        }
                                    }
                                    if(botDescriptor.y>0)
                                    {
                                        const unsigned int newX=botDescriptor.x;
                                        const unsigned int newY=botDescriptor.y-1;
                                        if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                        {
                                            const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                            if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                    !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            {}
                                            else
                                                if(nearBotDescriptor.shopId!=0)
                                                    botDescriptor.shopId=nearBotDescriptor.shopId;
                                        }
                                    }
                                    if(botDescriptor.x<(map->width()-1))
                                    {
                                        const unsigned int newX=botDescriptor.x+1;
                                        const unsigned int newY=botDescriptor.y;
                                        if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                        {
                                            const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                            if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                    !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            {}
                                            else
                                                if(nearBotDescriptor.shopId!=0)
                                                    botDescriptor.shopId=nearBotDescriptor.shopId;
                                        }
                                    }
                                    if(botDescriptor.y<(map->height()-1))
                                    {
                                        const unsigned int newX=botDescriptor.x;
                                        const unsigned int newY=botDescriptor.y+1;
                                        if(rawBotDescriptorbyCoord.contains(QPair<unsigned int,unsigned int>(newX,newY)))
                                        {
                                            const BotDescriptor &nearBotDescriptor=rawBotDescriptorbyCoord.value(QPair<unsigned int,unsigned int>(newX,newY));
                                            if(nearBotDescriptor.skin!="" && nearBotDescriptor.skin!="NULL" && nearBotDescriptor.skin!="0" &&
                                                    !nearBotDescriptor.name.isEmpty() && nearBotDescriptor.name!="NULL")
                                            {}
                                            else
                                                if(nearBotDescriptor.shopId!=0)
                                                    botDescriptor.shopId=nearBotDescriptor.shopId;
                                        }
                                    }
                                }
                            }

                            QStringList textIndexMonster=values.at(5).split(",");
                            if(textIndexMonster.size()>0)
                            {
                                if((textIndexMonster.size()%2)==0)
                                {
                                    int fightIndex=0;
                                    while(fightIndex<textIndexMonster.size())
                                    {
                                        const QString monsterString=simplifyItemName(textIndexMonster.at(fightIndex));
                                        if(monsterNameToMonsterId.contains(monsterString))
                                        {
                                            bool ok;
                                            const quint32 level=textIndexMonster.at(fightIndex+1).toUInt(&ok);
                                            const uint32_t monsterId=monsterNameToMonsterId.value(monsterString);
                                            if(ok)
                                            {
                                                botDescriptor.fightMonsterId << monsterId;
                                                if(monsterId==0)
                                                {
                                                    std::cerr << "monsterId==0 for bot monster!" << std::endl;
                                                    abort();
                                                }
                                                botDescriptor.fightMonsterLevel << level;
                                                if(level==0)
                                                {
                                                    std::cerr << "level==0 for bot monster!" << std::endl;
                                                    abort();
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "!monsterNameToMonsterId.contains(" << textIndexMonster.at(fightIndex).toStdString() << ")" << std::endl;
                                        fightIndex+=2;
                                    }
                                }
                                if(index==textIndex.size())
                                {
                                    if(values.at(0)=="NULL")
                                    {
                                        botDescriptor.name=QString();
                                        botDescriptor.skin=QString();
                                        botDescriptor.orientation=QString();
                                    }
                                    if(!botDescriptor.text.isEmpty() && botDescriptor.text.at(0).value("en")!="Please report this bug!")
                                        botList << botDescriptor;
                                }
                            }
                        }
                        else
                            qDebug() << file << "wrong npc count values";
                    }
                    else if(balise=="[warp]")
                    {
                        if(values.count()>5)
                        {
                            WarpDescriptor warpDescriptor;
                            warpDescriptor.x=values.at(0).toInt(&ok);
                            if(!ok)
                                continue;
                            warpDescriptor.y=values.at(1).toInt(&ok);
                            if(!ok)
                                continue;
                            warpDescriptor.destX=values.at(2).toInt(&ok);
                            if(!ok)
                                continue;
                            warpDescriptor.destY=values.at(3).toInt(&ok);
                            if(!ok)
                                continue;
                            warpDescriptor.destMap=QStringLiteral("%1.%2").arg(values.at(4)).arg(values.at(5));
                            const bool isWalkable=haveContent(map,indexLayerWalkable,warpDescriptor.x,warpDescriptor.y);
                            const bool isColision=haveContent(map,indexLayerCollisions,warpDescriptor.x,warpDescriptor.y);
                            bool haveTopTileWalkBehind=false;
                            if(warpDescriptor.y>0)
                                haveTopTileWalkBehind=haveContent(map,indexLayerWalkBehind,warpDescriptor.x,warpDescriptor.y-1);
                            if(isWalkable && !isColision)
                            {
                                if(haveTopTileWalkBehind)//just pgreen do that's
                                    warpDescriptor.type=WarpType_Door;
                                else
                                    warpDescriptor.type=WarpType_TpOnIt;
                            }
                            else
                                warpDescriptor.type=WarpType_TpOnPush;

                            //add to the list
                            if(!warpList.contains(QPair<int,int>(warpDescriptor.x,warpDescriptor.y)))
                                warpList[QPair<int,int>(warpDescriptor.x,warpDescriptor.y)]=warpDescriptor;
                        }
                        else
                            qDebug() << file << "wrong warp count values: " << values.count() << ": " << values.join(", ");
                    }
                    else if(balise=="[hmobject]")
                    {
                        const QString &type=values.at(0);
                        const unsigned int x=values.at(1).toInt(&ok);
                        if(!ok)
                            continue;
                        const unsigned int y=values.at(2).toInt(&ok);
                        if(!ok)
                            continue;
                        if(type=="CUT_TREE" || type=="HEADBUTT_TREE" || type=="ROCKSMASH_ROCK" || type=="WHIRLPOOL")
                        {
                            QString tileLayerName;
                            QString tilesetName;
                            unsigned int tiledIndex;
                            if(type=="CUT_TREE")
                            {
                                tileLayerName="CutTree";
                                tilesetName="normal1";
                                tiledIndex=33;
                            }
                            else if(type=="HEADBUTT_TREE")
                            {
                                tileLayerName="HeadButtTree";
                                tilesetName="normal1";
                                tiledIndex=34;
                            }
                            else if(type=="ROCKSMASH_ROCK")
                            {
                                tileLayerName="RockMash";
                                tilesetName="normal1";
                                tiledIndex=35;
                            }
                            else if(type=="WHIRLPOOL")
                            {
                                tileLayerName="WhirlPool";
                                tilesetName="normal16";
                                tiledIndex=428;
                            }
                            else
                            {
                                std::cerr << "unknown [hmobject] type for tile mode" << std::endl;
                                abort();
                            }
                            //add tile layer
                            int indexLayerCutTree=0;
                            while(indexLayerCutTree<map->layerCount())
                            {
                                if(map->layerAt(indexLayerCutTree)->isTileLayer() && map->layerAt(indexLayerCutTree)->name()==tileLayerName)
                                    break;
                                indexLayerCutTree++;
                            }
                            if(indexLayerCutTree>=map->layerCount())
                            {
                                Tiled::TileLayer *tileLayer=new Tiled::TileLayer(tileLayerName,0,0,map->width(),map->height());
                                indexLayerCutTree=map->layerCount();
                                map->addLayer(tileLayer);
                            }
                            //add tileset
                            int indexTileset=0;
                            while(indexTileset<map->tilesetCount())
                            {
                                const QString &imageSource=map->tilesetAt(indexTileset)->imageSource();
                                if(imageSource.endsWith(tilesetName+".tsx") || map->tilesetAt(indexTileset)->name()==tilesetName+".tsx" || map->tilesetAt(indexTileset)->name()==tilesetName)
                                    break;
                                indexTileset++;
                            }
                            if(indexTileset>=map->tilesetCount())
                            {
                                indexTileset=map->tilesetCount();
                                map->addTileset(tilesetnormal1);
                            }
                            //add the tile
                            Tiled::Cell cell;
                            cell.tile=map->tilesetAt(indexTileset)->tileAt(tiledIndex);
                            Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(map->layerAt(indexLayerCutTree));
                            tileLayer->setCell(x,y,cell);
                        }
                        else if(type=="STRENGTH_BOULDER")
                        {
                            //add tile layer
                            int indexLayerCutTree=0;
                            while(indexLayerCutTree<map->layerCount())
                            {
                                if(map->layerAt(indexLayerCutTree)->isObjectGroup() && map->layerAt(indexLayerCutTree)->name()=="Object")
                                    break;
                                indexLayerCutTree++;
                            }
                            if(indexLayerCutTree>=map->layerCount())
                            {
                                Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Object",0,0,map->width(),map->height());
                                indexLayerCutTree=map->layerCount();
                                map->addLayer(objectGroup);
                            }
                            //add tileset
                            int indexTileset=0;
                            while(indexTileset<map->tilesetCount())
                            {
                                const QString &imageSource=map->tilesetAt(indexTileset)->imageSource();
                                if(imageSource.endsWith("normal1.tsx") || map->tilesetAt(indexTileset)->name()=="normal1.tsx" || map->tilesetAt(indexTileset)->name()=="normal1")
                                    break;
                                indexTileset++;
                            }
                            if(indexTileset>=map->tilesetCount())
                            {
                                indexTileset=map->tilesetCount();
                                map->addTileset(tilesetnormal1);
                            }
                            //add the tile
                            Tiled::Cell cell;
                            cell.tile=map->tilesetAt(indexTileset)->tileAt(67);
                            Tiled::MapObject *mapObject=new Tiled::MapObject("","StrengthBoulder",QPointF(x,y+offsetToY),QSizeF(1,1));
                            mapObject->setCell(cell);
                            map->layerAt(indexLayerCutTree)->asObjectGroup()->addObject(mapObject);
                        }
                        else
                        {
                            std::cerr << "unknown [hmobject] type" << std::endl;
                            abort();
                        }
                    }
                    values.clear();
                    balise.clear();
                    baliseEnd.clear();
                }
                else if(!balise.isEmpty())
                    values << line;
                else if(line.contains(QRegularExpression("^\\[[a-z]+\\]$")))
                {
                    balise=line;
                    baliseEnd=balise;
                    baliseEnd.replace("[","[/");
                }
            }
            tempFile.close();
            npcinicountValid++;
        }
        /*else
            std::cerr << "File not found to open npc ini: " << npcFile.toStdString() << std::endl;*/
    }
    if(!botList.isEmpty())
    {
        //add the move layer if needed
        bool hadObjectLayer=false;
        int indexLayerObject=0;
        while(indexLayerObject<map->layerCount())
        {
            if(map->layerAt(indexLayerObject)->isObjectGroup() && map->layerAt(indexLayerObject)->name()=="Object")
            {
                hadObjectLayer=true;
                break;
            }
            indexLayerObject++;
        }
        if(indexLayerObject>=map->layerCount())
        {
            indexLayerObject=map->layerCount();
            Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Object",0,0,map->width(),map->height());
            map->addLayer(objectGroup);
        }

        if(!hadObjectLayer)
        {
            QString botsFile=file;
            botsFile.replace(".tmx","-bots.xml");
            QFile tempFile(botsFile);
            if(tempFile.open(QIODevice::WriteOnly))
            {
                tempFile.write(QStringLiteral("<bots>\n").toUtf8());
                int index=0;
                while(index<botList.size())
                {
                    const BotDescriptor &botDescriptor=botList.at(index);
                    if(dropBot.contains(QPair<unsigned int,unsigned int>(botDescriptor.x,botDescriptor.y)))
                    {
                        index++;
                        continue;
                    }
                    tempFile.write(QStringLiteral("  <bot id=\"%1\">\n").arg(botId).toUtf8());
                    if(botDescriptor.name!="NULL" && !botDescriptor.name.isEmpty())
                        tempFile.write(QStringLiteral("    <name>%1</name>\n").arg(botDescriptor.name).toUtf8());
                    QList<QMap<QString,QString> > textFull=botDescriptor.text;
                    if(botDescriptor.fightMonsterId.isEmpty())
                    {
                        bool additionalStep=false;
                        if(botDescriptor.shopId!=0)
                            additionalStep=true;
                        int sub_index=0;
                        while(sub_index<textFull.size())
                        {
                            const QMap<QString,QString> &text=textFull.at(sub_index);
                            if(text.value("en")=="heal")
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"heal\"/>\n").arg(sub_index+1).toUtf8());
                            else if(text.value("en")=="warehouse")
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"warehouse\"/>\n").arg(sub_index+1).toUtf8());
                            else
                            {
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"text\">\n").arg(sub_index+1).toUtf8());
                                QMapIterator<QString,QString> i(text);
                                while (i.hasNext()) {
                                    i.next();
                                    if(i.key()=="en" || i.value()!=text.value("en"))
                                    {
                                        QString lang;
                                        if(i.key()!="en")
                                            lang=" lang=\""+i.key()+"\"";
                                        if(sub_index<(textFull.size()-1) || additionalStep)
                                        {
                                            QString next="Next";
                                            if(i.key()=="nl")
                                                next="Volgende";
                                            else if(i.key()=="fi")
                                                next="Seuraava";
                                            else if(i.key()=="fr")
                                                next="Suivant";
                                            else if(i.key()=="de")
                                                next="Nächster";
                                            else if(i.key()=="it")
                                                next="Prossimo";
                                            else if(i.key()=="pt")
                                                next="Próximo";
                                            else if(i.key()=="es")
                                                next="Siguiente";
                                            tempFile.write(
                                                        QStringLiteral("      <text%1><![CDATA[%2<br /><br /><a href=\"%3\">[%4]</a>]]></text>\n")
                                                            .arg(lang)
                                                            .arg(i.value())
                                                            .arg(sub_index+2)
                                                            .arg(next)
                                                            .toUtf8()
                                                        );
                                        }
                                        else
                                            tempFile.write(
                                                        QStringLiteral("      <text%1><![CDATA[%2]]></text>\n")
                                                            .arg(lang)
                                                            .arg(i.value())
                                                            .toUtf8()
                                                    );
                                    }
                                }

                                tempFile.write(QStringLiteral("    </step>\n").toUtf8());
                            }
                            sub_index++;
                        }
                        if(botDescriptor.name.contains("Shop Keeper") && botDescriptor.shopId==0)
                            std::cerr << "2) botDescriptor.name.contains(\"Shop Keeper\") && botDescriptor.shopId==0" << std::endl;
                        if(botDescriptor.shopId!=0)
                        {
                            tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"text\">\n"
                            "      <text><![CDATA[<a href=\"%2\">Buy</a><br />\n"
                            "      <a href=\"%3\">Sell</a>]]></text>\n"
                            "    </step>\n")
                                           .arg(sub_index+1)
                                           .arg(sub_index+2)
                                           .arg(sub_index+3)
                                           .toUtf8()
                                           );
                            sub_index++;
                            tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"shop\" shop=\"%2\"/>\n")
                                           .arg(sub_index+1)
                                           .arg(botDescriptor.shopId)
                                           .toUtf8()
                                           );
                            sub_index++;
                            tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"sell\" shop=\"%2\"/>\n")
                                           .arg(sub_index+1)
                                           .arg(botDescriptor.shopId)
                                           .toUtf8()
                                           );
                            sub_index++;
                        }
                    }
                    else
                    {
                        tempFile.write(QStringLiteral("    <step type=\"fight\" id=\"1\" fightid=\"%1\" />\n").arg(fightid).toUtf8());
                        FightDescriptor fightDescriptor;
                        if(textFull.size()>=1)
                            fightDescriptor.start=textFull.first();
                        if(textFull.size()>=2)
                            fightDescriptor.win=textFull.last();
                        int sub_sub_index=0;
                        while(sub_sub_index<botDescriptor.fightMonsterId.size())
                        {
                            const uint32_t monsterId=botDescriptor.fightMonsterId.at(sub_sub_index);
                            if(monsterId==0)
                            {
                                std::cerr << "monsterId==0 for bot monster!" << std::endl;
                                abort();
                            }
                            fightDescriptor.fightMonsterId << monsterId;
                            const quint32 level=botDescriptor.fightMonsterLevel.at(sub_sub_index);
                            if(level==0)
                            {
                                std::cerr << "level==0 for bot monster!" << std::endl;
                                abort();
                            }
                            fightDescriptor.fightMonsterLevel << level;
                            sub_sub_index++;
                        }
                        fightDescriptor.id=fightid;
                        fightList << fightDescriptor;
                        fightid++;
                    }
                    tempFile.write(QStringLiteral("  </bot>\n").toUtf8());
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","bot",QPointF(botDescriptor.x,botDescriptor.y+offsetToY),QSizeF(1,1));
                        QString botsFileClean=botsFile;
                        botsFileClean.remove(".tmx");
                        mapObject->setProperty("file",botsFileClean);
                        mapObject->setProperty("id",QString::number(botId));
                        if(!botDescriptor.skin.isEmpty())
                        {
                            mapObject->setProperty("skin",botDescriptor.skin);
                            if(botDescriptor.orientation=="bottom" || botDescriptor.orientation=="down")
                                mapObject->setProperty("lookAt","bottom");
                            else if(botDescriptor.orientation=="top" || botDescriptor.orientation=="up")
                                mapObject->setProperty("lookAt","top");
                            else if(botDescriptor.orientation=="right")
                                mapObject->setProperty("lookAt","right");
                            else if(botDescriptor.orientation=="left")
                                mapObject->setProperty("lookAt","left");
                            else
                                mapObject->setProperty("lookAt","bottom");
                        }
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(0);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found (" << __LINE__ << ")";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayerObject)->asObjectGroup()->addObject(mapObject);
                    }
                    botId++;
                    index++;
                }
                tempFile.write(QStringLiteral("</bots>").toUtf8());
                tempFile.close();
            }
            else
                std::cerr << "File not found to open in write bot: " << botsFile.toStdString() << std::endl;
        }
    }

    if(!fightList.isEmpty())
    {
        QString fightsFile=file;
        fightsFile.replace(".tmx",".xml");
        QString fightPath="../fight/"+fightsFile;
        QFile tempFile(fightPath);
        if(tempFile.open(QIODevice::WriteOnly))
        {
            tempFile.write(QStringLiteral("<fights>\n").toUtf8());
            int index=0;
            while(index<fightList.size())
            {
                tempFile.write(QStringLiteral("  <fight id=\"%1\">\n").arg(fightList.at(index).id).toUtf8());
                if(!fightList.at(index).start.isEmpty())
                {
                    const QMap<QString,QString> &text=fightList.at(index).start;
                    QMapIterator<QString,QString> i(text);
                    while (i.hasNext()) {
                        i.next();
                        if(i.key()=="en" || i.value()!=text.value("en"))
                        {
                            QString lang;
                            if(i.key()!="en")
                                lang=" lang=\""+i.key()+"\"";
                            tempFile.write(
                                        QStringLiteral("      <start%1><![CDATA[%2]]></start>\n")
                                            .arg(lang)
                                            .arg(i.value())
                                            .toUtf8()
                                    );
                        }
                    }
                }
                if(!fightList.at(index).win.isEmpty())
                {
                    const QMap<QString,QString> &text=fightList.at(index).win;
                    QMapIterator<QString,QString> i(text);
                    while (i.hasNext()) {
                        i.next();
                        if(i.key()=="en" || i.value()!=text.value("en"))
                        {
                            QString lang;
                            if(i.key()!="en")
                                lang=" lang=\""+i.key()+"\"";
                            tempFile.write(
                                        QStringLiteral("      <win%1><![CDATA[%2]]></win>\n")
                                            .arg(lang)
                                            .arg(i.value())
                                            .toUtf8()
                                    );
                        }
                    }
                }
                int sub_index=0;
                while(sub_index<fightList.at(index).fightMonsterId.size())
                {
                    tempFile.write(QStringLiteral("    <monster id=\"%1\" level=\"%2\" />\n")
                                   .arg(fightList.at(index).fightMonsterId.at(sub_index))
                                   .arg(fightList.at(index).fightMonsterLevel.at(sub_index))
                                   .toUtf8());
                    sub_index++;
                }
                tempFile.write(QStringLiteral("  </fight>\n").toUtf8());
                index++;
            }
            tempFile.write(QStringLiteral("</fights>").toUtf8());
            tempFile.close();
        }
        else
            std::cerr << "File not found to open in write fight: " << fightPath.toStdString() << std::endl;
    }

    QMapIterator<QPair<int,int>, WarpDescriptor> i(warpList);
    while (i.hasNext()) {
        i.next();
        const QPair<int,int> &coord=i.key();
        const WarpDescriptor &warpDescriptor=i.value();
        const unsigned int x=coord.first;
        const unsigned int y=coord.second;
        Tiled::MapObject *mapObject=NULL;
        switch(warpDescriptor.type)
        {
        case WarpType_Door:
            mapObject=new Tiled::MapObject("","door",QPointF(x,y+offsetToY),QSizeF(1,1));
        break;
        default:
        case WarpType_TpOnIt:
            mapObject=new Tiled::MapObject("","teleport on it",QPointF(x,y+offsetToY),QSizeF(1,1));
        break;
        case WarpType_TpOnPush:
            mapObject=new Tiled::MapObject("","teleport on push",QPointF(x,y+offsetToY),QSizeF(1,1));
        break;
        }
        QString cleanMapName=warpDescriptor.destMap;
        cleanMapName.remove(".tmx");
        mapObject->setProperty("map",cleanMapName);
        mapObject->setProperty("x",QString::number(warpDescriptor.destX));
        mapObject->setProperty("y",QString::number(warpDescriptor.destY));
        Tiled::Cell cell=mapObject->cell();
        cell.tile=map->tilesetAt(indexTileset)->tileAt(2);
        if(cell.tile==NULL)
            qDebug() << "Tile not found (" << __LINE__ << ")";
        mapObject->setCell(cell);
        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);

        if(mapWithHeal.contains(warpDescriptor.destMap))
        {
            if(x>0)
            {
                unsigned int newX=x-1,newY=y;
                if(haveContent(map,indexLayerWalkable,newX,newY) && !haveContent(map,indexLayerCollisions,newX,newY) && !haveContent(map,indexLayerWalkBehind,newX,newY))
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","rescue",QPointF(newX,newY+offsetToY),QSizeF(1,1));
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(1);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found (" << __LINE__ << ")";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
            if(y>0)
            {
                unsigned int newX=x,newY=y-1;
                if(haveContent(map,indexLayerWalkable,newX,newY) && !haveContent(map,indexLayerCollisions,newX,newY) && !haveContent(map,indexLayerWalkBehind,newX,newY))
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","rescue",QPointF(newX,newY+offsetToY),QSizeF(1,1));
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(1);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found (" << __LINE__ << ")";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
            if(x<((unsigned int)map->width()-1))
            {
                unsigned int newX=x+1,newY=y;
                if(haveContent(map,indexLayerWalkable,newX,newY) && !haveContent(map,indexLayerCollisions,newX,newY) && !haveContent(map,indexLayerWalkBehind,newX,newY))
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","rescue",QPointF(newX,newY+offsetToY),QSizeF(1,1));
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(1);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found (" << __LINE__ << ")";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
            if(y<((unsigned int)map->height()-1))
            {
                unsigned int newX=x,newY=y+1;
                if(haveContent(map,indexLayerWalkable,newX,newY) && !haveContent(map,indexLayerCollisions,newX,newY) && !haveContent(map,indexLayerWalkBehind,newX,newY))
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","rescue",QPointF(newX,newY+offsetToY),QSizeF(1,1));
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(1);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found (" << __LINE__ << ")";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
        }
    }
    bool grassLayerDropped=false;
    bool haveGrassLayer=false;
    {
        int indexLayer=0;
        while(indexLayer<map->layerCount())
        {
            if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
            {
                Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
                if(tileLayer->name()=="Grass")
                    haveGrassLayer=true;
                if(tileLayer->isEmpty())
                {
                    if(tileLayer->name()=="Grass")
                        grassLayerDropped=true;
                    delete map->takeLayerAt(indexLayer);
                    indexLayer--;
                }
            }
            indexLayer++;
        }
    }
    bool mapHaveGrassMonster=map->hasProperty("dayPokemonChances") && !map->property("dayPokemonChances").isEmpty();
    bool mapHaveWaterMonster=map->hasProperty("waterPokemonChances") && !map->property("waterPokemonChances").isEmpty();
    bool mapHaveFishMonster=map->hasProperty("fishPokemonChances") && !map->property("fishPokemonChances").isEmpty();
    bool mapIsCave=map->hasProperty("isCave") && map->property("isCave")=="true";
    bool havePvPAttribute=map->hasProperty("pvp");

    if(!indexLayerCollisions.empty())
    {
        QStringList list;
        list << "CutTree" << "HeadButtTree" << "RockMash" << "WhirlPool";
        foreach (const QString &str, list) {
            int indexLayerCutTree=0;
            while(indexLayerCutTree<map->layerCount())
            {
                if(map->layerAt(indexLayerCutTree)->isTileLayer() && map->layerAt(indexLayerCutTree)->name()==str)
                    break;
                indexLayerCutTree++;
            }
            if(indexLayerCutTree<map->layerCount())
            {
                Tiled::Layer *layer=map->takeLayerAt(indexLayerCutTree);
                int indexLayerCollisions=0;
                while(indexLayerCollisions<map->layerCount())
                {
                    if(map->layerAt(indexLayerCollisions)->isTileLayer() && map->layerAt(indexLayerCollisions)->name()=="Collisions")
                        break;
                    indexLayerCollisions++;
                }
                if(indexLayerCollisions<map->layerCount())
                    map->insertLayer(indexLayerCollisions,layer);
                else
                    abort();
            }
        }
    }
    Tiled::Properties emptyProperties;
    map->setProperties(emptyProperties);
    map->setLayerDataFormat(Tiled::Map::LayerDataFormat::Base64Zlib);
    write.writeMap(map,file);
    //delete map;
    {
        QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        Tiled::Tileset::preloadedTileset.clear();
    }

    if((grassLayerDropped || !haveGrassLayer) && mapHaveGrassMonster)
    {
        QString botsFile=file;
        botsFile.replace(".tmx",".xml");
        QFile tempFile(botsFile);
        if(tempFile.open(QIODevice::ReadWrite))
        {
            QString content=QString::fromUtf8(tempFile.readAll());
            if(content.contains("<grass>") && !content.contains("<cave>"))
            {
                content.replace("<grass>","<cave>");
                content.replace("</grass>","</cave>");
            }
            if(content.contains("<grassNight>") && !content.contains("<caveNight>"))
            {
                content.replace("<grassNight>","<caveNight>");
                content.replace("</grassNight>","</caveNight>");
            }
            tempFile.resize(0);
            tempFile.write(content.toUtf8());
            tempFile.close();
        }
    }
    if(!mapHaveGrassMonster && !mapHaveWaterMonster && !mapHaveFishMonster && !mapIsCave && !havePvPAttribute)
    {
        QString botsFile=file;
        botsFile.replace(".tmx",".xml");
        QFile tempFile(botsFile);
        if(tempFile.open(QIODevice::ReadWrite))
        {
            QString content=QString::fromUtf8(tempFile.readAll());
            content.replace(" type=\"outdoor\""," type=\"indoor\"");
            tempFile.resize(0);
            tempFile.write(content.toUtf8());
            tempFile.close();
        }
    }
    return 0;
}

void loadMonster()
{
    //open and quick check the file
    QDirIterator it("../monsters/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath=it.next();
        if(filePath!="." && filePath!=".." && filePath!="" && QFileInfo(filePath).isFile())
        {
            QFile xmlFile(filePath);
            QByteArray xmlContent;
            if(xmlFile.open(QIODevice::ReadOnly))
            {
                xmlContent=xmlFile.readAll();
                xmlFile.close();
                QDomDocument domDocument;
                QString errorStr;
                int errorLine,errorColumn;
                if(domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
                {
                    QDomElement root = domDocument.documentElement();
                    if(root.tagName()=="monsters")
                    {
                        //load the content
                        bool ok;
                        QDomElement item = root.firstChildElement("monster");
                        while(!item.isNull())
                        {
                            if(item.isElement())
                            {
                                if(item.hasAttribute("id"))
                                {
                                    quint32 id=item.attribute("id").toUInt(&ok);
                                    if(ok)
                                    {
                                        QDomElement itemName = item.firstChildElement("name");
                                        while(!itemName.isNull())
                                        {
                                            if(itemName.isElement())
                                            {
                                                monsterNameToMonsterId[simplifyItemName(itemName.text())]=id;
                                                break;
                                            }
                                            itemName = itemName.nextSiblingElement("name");
                                        }
                                    }
                                }
                            }
                            item = item.nextSiblingElement("monster");
                        }
                        std::cout << "Loaded monsters from: " << filePath.toStdString() << std::endl;
                    }
                    else
                        std::cerr << "Wrong root balise: " << filePath.toStdString() << std::endl;
                }
                else
                    std::cerr << "Not xml file: " << filePath.toStdString() << std::endl;
            }
            else
                std::cerr << "Unable to read: " << filePath.toStdString() << std::endl;
        }
    }

    if(monsterNameToMonsterId.empty())
    {
        std::cerr << "monsterNameToMonsterId.empty(): have you ../monsters/ at catchchallenger format?" << std::endl;
        abort();
    }
}

void loadShop()
{
    //open and quick check the file
    QFile xmlFile("../itemdex.xml");
    QByteArray xmlContent;
    if(xmlFile.open(QIODevice::ReadOnly))
    {
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if(domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            QDomElement root = domDocument.documentElement();
            if(root.tagName()=="itemDatabase")
            {
                //load the content
                bool ok;
                QDomElement items = root.firstChildElement("items");
                while(!items.isNull())
                {
                    QDomElement item = items.firstChildElement("item");
                    while(!item.isNull())
                    {
                        if(item.isElement())
                        {
                            QString name;
                            QDomElement nameXml = item.firstChildElement("name");
                            if(!nameXml.isNull())
                                name=nameXml.text();
                            name=simplifyItemName(name);
                            QString shop;
                            QDomElement shopXml = item.firstChildElement("shop");
                            if(!shopXml.isNull())
                                shop=shopXml.text();
                            const unsigned int shopId=shop.toUInt(&ok);
                            if(!ok)
                            {
                                std::cerr << "shop id is not number" << std::endl;
                                abort();
                            }
                            if(shopId!=0)
                            {
                                if(!itemNameToId.contains(name))
                                {
                                    //std::cerr << "item not found for shop: " << name.toStdString() << std::endl;
                                    //abort();
                                }
                                else
                                    shopList[shopId].itemsId.push_back(itemNameToId.value(name));
                            }
                        }
                        item = item.nextSiblingElement("item");
                    }
                    items = items.nextSiblingElement("items");
                }
                std::cout << "Loaded item from: " << "../itemdex.xml" << std::endl;
            }
            else
                std::cerr << "Wrong root balise: " << "../itemdex.xml" << std::endl;
        }
        else
            std::cerr << "Not xml file: " << "../itemdex.xml" << std::endl;
    }
    else
        std::cerr << "Unable to read: " << "../itemdex.xml" << std::endl;

    if(shopList.size()<2)
    {
        std::cerr << "shopList.empty(): have you ../itemdex.xml at catchchallenger format?" << std::endl;
        abort();
    }
}

void loadCatchChallengerDatapack()
{
    //open and quick check the file
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack("../datapack/");
    if(CatchChallenger::CommonDatapack::commonDatapack.items.item.empty())
    {
        std::cerr << "CatchChallenger::CommonDatapack::commonDatapack.items.item.empty(): have you ../datapack/ at catchchallenger format?" << std::endl;
        abort();
    }

    DatapackClientLoader::datapackLoader.parseDatapack("../datapack/");
    if(DatapackClientLoader::datapackLoader.itemsExtra.empty())
    {
        std::cerr << "DatapackClientLoader::datapackLoader.itemsExtra.empty(): have you ../datapack/ at catchchallenger format?" << std::endl;
        abort();
    }

    QHashIterator<uint32_t,DatapackClientLoader::ItemExtra> i(DatapackClientLoader::datapackLoader.itemsExtra);
    while (i.hasNext()) {
        i.next();
        const CATCHCHALLENGER_TYPE_ITEM &item=i.key();
        const DatapackClientLoader::ItemExtra &itemExtra=i.value();
        itemNameToId[simplifyItemName(itemExtra.name)]=item;
    }

    if(itemNameToId.isEmpty())
    {
        std::cerr << "itemNameToId.isEmpty(): have you ../datapack/ at catchchallenger format?" << std::endl;
        abort();
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LanguagesSelect::languagesSelect=new LanguagesSelect();
    loadMonster();
    loadCatchChallengerDatapack();
    loadShop();
    botId=1;
    fightid=1;
    if(!QFile("invisible.tsx").exists())
    {
        qDebug() << "Tileset invisible.tsx not found";
        return 90;
    }
    Tiled::MapReader reader;
    tilesetinvisible = reader.readTileset("invisible.tsx");
    if (!tilesetinvisible)
    {
        qDebug() << "Unable to open the tilset" << reader.errorString();
        abort();
    }

    if(!QFile("normal1.tsx").exists())
    {
        qDebug() << "Tileset normal1.tsx not found";
        return 90;
    }
    tilesetnormal1 = reader.readTileset("normal1.tsx");
    if (!tilesetnormal1)
    {
        qDebug() << "Unable to open the tilset" << reader.errorString();
        abort();
    }

    if(!QDir("../language/english/NPC").exists())
    {
        qDebug() << "!QDir(\"../language/english/NPC\").exists()";
        return 90;
    }
    bool okX,okY;
    QMap<QString,QString> fileToName;
    QFile mapNames("../language/english/_MAPNAMES.txt");
    if(mapNames.open(QIODevice::ReadOnly))
    {
        while (!mapNames.atEnd()) {
            QStringList values=QString::fromUtf8(mapNames.readLine()).split(",");
            if(values.size()==3)
                fileToName[QStringLiteral("%1.%2.tmx").arg(values.at(0)).arg(values.at(1))]=values.at(2);
        }
        mapNames.close();
    }
    else
        std::cerr << "unable to open the city name: " << "../language/english/_MAPNAMES.txt" << std::endl;
    QFile mapPos("../language/english/UI/_MAP.txt");
    if(mapPos.open(QIODevice::ReadOnly))
    {
        while (!mapPos.atEnd()) {
            QStringList values=QString::fromUtf8(mapPos.readLine()).split(",");
            if(values.size()==5)
                if(fileToName.contains(values.at(0)))
                {
                    qint32 x=values.at(3).toInt(&okX);
                    qint32 y=values.at(4).toInt(&okY);
                    if(okX && okY)
                    {
                        mapX[fileToName[values.at(0)]]=x;
                        mapY[fileToName[values.at(0)]]=y;
                    }
                }
        }
        mapPos.close();
    }


    //QDir("./").mkpath("outofmap");
    QDir dir("./");
    QFileInfoList fileInfoList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot,QDir::Name);
    int index;
    index=0;
    while(index<fileInfoList.size())
    {
        const QString &fileName=fileInfoList.at(index).fileName();
        if(index%100==0)
            std::cout << "Processing " << index << "/" << fileInfoList.size() << ": " << fileName.toStdString() << std::endl;
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
        {
            if(fileInfoList.at(index).exists())
                //get the x/y offset modifier, heal point
                readMap(fileInfoList.at(index).fileName());
            else
            {
                std::cerr << fileName.toStdString() << " not exists" << std::endl;
                abort();
            }
        }
        index++;
    }
    if(mapWithHeal.isEmpty())
        abort();
    index=0;
    while(index<fileInfoList.size())
    {
        const QString &fileName=fileInfoList.at(index).fileName();
        if(index%100==0)
            std::cout << "Create border " << index << "/" << fileInfoList.size() << ": " << fileName.toStdString() << std::endl;
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
        {
            if(fileInfoList.at(index).exists())
                createBorder(fileInfoList.at(index).fileName(),true);
            else
            {
                std::cerr << fileName.toStdString() << " not exists" << std::endl;
                abort();
            }
        }
        index++;
    }

    if(!shopList.isEmpty())
    {
        QString shopPath="../shop/shop.xml";
        QFile tempFile(shopPath);
        if(tempFile.open(QIODevice::WriteOnly))
        {
            tempFile.write(QStringLiteral("<shops>\n").toUtf8());
            QMapIterator<unsigned int,Shop> i(shopList);
            while (i.hasNext()) {
                i.next();
                const unsigned int shopId=i.key();
                const Shop shop=i.value();
                tempFile.write(QStringLiteral("  <shop id=\"%1\">\n").arg(shopId).toUtf8());
                int sub_index=0;
                while(sub_index<shop.itemsId.size())
                {
                    tempFile.write(QStringLiteral("    <product itemId=\"%1\" />\n")
                                   .arg(shop.itemsId.at(sub_index))
                                   .toUtf8());
                    sub_index++;
                }
                tempFile.write(QStringLiteral("  </shop>\n").toUtf8());
            }
            tempFile.write(QStringLiteral("</shops>").toUtf8());
            tempFile.close();
        }
        else
            std::cerr << "File not found to open in write shop: " << shopPath.toStdString() << std::endl;
    }

    if(npcinicountValid==0)
        std::cerr << "npcinicountValid==0" << std::endl;

    std::cout << "Correctly finished" << std::endl;
    return 0;
}
