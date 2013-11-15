#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>

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

int readMap(QString file)
{
    if(!QFile(file).exists())
        return 0;
    QString xString=file;
    QString yString=file;
    xString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\1");
    yString.replace(QRegularExpression("^(-?[0-9]{1,2}).(-?[0-9]{1,2})\\.tmx$"),"\\2");
    int x=xString.toInt();
    int y=yString.toInt();
    Tiled::MapReader reader;
    Tiled::Map *map=reader.readMap(file);
    if(map==NULL)
    {
        qDebug() << "Can't read" << file << reader.errorString();
        return 86;
    }
    if(x>30 || x<-30 || y>30 || y<-30/*file!="0.0.tmx" && map->property("xOffsetModifier")=="0" && map->property("yOffsetModifier")=="0"*/)
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
    }
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
    int indexLayer=0;
    while(indexLayer<map->layerCount())
    {
        if(map->layerAt(indexLayer)->isObjectGroup() && map->layerAt(indexLayer)->name()=="Moving")
            break;
        indexLayer++;
    }
    if(indexLayer>=map->layerCount())
    {
        indexLayer=map->layerCount();
        Tiled::ObjectGroup *objectGroup=new Tiled::ObjectGroup("Moving",0,0,map->width(),map->height());
        map->addLayer(objectGroup);
    }

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
                map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
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
                map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
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
                map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
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
                map->layerAt(indexLayer)->asObjectGroup()->addObject(mapObject);
            }
        }
    }
    write.writeMap(map,file);
    return 0;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(!QFile("invisible.tsx").exists())
    {
        qDebug() << "Tileset invisible.tsx not found";
        return 90;
    }
    bool okX,okY;
    QHash<QString,QString> fileToName;
    QFile mapNames("../language/english/_MAPNAMES.txt");
    if(mapNames.open(QIODevice::ReadWrite))
    {
        while (!mapNames.atEnd()) {
            QStringList values=QString::fromUtf8(mapNames.readLine()).split(",");
            if(values.size()==3)
                fileToName[QString("%1.%2.tmx").arg(values.at(0)).arg(values.at(1))]=values.at(2);
        }
        mapNames.close();
    }
    QFile mapPos("../language/english/UI/_MAP.txt");
    if(mapPos.open(QIODevice::ReadWrite))
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

    QDir("./").mkpath("outofmap");
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
    qDebug() << "Done";
    return 0;
}
