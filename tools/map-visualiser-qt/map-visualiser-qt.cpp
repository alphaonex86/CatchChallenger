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
        return mRendererList[mMapObject->objectGroup()]->boundingRect(mMapObject);
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
class ObjectGroupItem : public QGraphicsItem
{
public:
    ObjectGroupItem(ObjectGroup *objectGroup,
                    QGraphicsItem *parent = 0)
        : QGraphicsItem(parent),
          mObjectGroup(objectGroup)
    {
//        setFlag(QGraphicsItem::ItemHasNoContents);

    }

    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
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
public:
    ObjectGroup *mObjectGroup;
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
            tempItem.graphic=new ObjectGroupItem(objectGroup, this);
            tempItem.layer=objectGroup;
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
        values.at(index).graphic->setPos(x*16,y*16);
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
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                         | QGraphicsView::DontSavePainterState);
    setBackgroundBrush(Qt::black);
    setFrameStyle(QFrame::NoFrame);

    //viewport()->setAttribute(Qt::WA_StaticContents);
    //setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
}

MapVisualiserQt::~MapVisualiserQt()
{
    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = other_map.constBegin();
    while (i != other_map.constEnd()) {
        //if it's the last reference
        if(!displayed_map.contains(*i))
        {
            delete (*i)->logicalMap.parsed_layer.walkable;
            delete (*i)->logicalMap.parsed_layer.water;
            qDeleteAll((*i)->tiledMap->tilesets());
            delete (*i)->tiledMap;
            delete (*i)->tiledRender;
            delete (*i);
        }
        other_map.remove((*i)->logicalMap.map_file);
        i = other_map.constBegin();//needed
    }
    delete mapItem;
    delete playerTileset;
    delete playerMapObject;
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
    centerOn(0, 0);

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
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));

    loadCurrentMap();

    qDebug() << startTime.elapsed();
    mScene->addItem(mapItem);
}

bool MapVisualiserQt::RectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
    {
        qDebug() << QString("MapVisualiserQt::RectTouch(): condition 1 not respected");
        return false;
    }
    if((r2.x()+r2.width())<r1.x())
    {
        qDebug() << QString("MapVisualiserQt::RectTouch(): condition 2 not respected");
        return false;
    }

    if((r1.y()+r1.height())<r2.y())
    {
        qDebug() << QString("MapVisualiserQt::RectTouch(): condition 3 not respected");
        return false;
    }
    if((r2.y()+r2.height())<r1.y())
    {
        qDebug() << QString("MapVisualiserQt::RectTouch(): condition 4 not respected: %1+%2<%3").arg(r2.y()).arg(r2.width()).arg(r1.y());
        return false;
    }

    return true;
}
