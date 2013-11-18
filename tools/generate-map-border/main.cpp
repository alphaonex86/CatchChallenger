#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QDomDocument>
#include <QDomElement>

#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_layer.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_mapobject.h"

QHash<QString,int> mapWidth,mapHeight;
QHash<QString,int> xOffsetModifier,yOffsetModifier;
QHash<QString,int> mapX,mapY;
QHash<QString,int> monsterNameToMonsterId;
int botId;
int fightid;

struct BotDescriptor
{
    QString name;
    QString orientation;
    QString skin;
    int x;
    int y;
    QStringList text;
    QList<int> fightMonsterId;
    QList<int> fightMonsterLevel;
};

struct FightDescriptor
{
    quint32 id;
    QString start;
    QString win;
    QList<int> fightMonsterId;
    QList<int> fightMonsterLevel;
};

struct WarpDescriptor
{
    int x;
    int y;
    int destX;
    int destY;
    QString destMap;
};

int readMap(QString file)
{
    if(!QFile(file).exists())
        return 0;
    Tiled::MapReader reader;
    Tiled::Map *map=reader.readMap(file);
    if(map==NULL)
    {
        qDebug() << "Can't read" << file << reader.errorString();
        return 86;
    }
    /*
    QString xString=file;
    QString yString=file;
    xString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\1");
    yString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\2");
    int x=xString.toInt();
    int y=yString.toInt();
    if(x>30 || x<-30 || y>30 || y<-30)//file!="0.0.tmx" && map->property("xOffsetModifier")=="0" && map->property("yOffsetModifier")=="0"
    {
        QFile tempFile(file);
        if(tempFile.open(QIODevice::ReadWrite))
        {
            QString content=QString::fromUtf8(tempFile.readAll());
            content.replace("source=\"","source=\"../");
            tempFile.close();
            QFile destFile("outofmap/"+file);
            if(destFile.open(QIODevice::ReadWrite))
            {
                destFile.write(content.toUtf8());
                tempFile.remove();
                destFile.close();
            }
            else
                qDebug() << "Destination can't be open" << "outofmap/"+file << destFile.errorString();
        }
        else
            qDebug() << "Source can't be open" << file << tempFile.errorString();
        file.replace(".tmx",".xml");
        QFile::rename(file,"outofmap/"+file);
        file.replace(".xml","-bots.xml");
        QFile::rename(file,"outofmap/"+file);
        return 0;
    }*/
    mapWidth[file]=map->width();
    mapHeight[file]=map->height();
    xOffsetModifier[file]=map->property("xOffsetModifier").toInt()/map->tileWidth();
    yOffsetModifier[file]=map->property("yOffsetModifier").toInt()/map->tileHeight();
    return 0;
}

