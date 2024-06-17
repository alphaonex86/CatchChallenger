/*
1) Detect the index where create the group Object
2) create new GroupObject
3) detect Object class "bot" in ObjectGroup "Object", detect their x,y
4) delete Object of 3
5) create Object at x,y from 3) but with correct image (get from properties of 3)) at groupObject of 2)
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

#include "../../general/base/MoveOnTheMap.hpp"
#include <tile.h>
#include "MapDoor.hpp"
#include "TriggerAnimation.hpp"

using namespace Tiled;

QRegularExpression MapVisualiserOrder::regexMs=QRegularExpression(QStringLiteral("^[0-9]{1,5}ms$"));
QRegularExpression MapVisualiserOrder::regexFrames=QRegularExpression(QStringLiteral("^[0-9]{1,3}frames$"));
QRegularExpression MapVisualiserOrder::regexTrigger=QRegularExpression("^start:([0-9]+)ms;([0-9]+)frames;leave:([0-9]+)ms;([0-9]+)frames;?(.*)$");
QRegularExpression MapVisualiserOrder::regexTriggerAgain=QRegularExpression("^again:([0-9]+)ms;([0-9]+)frames;?(.*)$");

MapVisualiserOrder::MapVisualiserOrder()
{
}

MapVisualiserOrder::~MapVisualiserOrder()
{
}

Map_full::Map_full()
{
    logicalMap.xmlRoot=NULL;
    logicalMap.border.bottom.map=NULL;
    logicalMap.border.bottom.x_offset=0;
    logicalMap.border.top.map=NULL;
    logicalMap.border.top.x_offset=0;
    logicalMap.border.right.map=NULL;
    logicalMap.border.right.y_offset=0;
    logicalMap.border.left.map=NULL;
    logicalMap.border.left.y_offset=0;
    logicalMap.teleporter=NULL;
    logicalMap.teleporter_list_size=0;
    logicalMap.width=0;
    logicalMap.height=0;
    logicalMap.group=0;
    logicalMap.id=0;
    tiledMap=NULL;
    tiledRender=NULL;
    objectGroup=NULL;
    objectGroupIndex=0;
    relative_x=0;
    relative_y=0;
    relative_x_pixel=0;
    relative_y_pixel=0;
    displayed=0;
}

void MapVisualiserOrder::layerChangeLevelAndTagsChange(Map_full *tempMapObject,bool hideTheDoors)
{
    //remove the hidden tags, and unknown layer
    int index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            //remove the unknow layer
            if(objectGroup->name()=="Moving")
            {
                QList<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    const uint32_t &x=objects.at(index2)->x();
                    const uint32_t &y=objects.at(index2)->y()-1;
                    //remove the unknow object
                    if(objects.at(index2)->type()=="door")
                    {
                        if(hideTheDoors)
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        else
                        {
                            const Tiled::Tile *tile=objects.at(index2)->cell().tile();
                            if(tile!=NULL)
                            {
                                //animation only for door
                                const QString &animation=tile->property("animation").toString();
                                if(!animation.isEmpty())
                                {
                                    const QStringList &animationList=animation.split(";");
                                    if(animationList.size()==2)
                                    {
                                        if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                        {
                                            QString msString=animationList.at(0);
                                            QString framesString=animationList.at(1);
                                            msString.remove("ms");
                                            framesString.remove("frames");
                                            uint16_t ms=msString.toUShort();
                                            uint8_t frames=static_cast<uint8_t>(framesString.toUShort());
                                            if(ms>0 && frames>1)
                                            {
                                                if(tempMapObject->doors.find(
                                                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))
                                                            )==
                                                        tempMapObject->doors.cend())
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=door;
                                                }
                                            }
                                            else
                                                qDebug() << "ms is 0 or frame is <=1" << animation;
                                        }
                                        else
                                            qDebug() << "Wrong animation tile args regex match" << animation;
                                    }
                                    else
                                        qDebug() << "Wrong animation tile args count (Order): " << animation;
                                }
                                else
                                {
                                    qDebug() << "animation properties not found for the door at " << x << "," << y;
                                    objectGroup->removeObject(objects.at(index2));
                                    delete objects.at(index2);
                                }
                            }
                        }
                    }
                    else if(objects.at(index2)->type()=="border-bottom" ||
                            objects.at(index2)->type()=="border-top" ||
                            objects.at(index2)->type()=="border-right" ||
                            objects.at(index2)->type()=="border-left" ||
                            objects.at(index2)->type()=="rescue" ||
                            objects.at(index2)->type()=="door" ||
                            objects.at(index2)->type()=="teleport on push" ||
                            objects.at(index2)->type()=="teleport on it")
                    {
                        qDebug() << "Layer Moving, TP object at " << x << "," << y << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    else
                    {
                        qDebug() << "Layer Moving, unknown object at " << x << "," << y << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    index2++;
                }
                index++;
            }
            else if(objectGroup->name()=="Object")
            {
                QList<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    //remove the bot
                    if(objects.at(index2)->type()=="bot")
                    {
                        qDebug() << "Layer Moving, bot object at " << objects.at(index2)->x() << "," << objects.at(index2)->y() << " type: " << objects.at(index2)->type();
                        /// \see MapController::loadBotOnTheMap()
                        #ifndef ONLYMAPRENDER
                        /*objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);*/
                        #endif
                    }
                    else if(objects.at(index2)->type()=="object")
                    {}
                    //remove the unknow object
                    else
                    {
                        qDebug() << "layer Object, unknown object at " << objects.at(index2)->x() << "," << objects.at(index2)->y() << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    index2++;
                }
                index++;
            }
            else
                delete tempMapObject->tiledMap->takeLayerAt(index);
        }
        /// for the animation of tile see MapVisualiser::applyTheAnimationTimer()
        else
            index++;
    }


    //search WalkBehind layer
    {
        int index=0;
        const int &listSize=tempMapObject->tiledMap->layerCount();
        while(index<listSize)
        {
            const Tiled::Layer * layer=tempMapObject->tiledMap->layerAt(index);
            if(layer->name()=="WalkBehind")
            {
                tempMapObject->objectGroupIndex=index;
                tempMapObject->tiledMap->insertLayer(index,tempMapObject->objectGroup);
                break;
            }
            index++;
        }
        if(index==listSize)
        {
            //search Collisions layer
            index=listSize-1;
            while(index>=0)
            {
                if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
                {
                    tempMapObject->objectGroupIndex=index+1;
                    tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
                    break;
                }
                index--;
            }
            if(index<0)
            {
                qDebug() << QStringLiteral("Unable to locate the \"Collisions\" layer on the map: %1").arg(QString::fromStdString(tempMapObject->logicalMap.map_file));
                tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
            }
        }
    }

    //move the Moving layer on dyna layer
    {
        int index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
            {
                if(objectGroup->name()=="Moving")
                {
                    Tiled::Layer *layer = tempMapObject->tiledMap->takeLayerAt(index);
                    if(tempMapObject->objectGroupIndex-1<=0)
                        tempMapObject->tiledMap->insertLayer(0,layer);
                    else
                    {
                        if(index>tempMapObject->objectGroupIndex)
                            tempMapObject->objectGroupIndex++;
                        tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,layer);
                    }
                }
            }
            index++;
        }
    }
}



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
    if(displayed_layer.size()>10000000)
        abort();
    setFlag(QGraphicsItem::ItemHasNoContents);
}

