/*
 * tmxviewer.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of the TMX Viewer example.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "map2png.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>

#include "../../general/base/MoveOnTheMap.h"

QString Map2Png::text_slash=QLatin1Literal("/");
QString Map2Png::text_dottmx=QLatin1Literal(".tmx");
QString Map2Png::text_dotpng=QLatin1Literal(".png");
QString Map2Png::text_Moving=QLatin1Literal("Moving");
QString Map2Png::text_door=QLatin1Literal("door");
QString Map2Png::text_Object=QLatin1Literal("Object");
QString Map2Png::text_bot=QLatin1Literal("bot");
QString Map2Png::text_skin=QLatin1Literal("skin");
QString Map2Png::text_fightertrainer=QStringLiteral("%2/skin/fighter/%1/trainer.png");
QString Map2Png::text_lookAt=QLatin1Literal("lookAt");
QString Map2Png::text_empty;
QString Map2Png::text_top=QLatin1Literal("top");
QString Map2Png::text_right=QLatin1Literal("right");
QString Map2Png::text_left=QLatin1Literal("left");
QString Map2Png::text_Collisions=QLatin1Literal("Collisions");
QString Map2Png::text_animation=QLatin1Literal("animation");
QString Map2Png::text_dotcomma=QLatin1Literal(";");
QString Map2Png::text_ms=QLatin1Literal("ms");
QString Map2Png::text_frames=QLatin1Literal("frames");
QString Map2Png::text_visible=QLatin1Literal("visible");
QString Map2Png::text_false=QLatin1Literal("false");
QString Map2Png::text_object=QLatin1Literal("object");

using namespace Tiled;

/**
 * Item that represents a map object.
 */
class MapObjectItem : public QGraphicsItem
{
public:
    MapObjectItem(MapObject *mapObject, MapRenderer *renderer,
                  QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
        , mMapObject(mapObject)
        , mRenderer(renderer)
    {}

    QRectF boundingRect() const
    {
        return mRenderer->boundingRect(mMapObject);
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
    {
        const QColor &color = mMapObject->objectGroup()->color();
        mRenderer->drawMapObject(p, mMapObject,
                                 color.isValid() ? color : Qt::darkGray);
    }

private:
    MapObject *mMapObject;
    MapRenderer *mRenderer;
};

/**
 * Item that represents a tile layer.
 */
class TileLayerItem : public QGraphicsItem
{
public:
    TileLayerItem(TileLayer *tileLayer, MapRenderer *renderer,
                  QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
        , mTileLayer(tileLayer)
        , mRenderer(renderer)
    {
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    }

    QRectF boundingRect() const
    {
        return mRenderer->boundingRect(mTileLayer->bounds());
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *)
    {
        mRenderer->drawTileLayer(p, mTileLayer, option->rect);
    }

private:
    TileLayer *mTileLayer;
    MapRenderer *mRenderer;
};

/**
 * Item that represents an object group.
 */
class ObjectGroupItem : public QGraphicsItem
{
public:
    ObjectGroupItem(ObjectGroup *objectGroup, MapRenderer *renderer,
                    QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
    {
        setFlag(QGraphicsItem::ItemHasNoContents);

        // Create a child item for each object
        foreach (MapObject *object, objectGroup->objects())
            new MapObjectItem(object, renderer, this);
    }

    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

/**
 * Item that represents a map.
 */
MapItem::MapItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
}

void MapItem::addMap(Map *map, MapRenderer *renderer)
{
    //align zIndex to Collision Layer
    int index=0;
    while(index<map->layers().size())
    {
        if(map->layers().at(index)->name()==Map2Png::text_Collisions)
            break;
        index++;
    }
    if(index==map->layers().size())
        index=-map->layers().size()-1;
    else
        index=-index;

    // Create a child item for each layer
    foreach (Layer *layer, map->layers()) {
        if (TileLayer *tileLayer = layer->asTileLayer()) {
            TileLayerItem *item=new TileLayerItem(tileLayer, renderer, this);
            item->setZValue(index);
            displayed_layer.insert(map,item);
        } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
            ObjectGroupItem *item=new ObjectGroupItem(objectGroup, renderer, this);
            item->setZValue(index);
            displayed_layer.insert(map,item);
        }
        index++;
    }
}

void MapItem::setMapPosition(Tiled::Map *map,qint16 x,qint16 y)
{
    QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        values.at(index)->setPos(x*16,y*16);
        index++;
    }
}

QRectF MapItem::boundingRect() const
{
    return QRectF();
}

void MapItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}

QStringList Map2Png::listFolder(const QString& folder,const QString& suffix)
{
    QStringList returnList;
    QFileInfoList entryList=QDir(folder+suffix).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnList+=listFolder(folder,suffix+fileInfo.fileName()+Map2Png::text_slash);//put unix separator because it's transformed into that's under windows too
        else if(fileInfo.isFile())
            returnList+=suffix+fileInfo.fileName();
    }
    return returnList;
}