int createBorder(QString file)
{
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
        if(map->tilesetAt(indexTileset)->imageSource()=="invisible.tsx")
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

    //add the move layer if needed
    int indexLayerMoving=0;
    while(indexLayerMoving<map->layerCount())
    {
        if(map->layerAt(indexLayerMoving)->isObjectGroup() && map->layerAt(indexLayerMoving)->name()=="Moving")
            break;
        indexLayerMoving++;
    }
    if(indexLayerMoving>=map->layerCount())
    {
        indexLayerMoving=map->layerCount();
        Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Moving",0,0,map->width(),map->height());
        map->addLayer(objectGroup);
    }

    if(x<=30 && x>=-30 && y<=30 && y>=-30)
    {
        QString mapBorderFile;
        //check the left map
        mapBorderFile=QString("%1.%2.tmx").arg(x-1).arg(y);
        if(mapHeight.contains(mapBorderFile))
        {
            if(false && mapX.contains(file) && mapX.contains(mapBorderFile))
            {
                /*//if is the bigger, do the calcule
                if(map->height()>mapHeight[mapBorderFile])
                {
                    quint32 yOnBorderMap=mapHeight[mapBorderFile]/2+1;
                    QPointF point(0,yOnBorderMap+(mapY[file]-mapY[mapBorderFile]));
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-left",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
                    }
                }
                //if is the smaller, just put at the middle
                else
                {
                    QPointF point(0,map->height()/2+1);
                    if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    {
                        Tiled::MapObject *mapObject=new Tiled::MapObject("","border-left",point,QSizeF(1,1));
                        mapObject->setProperty("map",mapBorderFile);
                        Tiled::Cell cell=mapObject->cell();
                        cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                        if(cell.tile==NULL)
                            qDebug() << "Tile not found";
                        mapObject->setCell(cell);
                        map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
                    }
                }*/
            }
            else
            {
                int offset=0;
                if(map->height()>mapHeight[mapBorderFile])
                {
                    offset=mapHeight[mapBorderFile]-map->height();
                    offset/=2;
                }
                QPointF point(0,map->height()/2+1+yOffsetModifier[mapBorderFile]/2+offset);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    point=QPointF(0,map->height()/2+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","border-left",point,QSizeF(1,1));
                    mapObject->setProperty("map",mapBorderFile);
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
        }
        //check the right map
        mapBorderFile=QString("%1.%2.tmx").arg(x+1).arg(y);
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
                QPointF point(map->width()-1,map->height()/2+1+yOffsetModifier[mapBorderFile]/2+offset);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    point=QPointF(map->width()-1,map->height()/2+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","border-right",point,QSizeF(1,1));
                    mapObject->setProperty("map",mapBorderFile);
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
        }
        //check the top map
        mapBorderFile=QString("%1.%2.tmx").arg(x).arg(y-1);
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
                QPointF point(map->width()/2+xOffsetModifier[mapBorderFile]/2+offset,0+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    point=QPointF(map->width()/2,0+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","border-top",point,QSizeF(1,1));
                    mapObject->setProperty("map",mapBorderFile);
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
        }
        //check the bottom map
        mapBorderFile=QString("%1.%2.tmx").arg(x).arg(y+1);
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
                QPointF point(map->width()/2+xOffsetModifier[mapBorderFile]/2+offset,map->height()-1+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                    point=QPointF(map->width()/2,map->height()-1+1);
                if(point.x()>=0 && point.x()<map->width() && point.y()>=1 && point.y()<=map->height())
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","border-bottom",point,QSizeF(1,1));
                    mapObject->setProperty("map",mapBorderFile);
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(3);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
                }
            }
        }
    }

    QList<BotDescriptor> botList;
    QHash<QPair<int,int>,WarpDescriptor> warpList;
    QList<FightDescriptor> fightList;
    QStringList textList;
    {
        QString npcFile=file;
        npcFile.replace(".tmx",".txt");
        QFile tempFile("../language/english/NPC/"+npcFile);
        if(tempFile.open(QIODevice::ReadOnly))
        {
            while (!tempFile.atEnd()) {
                QString line=QString::fromUtf8(tempFile.readLine());
                line.replace("\n","");
                line.replace("\r","");
                line.replace("\t","");
                line.replace("Route ","Road ");
                line.replace(QRegularExpression(" +$"),"");
                textList << line;
            }
            tempFile.close();
        }
    }
    bool ok;
    {
        QString npcFile=file;
        npcFile.replace(".tmx",".txt");
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
                                if((int)tempIndex>=textList.size())
                                    break;
                                botDescriptor.text << textList.at(tempIndex);
                                index++;
                            }
                            QStringList textIndexMonster=values.at(5).split(",");
                            if(textIndexMonster.size()>0)
                            {
                                if((textIndexMonster.size()%2)==0)
                                {
                                    int fightIndex=0;
                                    while(fightIndex<textIndexMonster.size())
                                    {
                                        if(monsterNameToMonsterId.contains(textIndexMonster.at(fightIndex)))
                                        {
                                            bool ok;
                                            quint32 level=textIndexMonster.at(fightIndex+1).toUInt(&ok);
                                            if(ok)
                                            {
                                                botDescriptor.fightMonsterId << monsterNameToMonsterId[textIndexMonster.at(fightIndex)];
                                                botDescriptor.fightMonsterLevel << level;
                                            }
                                        }
                                        fightIndex+=2;
                                    }
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
                                botList << botDescriptor;
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
                            warpDescriptor.destMap=QString("%1.%2.tmx").arg(values.at(4)).arg(values.at(5));
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
    }
    if(!botList.isEmpty())
    {
        //add the move layer if needed
        int indexLayerObject=0;
        while(indexLayerObject<map->layerCount())
        {
            if(map->layerAt(indexLayerObject)->isObjectGroup() && map->layerAt(indexLayerObject)->name()=="Object")
                break;
            indexLayerObject++;
        }
        if(indexLayerObject>=map->layerCount())
        {
            indexLayerObject=map->layerCount();
            Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Object",0,0,map->width(),map->height());
            map->addLayer(objectGroup);
        }

        QString botsFile=file;
        botsFile.replace(".tmx","-bots.xml");
        QFile tempFile(botsFile);
        if(tempFile.open(QIODevice::WriteOnly))
        {
            tempFile.write(QString("<bots>\n").toUtf8());
            int index=0;
            while(index<botList.size())
            {
                tempFile.write(QString("  <bot id=\"%1\">\n").arg(botId).toUtf8());
                if(botList.at(index).fightMonsterId.isEmpty())
                {
                    int sub_index=0;
                    while(sub_index<botList.at(index).text.size())
                    {
                        tempFile.write(QString("    <step id=\"%1\" type=\"text\"><text><![CDATA[%2]]></text></step>\n").arg(sub_index+1).arg(botList.at(index).text.at(sub_index)).toUtf8());
                        sub_index++;
                    }
                }
                else
                {
                    tempFile.write(QString("    <step type=\"fight\" id=\"%1\" fightid=\"%1\">\n").arg(fightid).toUtf8());
                    FightDescriptor fightDescriptor;
                    if(botList.at(index).text.size()>=1)
                        fightDescriptor.start=botList.at(index).text.first();
                    if(botList.at(index).text.size()>=2)
                        fightDescriptor.win=botList.at(index).text.last();
                    int sub_sub_index=0;
                    while(sub_sub_index<botList.at(index).fightMonsterId.size())
                    {
                        fightDescriptor.fightMonsterId << botList.at(index).fightMonsterId.at(sub_sub_index);
                        fightDescriptor.fightMonsterLevel << botList.at(index).fightMonsterLevel.at(sub_sub_index);
                        sub_sub_index++;
                    }
                    fightDescriptor.id=fightid;
                    fightList << fightDescriptor;
                    fightid++;
                }
                tempFile.write(QString("  </bot>\n").toUtf8());
                {
                    Tiled::MapObject *mapObject=new Tiled::MapObject("","bot",QPointF(botList.at(index).x,botList.at(index).y+1),QSizeF(1,1));
                    mapObject->setProperty("file",botsFile);
                    mapObject->setProperty("id",QString::number(botId));
                    if(!botList.at(index).skin.isEmpty())
                    {
                        mapObject->setProperty("skin",botList.at(index).skin);
                        if(botList.at(index).orientation=="bottom" || botList.at(index).orientation=="down")
                            mapObject->setProperty("lookAt","bottom");
                        else if(botList.at(index).orientation=="top" || botList.at(index).orientation=="up")
                            mapObject->setProperty("lookAt","top");
                        else if(botList.at(index).orientation=="right")
                            mapObject->setProperty("lookAt","right");
                        else if(botList.at(index).orientation=="left")
                            mapObject->setProperty("lookAt","left");
                        else
                            mapObject->setProperty("lookAt","bottom");
                    }
                    Tiled::Cell cell=mapObject->cell();
                    cell.tile=map->tilesetAt(indexTileset)->tileAt(0);
                    if(cell.tile==NULL)
                        qDebug() << "Tile not found";
                    mapObject->setCell(cell);
                    map->layerAt(indexLayerObject)->asObjectGroup()->addObject(mapObject);
                }
                botId++;
                index++;
            }
            tempFile.write(QString("</bots>").toUtf8());
            tempFile.close();
        }
    }

    if(!fightList.isEmpty())
    {
        QString fightsFile=file;
        fightsFile.replace(".tmx",".xml");
        QFile tempFile("../fight/"+fightsFile);
        if(tempFile.open(QIODevice::WriteOnly))
        {
            tempFile.write(QString("<fights>\n").toUtf8());
            int index=0;
            while(index<fightList.size())
            {
                tempFile.write(QString("  <fight id=\"%1\">\n").arg(fightList.at(index).id).toUtf8());
                if(!fightList.at(index).start.isEmpty())
                    tempFile.write(QString("    <start><![CDATA[%1]]></start>\n").arg(fightList.at(index).start).toUtf8());
                if(!fightList.at(index).win.isEmpty())
                    tempFile.write(QString("    <win><![CDATA[%1]]></win>\n").arg(fightList.at(index).win).toUtf8());
                int sub_index=0;
                while(sub_index<fightList.at(index).fightMonsterId.size())
                {
                    tempFile.write(QString("    <monster id=\"%1\" level=\"%2\" />\n").arg(fightList.at(index).fightMonsterId.at(sub_index)).arg(fightList.at(index).fightMonsterLevel.at(sub_index)).toUtf8());
                    sub_index++;
                }
                tempFile.write(QString("  </fight>\n").toUtf8());
                index++;
            }
            tempFile.write(QString("</fights>").toUtf8());
            tempFile.close();
        }
    }

    QHashIterator<QPair<int,int>, WarpDescriptor> i(warpList);
    while (i.hasNext()) {
        i.next();
        Tiled::MapObject *mapObject=new Tiled::MapObject("","door",QPointF(i.key().first,i.key().second+1),QSizeF(1,1));
        mapObject->setProperty("map",i.value().destMap);
        mapObject->setProperty("x",QString::number(i.value().destX));
        mapObject->setProperty("y",QString::number(i.value().destY));
        Tiled::Cell cell=mapObject->cell();
        cell.tile=map->tilesetAt(indexTileset)->tileAt(2);
        if(cell.tile==NULL)
            qDebug() << "Tile not found";
        mapObject->setCell(cell);
        map->layerAt(indexLayerMoving)->asObjectGroup()->addObject(mapObject);
    }
    {
        int indexLayer=0;
        while(indexLayer<map->layerCount())
        {
            if(map->layerAt(indexLayer)->layerType()==Tiled::Layer::TileLayerType)
            {
                Tiled::TileLayer *tileLayer=map->layerAt(indexLayer)->asTileLayer();
                if(tileLayer->isEmpty())
                {
                    delete map->takeLayerAt(indexLayer);
                    indexLayer--;
                }
            }
            indexLayer++;
        }
    }

    write.writeMap(map,file);
    return 0;
}

void loadMonster()
{
    //open and quick check the file
    QFile xmlFile("../monsters/monster.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
        return;
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        return;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
        return;

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
                            monsterNameToMonsterId[itemName.text()]=id;
                            break;
                        }
                        itemName = itemName.nextSiblingElement("name");
                    }
                }
            }
        }
        item = item.nextSiblingElement("monster");
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
    bool okX,okY;
    QHash<QString,QString> fileToName;
    QFile mapNames("../language/english/_MAPNAMES.txt");
    if(mapNames.open(QIODevice::ReadOnly))
    {
        while (!mapNames.atEnd()) {
            QStringList values=QString::fromUtf8(mapNames.readLine()).split(",");
            if(values.size()==3)
                fileToName[QString("%1.%2.tmx").arg(values.at(0)).arg(values.at(1))]=values.at(2);
        }
        mapNames.close();
    }
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
    QFileInfoList fileInfoList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int index;
    index=0;
    while(index<fileInfoList.size())
    {
        const QString &fileName=fileInfoList.at(index).fileName();
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
            if(fileInfoList.at(index).exists())
                readMap(fileInfoList.at(index).fileName());
        index++;
    }
    index=0;
    while(index<fileInfoList.size())
    {
        const QString &fileName=fileInfoList.at(index).fileName();
        if(fileName.contains(QRegularExpression("^-?[0-9]+\\.-?[0-9]+\\.tmx$")))
            if(fileInfoList.at(index).exists())
                createBorder(fileInfoList.at(index).fileName());
        index++;
    }
    return 0;
}
