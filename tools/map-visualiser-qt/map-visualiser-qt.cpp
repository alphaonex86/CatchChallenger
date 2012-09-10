#include "map-visualiser-qt.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/MoveOnTheMap.h"

//setParent(ObjectGroupItem)

using namespace Tiled;

/**
 * Item that represents a map object.
 */
class MapObjectItem : public QGraphicsItem
{
public:
    MapObjectItem(MapObject *mapObject,
                  QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
        , mMapObject(mapObject)
    {}

    QRectF boundingRect() const
    {
        ObjectGroup * objectGroup=mMapObject->objectGroup();
        if(mRendererList.contains(objectGroup))
            return mRendererList[objectGroup]->boundingRect(mMapObject);
        else
            return QRectF();
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
    {
        if(!mMapObject->isVisible())
            return;
        if(mMapObject->objectGroup()!=NULL)
            if(!mMapObject->objectGroup()->isVisible())
                return;
        const QColor &color = mMapObject->objectGroup()->color();
        mRendererList[mMapObject->objectGroup()]->drawMapObject(p, mMapObject,
                                 color.isValid() ? color : Qt::darkGray);
    }

private:
    MapObject *mMapObject;
public:
    //interresting if have lot of object by group layer
    static QHash<ObjectGroup *,MapRenderer *> mRendererList;
};
QHash<ObjectGroup *,MapRenderer *> MapObjectItem::mRendererList;

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
ObjectGroupItem::ObjectGroupItem(ObjectGroup *objectGroup,
                QGraphicsItem *parent)
    : QGraphicsItem(parent),
      mObjectGroup(objectGroup)
{
    //setFlag(QGraphicsItem::ItemHasNoContents);
}

QRectF ObjectGroupItem::boundingRect() const
{
    return QRectF();
}

void ObjectGroupItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
    updateObject();
}

void ObjectGroupItem::updateObject()
{
    // Create a child item for each object
    QList<QGraphicsItem *> childs=childItems();
    int index=0;
    while(index<childs.size())
    {
        delete childs.at(index);
        index++;
    }
    foreach (MapObject *object, mObjectGroup->objects())
        new MapObjectItem(object, this);
}

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
    QList<Layer *> layers=map->layers();
    while(index<layers.size())
    {
        if(layers.at(index)->name()=="Collisions")
            break;
        index++;
    }
    if(index==map->layers().size())
        index=-map->layers().size()-1;
    else
        index=-index;

    // Create a child item for each layer
    foreach (Layer *layer, layers) {
        Map_graphics_link tempItem;
        if (TileLayer *tileLayer = layer->asTileLayer()) {
            tempItem.graphic=new TileLayerItem(tileLayer, renderer, this);
            tempItem.layer=tileLayer;
        } else if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
            MapObjectItem::mRendererList[objectGroup]=renderer;
            ObjectGroupItem *newObject=new ObjectGroupItem(objectGroup, this);
            tempItem.graphic=newObject;
            tempItem.layer=objectGroup;
            ObjectGroupItemList.insert(map,newObject);
        }
        tempItem.graphic->setZValue(index++);
        displayed_layer.insert(map,tempItem);
    }
}

void MapItem::removeMap(Map *map)
{
    QList<Map_graphics_link> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        if (ObjectGroup *objectGroup = values.at(index).layer->asObjectGroup())
            MapObjectItem::mRendererList.remove(objectGroup);
        delete values.at(index).graphic;
        index++;
    }
    displayed_layer.remove(map);
}