Map2Png::Map2Png(QWidget *parent) :
    QGraphicsView(parent),
    mScene(new QGraphicsScene(this))
{
    regexMs=QRegularExpression(QStringLiteral("^[0-9]{1,5}ms$"));
    regexFrames=QRegularExpression(QStringLiteral("^[0-9]{1,3}frames$"));

    setWindowTitle(tr("Map2Png"));
    //scale(2,2);

    setScene(mScene);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                         | QGraphicsView::DontSavePainterState);
    setBackgroundBrush(Qt::black);
    setFrameStyle(QFrame::NoFrame);

    //viewport()->setAttribute(Qt::WA_StaticContents);
    //setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    hideTheDoors=false;
}



Map2Png::~Map2Png()
{
/*    qDeleteAll(tiledMap->tilesets());
    delete tiledMap;
    delete tiledRender;*/
}

Tiled::Tileset * Map2Png::getTileset(Tiled::Map * map,const QString &file)
{
    Tiled::Tileset *tileset = new Tiled::Tileset(file,16,24,0,0);
    QImage image(file);
    if(image.isNull())
    {
        qDebug() << "Unable to open the tilset image" << file;
        return NULL;
    }
    tileset->loadFromImage(image,file);
    if(!tileset)
    {
        qDebug() << "Unable to open the tilset" << reader.errorString();
        return NULL;
    }
    else
        map->addTileset(tileset);
    return tileset;
}