void MapItem::addMap(Map *map, MapRenderer *renderer)
{
    //align zIndex to Collision Layer
    int index=0;
    while(index<map->layers().size())
    {
        if(map->layers().at(index)->name()=="Collisions")
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
        if(displayed_layer.size()>10000000)
            abort();
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
            returnList+=listFolder(folder,suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
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
/* lib tiled do it them self, uncomment generate double free
 *     qDeleteAll(tiledMap->tilesets());
    delete tiledMap;
    delete tiledRender;*/
}

Tiled::SharedTileset Map2Png::getTileset(Tiled::Map * map,const QString &file)
{
    qDebug() << file;
    Tiled::SharedTileset tileset = Tiled::Tileset::create(file,16,24,0,0);
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

void Map2Png::layerChangeLevelAndTagsChange(Map_full *tempMapObject,bool hideTheDoors)
{
    QRegularExpression regexMs=QRegularExpression(QStringLiteral("^[0-9]{1,5}ms$"));
    QRegularExpression regexFrames=QRegularExpression(QStringLiteral("^[0-9]{1,3}frames$"));
    QRegularExpression regexTrigger=QRegularExpression("^start:([0-9]+)ms;([0-9]+)frames;leave:([0-9]+)ms;([0-9]+)frames;?(.*)$");
    QRegularExpression regexTriggerAgain=QRegularExpression("^again:([0-9]+)ms;([0-9]+)frames;?(.*)$");

    //remove the hidden tags, and unknown layer
    int index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            //remove the unknow layer
            if(objectGroup->name()=="Moving")
            {
                QList<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    const uint32_t &x=objects.at(index2)->x();
                    const uint32_t &y=objects.at(index2)->y()-1;
                    //remove the unknow object
                    if(objects.at(index2)->type()=="door")
                    {
                        if(hideTheDoors)
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        else
                        {
                            const Tiled::Tile *tile=objects.at(index2)->cell().tile();
                            if(tile!=NULL)
                            {
                                //animation only for door
                                const QString &animation=tile->property("animation").toString();
                                if(!animation.isEmpty())
                                {
                                    const QStringList &animationList=animation.split(";");
                                    if(animationList.size()==2)
                                    {
                                        if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                        {
                                            QString msString=animationList.at(0);
                                            QString framesString=animationList.at(1);
                                            msString.remove("ms");
                                            framesString.remove("frames");
                                            uint16_t ms=msString.toUShort();
                                            uint8_t frames=static_cast<uint8_t>(framesString.toUShort());
                                            if(ms>0 && frames>1)
                                            {
                                                if(tempMapObject->doors.find(
                                                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))
                                                            )==
                                                        tempMapObject->doors.cend())
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=door;
                                                }
                                            }
                                            else
                                                qDebug() << "ms is 0 or frame is <=1" << animation;
                                        }
                                        else
                                            qDebug() << "Wrong animation tile args regex match" << animation;
                                    }
                                    else
                                        qDebug() << "Wrong animation tile args count (Order): " << animation;
                                }
                                else
                                {
                                    qDebug() << "animation properties not found for the door at " << x << "," << y;
                                    objectGroup->removeObject(objects.at(index2));
                                    delete objects.at(index2);
                                }
                            }
                        }
                    }
                    else if(objects.at(index2)->type()=="border-bottom" ||
                            objects.at(index2)->type()=="border-top" ||
                            objects.at(index2)->type()=="border-right" ||
                            objects.at(index2)->type()=="border-left" ||
                            objects.at(index2)->type()=="rescue" ||
                            objects.at(index2)->type()=="door" ||
                            objects.at(index2)->type()=="teleport on push" ||
                            objects.at(index2)->type()=="teleport on it")
                    {
                        qDebug() << "Layer Moving, TP object at " << x << "," << y << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    else
                    {
                        qDebug() << "Layer Moving, unknown object at " << x << "," << y << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    index2++;
                }
                index++;
            }
            else if(objectGroup->name()=="Object")
            {
                QList<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    //remove the bot
                    if(objects.at(index2)->type()=="bot")
                    {
                        qDebug() << "layer Object, bot object at " << objects.at(index2)->x() << "," << objects.at(index2)->y() << " type: " << objects.at(index2)->type();
                        /// \see MapController::loadBotOnTheMap()
                        #ifndef ONLYMAPRENDER
                        /*objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);*/
                        #endif
                    }
                    else if(objects.at(index2)->type()=="object")
                    {}
                    //remove the unknow object
                    else
                    {
                        qDebug() << "layer Object, unknown object at " << objects.at(index2)->x() << "," << objects.at(index2)->y() << " type: " << objects.at(index2)->type();
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    index2++;
                }
                index++;
            }
            else
                delete tempMapObject->tiledMap->takeLayerAt(index);
        }
        /// for the animation of tile see MapVisualiser::applyTheAnimationTimer()
        else
            index++;
    }


    //search WalkBehind layer
    {
        int index=0;
        const int &listSize=tempMapObject->tiledMap->layerCount();
        while(index<listSize)
        {
            const Tiled::Layer * layer=tempMapObject->tiledMap->layerAt(index);
            if(layer->name()=="WalkBehind")
            {
                tempMapObject->objectGroupIndex=index;
                tempMapObject->tiledMap->insertLayer(index,tempMapObject->objectGroup);
                break;
            }
            index++;
        }
        if(index==listSize)
        {
            //search Collisions layer
            index=listSize-1;
            while(index>=0)
            {
                if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
                {
                    tempMapObject->objectGroupIndex=index+1;
                    tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
                    break;
                }
                index--;
            }
            if(index<0)
            {
                qDebug() << QStringLiteral("Unable to locate the \"Collisions\" layer on the map: %1").arg(QString::fromStdString(tempMapObject->logicalMap.map_file));
                tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
            }
        }
    }

    //move the Moving layer on dyna layer
    {
        int index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
            {
                if(objectGroup->name()=="Moving")
                {
                    Tiled::Layer *layer = tempMapObject->tiledMap->takeLayerAt(index);
                    if(tempMapObject->objectGroupIndex-1<=0)
                        tempMapObject->tiledMap->insertLayer(0,layer);
                    else
                    {
                        if(index>tempMapObject->objectGroupIndex)
                            tempMapObject->objectGroupIndex++;
                        tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,layer);
                    }
                }
            }
            index++;
        }
    }
}

