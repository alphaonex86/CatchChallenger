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

//todo: use zopfli to improve the layer compression
//split map part delimited with empty tile and adapt the tp (scan all the square)
//drop not used tileset

QHash<QString,int> mapWidth,mapHeight;
QHash<QString,int> xOffsetModifier,yOffsetModifier;
QHash<QString,int> mapX,mapY;
QHash<QString,int> monsterNameToMonsterId;
int botId;
int fightid;
QSet<QString> mapWithHeal;

struct BotDescriptor
{
    QString name;
    QString orientation;
    QString skin;
    int x;
    int y;
    QList<QHash<QString,QString> > text;
    QList<int> fightMonsterId;
    QList<int> fightMonsterLevel;
};

struct FightDescriptor
{
    quint32 id;
    QHash<QString,QString> start;
    QHash<QString,QString> win;
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

QStringList loadNPCText(const QString &npcFile,const QString &language);

//get the x/y offset modifier, heal point
int readMap(QString file)
{
    if(!QFile(file).exists())
        return 0;
    Tiled::MapReader reader;
    reader.loadImage=false;
    Tiled::Map *map=reader.readMap(file);
    if(map==NULL)
    {
        qDebug() << "Can't read" << file << reader.errorString();
        return 86;
    }
    mapWidth[file]=map->width();
    mapHeight[file]=map->height();
    xOffsetModifier[file]=map->property("xOffsetModifier").toInt()/map->tileWidth();
    yOffsetModifier[file]=map->property("yOffsetModifier").toInt()/map->tileHeight();

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

    delete map;
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
        return 0;

    Tiled::MapReader reader;
    Tiled::MapWriter write;
    Tiled::Map *map=reader.readMap(file);
    if(map==NULL)
    {
        qDebug() << "Can't open to create the border" << file << reader.errorString();
        return 86;
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
        Tiled::Tileset *tileset = reader.readTileset("invisible.tsx");
        if (!tileset)
            qDebug() << "Unable to open the tilset" << reader.errorString();
        else
            map->addTileset(tileset);
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
    int indexLayerMoving=0;
    while(indexLayerMoving<map->layerCount())
    {
        if(map->layerAt(indexLayerMoving)->isObjectGroup() && map->layerAt(indexLayerMoving)->name()=="Moving")
        {
            hadMovingLayer=true;
            break;
        }
        indexLayerMoving++;
    }
    if(indexLayerMoving>=map->layerCount())
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
                    int offset=0;
                    if(map->height()>mapHeight[mapBorderFile])
                    {
                        offset=mapHeight[mapBorderFile]-map->height();
                        offset/=2;
                    }
                    QPointF point(0,map->height()/2+offsetToY+yOffsetModifier[mapBorderFile]/2+offset);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        point=QPointF(0,map->height()/2+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-left",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found (" << __LINE__ << ")";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                    }
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
                    int offset=0;
                    if(map->height()>mapHeight[mapBorderFile])
                    {
                        offset=mapHeight[mapBorderFile]-map->height();
                        offset/=2;
                    }
                    QPointF point(map->width()-1,map->height()/2+offsetToY+yOffsetModifier[mapBorderFile]/2+offset);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        point=QPointF(map->width()-1,map->height()/2+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-right",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found (" << __LINE__ << ")";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                    }
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
                    int offset=0;
                    if(map->width()>mapWidth[mapBorderFile])
                    {
                        offset=mapWidth[mapBorderFile]-map->width();
                        offset/=2;
                    }
                    QPointF point(map->width()/2+xOffsetModifier[mapBorderFile]/2+offset,0+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        point=QPointF(map->width()/2,0+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-top",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found (" << __LINE__ << ")";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                    }
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
                    int offset=0;
                    if(map->width()>mapWidth[mapBorderFile])
                    {
                        offset=mapWidth[mapBorderFile]-map->width();
                        offset/=2;
                    }
                    QPointF point(map->width()/2+xOffsetModifier[mapBorderFile]/2+offset,map->height()-1+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                        point=QPointF(map->width()/2,map->height()-1+offsetToY);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-bottom",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found (" << __LINE__ << ")";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                    }
                }
            }
        }

    QList<BotDescriptor> botList;
    QHash<QPair<int,int>,WarpDescriptor> warpList;
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
                                QHash<QString,QString> fullTextList;
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