QString Map2Png::loadOtherMap(const QString &fileName)
{
    Map_full *tempMapObject=new Map_full();
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(other_map.contains(resolvedFileName))
        return resolvedFileName;

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return QString();
    }

    {
        MapVisualiserThread::Map_full *tempMapObjectFull=new MapVisualiserThread::Map_full();
        tempMapObjectFull->tiledMap=tempMapObject->tiledMap;

        //do the object group to move the player on it
        tempMapObjectFull->objectGroup = new Tiled::ObjectGroup(MapVisualiserThread::text_Dyna_management,0,0,tempMapObjectFull->tiledMap->width(),tempMapObjectFull->tiledMap->height());
        tempMapObjectFull->objectGroup->setName("objectGroup for player layer");

        MapVisualiserThread::layerChangeLevelAndTagsChange(tempMapObjectFull,hideTheDoors);

        tempMapObject->objectGroup=tempMapObjectFull->objectGroup;
        tempMapObject->tiledMap=tempMapObjectFull->tiledMap;
    }

    int index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(tempMapObject->tiledMap->layerAt(index)->name()==Map2Png::text_Object && tempMapObject->tiledMap->layerAt(index)->isObjectGroup())
        {
            QList<MapObject *> objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
            int index2=0;
            while(index2<objects.size())
            {
                if(objects.at(index2)->type()==Map2Png::text_bot)
                {
                    if(objects.at(index2)->property(Map2Png::text_skin).isEmpty())
                    {
                        tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->removeObjectAt(index2);
                        objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
                        index2--;
                    }
                    else
                    {
                        Tiled::Tileset * tileset=NULL;
                        QString tilesetPath=Map2Png::text_fightertrainer.arg(objects.at(index2)->property(Map2Png::text_skin)).arg(baseDatapack);
                        if(QFile(tilesetPath).exists())
                            tileset=Map2Png::getTileset(tempMapObject->tiledMap,tilesetPath);
                        else
                        {
                            int entryListIndex=0;
                            while(entryListIndex<folderListSkin.size())
                            {
                                tilesetPath=QStringLiteral("%1/skin/%2/%3/trainer.png").arg(baseDatapack).arg(folderListSkin.at(entryListIndex)).arg(objects.at(index2)->property(Map2Png::text_skin));
                                if(QFile(tilesetPath).exists())
                                {
                                    tileset=Map2Png::getTileset(tempMapObject->tiledMap,tilesetPath);
                                    break;
                                }
                                entryListIndex++;
                            }
                            if(entryListIndex>=folderListSkin.size())
                            {
                                qDebug() << "Skin bot not found: " << tilesetPath;
                                //abort();
                            }
                        }
                        if(tileset!=NULL)
                        {
                            QString lookAt=objects.at(index2)->property(Map2Png::text_lookAt);
                            QPointF position=objects.at(index2)->position();
                            objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
                            tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->removeObjectAt(index2);
                            MapObject *object=new MapObject(Map2Png::text_empty,Map2Png::text_empty,position,QSizeF(1,1));
                            tempMapObject->objectGroup->addObject(object);
                            objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
                            index2--;
                            Cell cell=object->cell();
                            if(lookAt==Map2Png::text_top)
                                cell.tile=tileset->tileAt(1);
                            else if(lookAt==Map2Png::text_right)
                                cell.tile=tileset->tileAt(4);
                            else if(lookAt==Map2Png::text_left)
                                cell.tile=tileset->tileAt(10);
                            else
                                cell.tile=tileset->tileAt(7);
                            object->setCell(cell);
                        }
                    }
                }
                else if(objects.at(index2)->type()==Map2Png::text_object)
                {
                    if(objects.at(index2)->property(Map2Png::text_visible)==Map2Png::text_false)
                    {
                        tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->removeObjectAt(index2);
                        objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
                        index2--;
                    }
                }
                index2++;
            }
        }
        index++;
    }
    CatchChallenger::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName.toStdString()))
    {
        mLastError=QString::fromStdString(map_loader.errorString());
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(QString::fromStdString(map_loader.errorString()));
        delete tempMapObject->tiledMap;
        return QString();
    }

    //copy the variables
    tempMapObject->logicalMap.width                                 = map_loader.map_to_send.width;
    tempMapObject->logicalMap.height                                = map_loader.map_to_send.height;
    tempMapObject->logicalMap.parsed_layer                          = map_loader.map_to_send.parsed_layer;
    tempMapObject->logicalMap.map_file                              = resolvedFileName.toStdString();
    tempMapObject->logicalMap.border.bottom.map                     = NULL;
    tempMapObject->logicalMap.border.top.map                        = NULL;
    tempMapObject->logicalMap.border.right.map                      = NULL;
    tempMapObject->logicalMap.border.left.map                       = NULL;

    //load the string
    tempMapObject->logicalMap.border_semi                = map_loader.map_to_send.border;
    if(!map_loader.map_to_send.border.bottom.fileName.empty())
        tempMapObject->logicalMap.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+Map2Png::text_slash+QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.top.fileName.empty())
        tempMapObject->logicalMap.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+Map2Png::text_slash+QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.right.fileName.empty())
        tempMapObject->logicalMap.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+Map2Png::text_slash+QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.left.fileName.empty())
        tempMapObject->logicalMap.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+Map2Png::text_slash+QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName)).absoluteFilePath().toStdString();

    //load the render
    switch (tempMapObject->tiledMap->orientation()) {
    case Map::Isometric:
        tempMapObject->tiledRender = new IsometricRenderer(tempMapObject->tiledMap);
        break;
    case Map::Orthogonal:
    default:
        tempMapObject->tiledRender = new OrthogonalRenderer(tempMapObject->tiledMap);
        break;
    }

    other_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void Map2Png::loadCurrentMap(const QString &fileName, qint32 x, qint32 y)
{
    //qDebug() << QStringLiteral("loadCurrentMap(%1)").arg(fileName);
    Map_full *tempMapObject;
    if(!other_map.contains(fileName))
    {
        qDebug() << QStringLiteral("loadCurrentMap(): the current map is unable to load: %1").arg(fileName);
        return;
    }
    else
        tempMapObject=other_map.value(fileName);

    tempMapObject->x=x;
    tempMapObject->y=y;

    QString mapIndex;

    if(mRenderAll)
    {
        //if have border
        if(!tempMapObject->logicalMap.border_semi.bottom.fileName.empty())
        {
            if(!other_map.contains(QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName)))
            {
                mapIndex=loadOtherMap(QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName));
                //if is correctly loaded
                if(!mapIndex.isEmpty())
                {
                    //if both border match
                    if(fileName.toStdString()==other_map.value(mapIndex)->logicalMap.border_semi.top.fileName && tempMapObject->logicalMap.border_semi.bottom.fileName==mapIndex.toStdString())
                    {
                        tempMapObject->logicalMap.border.bottom.map=&other_map[mapIndex]->logicalMap;
                        int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-other_map.value(mapIndex)->logicalMap.border_semi.top.x_offset;
                        tempMapObject->logicalMap.border.bottom.x_offset=offset;
                        other_map[mapIndex]->logicalMap.border.top.x_offset=-offset;
                        loadCurrentMap(mapIndex,tempMapObject->x+offset,tempMapObject->y+tempMapObject->logicalMap.height);
                    }
                }
            }
        }

        //if have border
        if(!tempMapObject->logicalMap.border_semi.top.fileName.empty())
        {
            if(!other_map.contains(QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName)))
            {
                mapIndex=loadOtherMap(QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName));
                //if is correctly loaded
                if(!mapIndex.isEmpty())
                {
                    //if both border match
                    if(fileName.toStdString()==other_map.value(mapIndex)->logicalMap.border_semi.bottom.fileName && tempMapObject->logicalMap.border_semi.top.fileName==mapIndex.toStdString())
                    {
                        tempMapObject->logicalMap.border.top.map=&other_map[mapIndex]->logicalMap;
                        int offset=tempMapObject->logicalMap.border_semi.top.x_offset-other_map.value(mapIndex)->logicalMap.border_semi.bottom.x_offset;
                        tempMapObject->logicalMap.border.top.x_offset=offset;
                        other_map[mapIndex]->logicalMap.border.bottom.x_offset=-offset;
                        loadCurrentMap(mapIndex,tempMapObject->x+offset,tempMapObject->y-other_map.value(mapIndex)->logicalMap.height);
                    }
                }
            }
        }

        //if have border
        if(!tempMapObject->logicalMap.border_semi.left.fileName.empty())
        {
            if(!other_map.contains(QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName)))
            {
                mapIndex=loadOtherMap(QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName));
                //if is correctly loaded
                if(!mapIndex.isEmpty())
                {
                    //if both border match
                    if(fileName.toStdString()==other_map.value(mapIndex)->logicalMap.border_semi.right.fileName && tempMapObject->logicalMap.border_semi.left.fileName==mapIndex.toStdString())
                    {
                        tempMapObject->logicalMap.border.left.map=&other_map[mapIndex]->logicalMap;
                        int offset=tempMapObject->logicalMap.border_semi.left.y_offset-other_map.value(mapIndex)->logicalMap.border_semi.right.y_offset;
                        tempMapObject->logicalMap.border.left.y_offset=offset;
                        other_map[mapIndex]->logicalMap.border.right.y_offset=-offset;
                        loadCurrentMap(mapIndex,tempMapObject->x-other_map.value(mapIndex)->logicalMap.width,tempMapObject->y+offset);
                    }
                }
            }
        }

        //if have border
        if(!tempMapObject->logicalMap.border_semi.right.fileName.empty())
        {
            if(!other_map.contains(QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName)))
            {
                mapIndex=loadOtherMap(QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName));
                //if is correctly loaded
                if(!mapIndex.isEmpty())
                {
                    //if both border match
                    if(fileName.toStdString()==other_map.value(mapIndex)->logicalMap.border_semi.left.fileName && tempMapObject->logicalMap.border_semi.right.fileName==mapIndex.toStdString())
                    {
                        tempMapObject->logicalMap.border.right.map=&other_map[mapIndex]->logicalMap;
                        int offset=tempMapObject->logicalMap.border_semi.right.y_offset-other_map.value(mapIndex)->logicalMap.border_semi.left.y_offset;
                        tempMapObject->logicalMap.border.right.y_offset=offset;
                        other_map[mapIndex]->logicalMap.border.left.y_offset=-offset;
                        loadCurrentMap(mapIndex,tempMapObject->x+tempMapObject->logicalMap.width,tempMapObject->y+offset);
                    }
                }
            }
        }
    }
}