QString Map2Png::loadOtherMap(const QString &fileName)
{
    Map_full_map2png *tempMapObject=new Map_full_map2png();
    tempMapObject->logicalMap.group=0;
    tempMapObject->logicalMap.height=0;
    tempMapObject->logicalMap.id=0;
    tempMapObject->logicalMap.width=0;
    tempMapObject->logicalMap.teleporter=nullptr;
    tempMapObject->logicalMap.teleporter_list_size=0;
    tempMapObject->logicalMap.xmlRoot=nullptr;
    tempMapObject->objectGroup=nullptr;
    tempMapObject->tiledMap=nullptr;
    tempMapObject->tiledRender=nullptr;
    tempMapObject->x=0;
    tempMapObject->y=0;
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(other_map.contains(resolvedFileName))
        return resolvedFileName;

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2, from %3").arg(resolvedFileName).arg(reader.errorString()).arg(fileName);
        delete tempMapObject;
        return QString();
    }

    {
        Map_full *tempMapObjectFull=new Map_full();
        tempMapObjectFull->tiledMap=tempMapObject->tiledMap;

        //do the object group to move the player on it
        tempMapObjectFull->objectGroup = new Tiled::ObjectGroup("text_Dyna_management",0,0);//,tempMapObjectFull->tiledMap->width(),tempMapObjectFull->tiledMap->height()
        tempMapObjectFull->objectGroup->setName("objectGroup for player layer");

        Map2Png::layerChangeLevelAndTagsChange(tempMapObjectFull,hideTheDoors);

        tempMapObject->objectGroup=tempMapObjectFull->objectGroup;
        tempMapObject->tiledMap=tempMapObjectFull->tiledMap;
    }

    int index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(tempMapObject->tiledMap->layerAt(index)->name()=="Object" && tempMapObject->tiledMap->layerAt(index)->isObjectGroup())
        {
            QList<MapObject *> objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
            int index2=0;
            while(index2<objects.size())
            {
                if(objects.at(index2)->type()=="bot")
                {
                    //qDebug() << "bot found at " << __FILE__ << ":" << __LINE__;
                    if(objects.at(index2)->property("skin").toString().isEmpty())
                    {
                        qDebug() << "bot found but no skin at " << __FILE__ << ":" << __LINE__ << " on map " << resolvedFileName << objects.at(index2)->x()/16 << objects.at(index2)->y()/16-1 << objects.at(index2)->property("skin").toString();
                        tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->removeObjectAt(index2);
                        objects=tempMapObject->tiledMap->layerAt(index)->asObjectGroup()->objects();
                        index2--;
                    }
                    else
                    {
                        Tiled::SharedTileset tileset=NULL;
                        QString tilesetPath=QString("%2/skin/fighter/%1/trainer.png").arg(objects.at(index2)->property("skin").toString()).arg(baseDatapack);
                        if(QFile(tilesetPath).exists())
                            tileset=Map2Png::getTileset(tempMapObject->tiledMap.get(),tilesetPath);
                        else
                        {
                            int entryListIndex=0;
                            while(entryListIndex<folderListSkin.size())
                            {
                                tilesetPath=QStringLiteral("%1/skin/%2/%3/trainer.png").arg(baseDatapack).arg(folderListSkin.at(entryListIndex)).arg(objects.at(index2)->property("skin").toString());
                                if(QFile(tilesetPath).exists())
                                {
                                    tileset=Map2Png::getTileset(tempMapObject->tiledMap.get(),tilesetPath);
                                    break;
                                }
                                entryListIndex++;
                            }
                            if(entryListIndex>=folderListSkin.size())
                            {
                                qDebug() << "Skin bot not found: " << tilesetPath << " on map " << resolvedFileName << objects.at(index2)->x() << objects.at(index2)->y();
                                entryListIndex=0;
                                while(entryListIndex<folderListSkin.size())
                                {
                                    tilesetPath=QStringLiteral("%1/skin/%2/%3/trainer.png").arg(baseDatapack).arg(folderListSkin.at(entryListIndex)).arg(objects.at(index2)->property("skin").toString());
                                    qDebug() << "Skin bot not found into: " << tilesetPath;
                                    entryListIndex++;
                                }
                                //abort();
                            }
                        }
                        if(tileset!=NULL)
                        {
                            if(tileset.isNull())
                                qDebug() << "insert bot but tileset.isNull() at " << __FILE__ << ":" << __LINE__ << " on map " << resolvedFileName << objects.at(index2)->x() << objects.at(index2)->y() << objects.at(index2)->property("skin").toString();
                            else
                                qDebug() << "insert bot at " << __FILE__ << ":" << __LINE__ << " on map " << resolvedFileName << objects.at(index2)->x() << objects.at(index2)->y() << objects.at(index2)->property("skin").toString();
                            QString lookAt=objects.at(index2)->property("lookAt").toString();
                            Layer * layer=tempMapObject->tiledMap->layerAt(index);
                            ObjectGroup *objGrou=layer->asObjectGroup();

                            Cell cell=objects.at(index2)->cell();
                            if(lookAt=="top")
                                cell.setTile(tileset->tileAt(1));
                            else if(lookAt=="right")
                                cell.setTile(tileset->tileAt(4));
                            else if(lookAt=="left")
                                cell.setTile(tileset->tileAt(10));
                            else
                                cell.setTile(tileset->tileAt(7));
                            objects.at(index2)->setCell(cell);
                            if(objects.at(index2)->hasDimensions())
                                objects.at(index2)->setSize(tileset->tileAt(1)->width(),tileset->tileAt(1)->height());
                            //tempMapObject->objectGroup->addObject(objects.at(index2));

                            if(tempMapObject->objectGroup!=nullptr)
                            {
                                tempMapObject->objectGroup->addObject(objects.at(index2));
                                objGrou->removeObject(objects.at(index2));
                                index2--;
                            }
                            else
                                qDebug() << "insert bot but tempMapObject->objectGroup==null at " << __FILE__ << ":" << __LINE__ << " on map " << resolvedFileName << objects.at(index2)->x() << objects.at(index2)->y() << objects.at(index2)->property("skin").toString();
                        }
                        else
                            qDebug() << "bot found tileset NULL at " << __FILE__ << ":" << __LINE__;
                    }
                }
                else if(objects.at(index2)->type()=="object")
                {
                    if(objects.at(index2)->property("visible")=="false")
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
        //delete tempMapObject->tiledMap;
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
        tempMapObject->logicalMap.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.top.fileName.empty())
        tempMapObject->logicalMap.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.right.fileName.empty())
        tempMapObject->logicalMap.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.left.fileName.empty())
        tempMapObject->logicalMap.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName)).absoluteFilePath().toStdString();

    //load the render
    switch (tempMapObject->tiledMap->orientation()) {
    case Map::Isometric:
        tempMapObject->tiledRender = new IsometricRenderer(tempMapObject->tiledMap.get());
        //qDebug() << QStringLiteral("render Map::Isometric");
        break;
    case Map::Orthogonal:
    default:
        tempMapObject->tiledRender = new OrthogonalRenderer(tempMapObject->tiledMap.get());
        //qDebug() << QStringLiteral("render Map::Orthogonal");
        break;
    }

    if(tempMapObject->tiledRender->mapBoundingRect().isEmpty())
        qDebug() << QStringLiteral("displayMap(): empty map");
    other_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void Map2Png::loadCurrentMap(const QString &fileName, qint32 x, qint32 y)
{
    //qDebug() << QStringLiteral("loadCurrentMap(%1)").arg(fileName);
    Map_full_map2png *tempMapObject;
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
                else
                    qDebug() << "Failed link from " << fileName << " to " << QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName);
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
                else
                    qDebug() << "Failed link from " << fileName << " to " << QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName);
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
                else
                    qDebug() << "Failed link from " << fileName << " to " << QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName);
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
                else
                    qDebug() << "Failed link from " << fileName << " to " << QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName);
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
    /*qDebug() << QStringLiteral("mScene size: %1,%2").arg(mScene->sceneRect().size().width()).arg(mScene->sceneRect().size().height());
    qDebug() << QStringLiteral("save as: %1").arg(destination);*/

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

    QHash<QString,Map_full_map2png *>::const_iterator i = other_map.constBegin();
     while (i != other_map.constEnd()) {
         Map_full_map2png *m=i.value();
         if(m==nullptr)
             qDebug() << QStringLiteral("displayMap(): buggy map");
         else
         {
             if(m->tiledRender->mapBoundingRect().isEmpty())
                 qDebug() << QStringLiteral("displayMap(): empty map");
             //qDebug() << QStringLiteral("displayMap(): %1 at %2,%3").arg(i.key()).arg(i.value()->x).arg(i.value()->y);
             mapItem->addMap(m->tiledMap.get(),m->tiledRender);
             mapItem->setMapPosition(m->tiledMap.get(),m->x,m->y);
         }
         ++i;
     }
}