                            //add heal and warehouse step
                            {
                                QHash<QString,QString> fullTextListHeal;
                                fullTextListHeal["nl"]="heal";
                                fullTextListHeal["en"]="heal";
                                fullTextListHeal["fi"]="heal";
                                fullTextListHeal["fr"]="heal";
                                fullTextListHeal["de"]="heal";
                                fullTextListHeal["it"]="heal";
                                fullTextListHeal["pt"]="heal";
                                fullTextListHeal["es"]="heal";
                                QHash<QString,QString> fullTextListWarehouse;
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
                                    index++;
                                }
                                if(!warehouseAdded && botDescriptor.name=="Pokemon storage system" && !botDescriptor.text.empty())
                                {
                                    botDescriptor.text.insert(index+1,fullTextListWarehouse);
                                    warehouseAdded=true;
                                    index++;
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
                                        const QString monsterString=textIndexMonster.at(fightIndex).toUpper();
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
                            values.clear();
                        }
                        else
                            qDebug() << file << "wrong npc count values";
                    }
                    if(balise=="[warp]")
                    {
                        if(values.count()==7)
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

                            values.clear();
                        }
                        else
                            qDebug() << file << "wrong warp count values";
                    }
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
        }
        else
            std::cerr << "File not found to open npc ini: " << npcFile.toStdString() << std::endl;
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
                    tempFile.write(QStringLiteral("  <bot id=\"%1\">\n").arg(botId).toUtf8());
                    if(botDescriptor.name!="NULL" && !botDescriptor.name.isEmpty())
                        tempFile.write(QStringLiteral("    <name>%1</name>\n").arg(botDescriptor.name).toUtf8());
                    QList<QHash<QString,QString> > textFull=botDescriptor.text;
                    if(botDescriptor.fightMonsterId.isEmpty())
                    {
                        int sub_index=0;
                        while(sub_index<textFull.size())
                        {
                            const QHash<QString,QString> &text=textFull.at(sub_index);
                            if(text.value("en")=="heal")
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"heal\"/>\n").arg(sub_index+1).toUtf8());
                            else if(text.value("en")=="warehouse")
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"warehouse\"/>\n").arg(sub_index+1).toUtf8());
                            else
                            {
                                tempFile.write(QStringLiteral("    <step id=\"%1\" type=\"text\">\n").arg(sub_index+1).toUtf8());
                                QHashIterator<QString,QString> i(text);
                                while (i.hasNext()) {
                                    i.next();
                                    if(i.key()=="en" || i.value()!=text.value("en"))
                                    {
                                        QString lang;
                                        if(i.key()!="en")
                                            lang=" lang=\""+i.key()+"\"";
                                        if(sub_index<(textFull.size()-1))
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
                        mapObject->setProperty("file",botsFile);
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
                    const QHash<QString,QString> &text=fightList.at(index).start;
                    QHashIterator<QString,QString> i(text);
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
                    const QHash<QString,QString> &text=fightList.at(index).win;
                    QHashIterator<QString,QString> i(text);
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
    else
        std::cerr << "Warning no fight detected" << std::endl;

    QHashIterator<QPair<int,int>, WarpDescriptor> i(warpList);
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
    {
        int indexLayer=0;
        while(indexLayer<map->layerCount())
        {
            if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
            {
                Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
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

    Tiled::Properties emptyProperties;
    map->setProperties(emptyProperties);
    map->setLayerDataFormat(Tiled::Map::LayerDataFormat::Base64Zlib);
    write.writeMap(map,file);
    delete map;
    {
        QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        Tiled::Tileset::preloadedTileset.clear();
    }

    if(grassLayerDropped && mapHaveGrassMonster)
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
                                                monsterNameToMonsterId[itemName.text().toUpper()]=id;
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



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    loadMonster();
    botId=1;
    fightid=1;
    if(!QFile("invisible.tsx").exists())
    {
        qDebug() << "Tileset invisible.tsx not found";
        return 90;
    }
    if(!QDir("../language/english/NPC").exists())
    {
        qDebug() << "!QDir(\"../language/english/NPC\").exists()";
        return 90;
    }
    bool okX,okY;
    QHash<QString,QString> fileToName;
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
        if(index%1000==0)
            std::cout << "Processing " << index << "/" << fileInfoList.size() << ": " << fileName.toStdString() << std::endl;
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
            if(fileInfoList.at(index).exists())
                //get the x/y offset modifier, heal point
                readMap(fileInfoList.at(index).fileName());
        index++;
    }
    if(mapWithHeal.isEmpty())
        abort();
    index=0;
    while(index<fileInfoList.size())
    {
        const QString &fileName=fileInfoList.at(index).fileName();
        if(index%1000==0)
            std::cout << "Create border " << index << "/" << fileInfoList.size() << ": " << fileName.toStdString() << std::endl;
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
            if(fileInfoList.at(index).exists())
                createBorder(fileInfoList.at(index).fileName(),true);
        index++;
    }
    return 0;
}