void Map2Png::viewMap(const bool &renderAll,const QString &fileName,const QString &destination)
{
    /*QTime startTime;
    startTime.restart();*/

    mRenderAll=renderAll;
    mapItem=new MapItem();
    mScene->clear();
    centerOn(0, 0);

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
    {
        qDebug() << mLastError;
        return;
    }
    loadCurrentMap(current_map_fileName,0,0);

    displayMap();
    //qDebug() << startTime.elapsed();
    mScene->addItem(mapItem);

    QSizeF size=mScene->sceneRect().size();
    QImage newImage(QSize(size.width(),size.height()),QImage::Format_ARGB32);
    newImage.fill(Qt::transparent);
    QPainter painter(&newImage);
    mScene->render(&painter);//,mScene->sceneRect()
    //qDebug() << QStringLiteral("mScene size: %1,%2").arg(mScene->sceneRect().size().width()).arg(mScene->sceneRect().size().height());
    //qDebug() << QStringLiteral("save as: %1").arg(destination);

    if(!destination.isEmpty())
    {
        QDir().mkpath(QFileInfo(destination).absolutePath());
        if(!newImage.save(destination))
            qDebug() << QStringLiteral("Unable to save the image")+destination;
    }
}

void Map2Png::displayMap()
{
    //qDebug() << QStringLiteral("displayMap()");

    QHash<QString,Map_full *>::const_iterator i = other_map.constBegin();
     while (i != other_map.constEnd()) {
         //qDebug() << QStringLiteral("displayMap(): %1 at %2,%3").arg(i.key()).arg(i.value()->x).arg(i.value()->y);
         mapItem->addMap(i.value()->tiledMap,i.value()->tiledRender);
         mapItem->setMapPosition(i.value()->tiledMap,i.value()->x,i.value()->y);
         ++i;
     }
}