void MapItem::setMapPosition(Tiled::Map *map,qint16 x,qint16 y)
{
    QList<Map_graphics_link> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        values.at(index).graphic->setPos(x*TILE_SIZE,y*TILE_SIZE);
        index++;
    }

    QList<ObjectGroupItem *> values2 = ObjectGroupItemList.values(map);
    index=0;
    while(index<values2.size())
    {
        qDebug() << QString("mObjectGroup->name(): %1").arg(values2.at(index)->mObjectGroup->name());
        values2.at(index)->updateObject();
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

MapVisualiserQt::MapVisualiserQt(QWidget *parent) :
    QGraphicsView(parent),
    mScene(new QGraphicsScene(this)),
    inMove(false)
{
    xPerso=0;
    yPerso=0;

    lookToMove.setInterval(200);
    lookToMove.setSingleShot(true);
    connect(&lookToMove,SIGNAL(timeout()),this,SLOT(transformLookToMove()));

    moveTimer.setInterval(66);
    moveTimer.setSingleShot(true);
    connect(&moveTimer,SIGNAL(timeout()),this,SLOT(moveStepSlot()));
    setWindowTitle(tr("map-visualiser-qt"));
    //scale(2,2);

    setScene(mScene);
/*    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);*/
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                         | QGraphicsView::DontSavePainterState);
    setBackgroundBrush(Qt::black);
    setFrameStyle(QFrame::NoFrame);

    viewport()->setAttribute(Qt::WA_StaticContents);
    //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

MapVisualiserQt::~MapVisualiserQt()
{
    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = other_map.constBegin();
    while (i != other_map.constEnd()) {
        delete (*i)->logicalMap.parsed_layer.walkable;
        delete (*i)->logicalMap.parsed_layer.water;
        qDeleteAll((*i)->tiledMap->tilesets());
        delete (*i)->tiledMap;
        delete (*i)->tiledRender;
        delete (*i);
        other_map.remove((*i)->logicalMap.map_file);
        i = other_map.constBegin();//needed
    }

    delete mapItem;
    delete playerTileset;
    //delete playerMapObject;
    delete tagTileset;
}

void MapVisualiserQt::viewMap(const QString &fileName)
{
    current_map=NULL;

    QTime startTime;
    startTime.restart();

    mapItem=new MapItem();

    playerTileset = new Tileset("player",16,24);
    playerTileset->loadFromImage(QImage(":/player_skin.png"),":/player_skin.png");
    playerMapObject = new MapObject();

    tagTilesetIndex=0;
    tagTileset = new Tileset("tags",16,16);
    tagTileset->loadFromImage(QImage(":/tags.png"),":/tags.png");

    //commented to not blink
    //blink_dyna_layer.start(200);
    connect(&blink_dyna_layer,SIGNAL(timeout()),this,SLOT(blinkDynaLayer()));

    mScene->clear();
    //centerOn(0, 0);

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
        return;
    current_map=other_map[current_map_fileName];
    other_map.remove(current_map_fileName);

    //the direction
    direction=Pokecraft::Direction_look_at_bottom;
    playerMapObject->setTile(playerTileset->tileAt(7));

    //position
    if(!current_map->logicalMap.rescue_points.empty())
    {
        xPerso=current_map->logicalMap.rescue_points.first().x;
        yPerso=current_map->logicalMap.rescue_points.first().y;
    }
    else if(!current_map->logicalMap.bot_spawn_points.empty())
    {
        xPerso=current_map->logicalMap.bot_spawn_points.first().x;
        yPerso=current_map->logicalMap.bot_spawn_points.first().y;
    }
    else
    {
        xPerso=current_map->logicalMap.width/2;
        yPerso=current_map->logicalMap.height/2;
    }

    loadCurrentMap();
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

    qDebug() << startTime.elapsed();
    mScene->addItem(mapItem);
    //mScene->setSceneRect(QRectF(xPerso*TILE_SIZE,yPerso*TILE_SIZE,64,32));

    viewport()->update();
}

bool MapVisualiserQt::RectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
        return false;
    if((r2.x()+r2.width())<r1.x())
        return false;

    if((r1.y()+r1.height())<r2.y())
        return false;
    if((r2.y()+r2.height())<r1.y())
        return false;

    return true;
}

void MapVisualiserQt::displayTheDebugMap()
{
    qDebug() << QString("xPerso: %1, yPerso: %2, map: %3").arg(xPerso).arg(yPerso).arg(current_map->logicalMap.map_file);
    int y=0;
    while(y<current_map->logicalMap.height)
    {
        QString line;
        int x=0;
        while(x<current_map->logicalMap.width)
        {
            if(x==xPerso && y==yPerso)
                line+="P";
            else if(current_map->logicalMap.parsed_layer.walkable[x+y*current_map->logicalMap.width])
                line+="_";
            else
                line+="X";
            x++;
        }
        qDebug() << line;
        y++;
    }
}
