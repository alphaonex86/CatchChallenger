#include "MapItem.h"
#include "Variables.h"

MapItem::MapItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
}

void MapItem::addMap(Tiled::Map *map, Tiled::MapRenderer *renderer)
{
    //align zIndex to Collision Layer
    int index=0;
    QList<Tiled::Layer *> layers=map->layers();
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

    QGraphicsItem * graphicsItem=NULL;
    // Create a child item for each layer
    foreach (Tiled::Layer *layer, layers) {
        if (Tiled::TileLayer *tileLayer = layer->asTileLayer()) {
            graphicsItem=new TileLayerItem(tileLayer, renderer, this);
        } else if (Tiled::ObjectGroup *objectGroup = layer->asObjectGroup()) {
            ObjectGroupItem *newObject=new ObjectGroupItem(objectGroup, this);
            graphicsItem=newObject;

            ObjectGroupItem::objectGroupLink[objectGroup]=newObject;
            MapObjectItem::mRendererList[objectGroup]=renderer;
        }
        graphicsItem->setZValue(index++);
        displayed_layer.insert(map,graphicsItem);
    }
}

void MapItem::removeMap(Tiled::Map *map)
{
    QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        delete values.at(index);
        index++;
    }
    displayed_layer.remove(map);
}

void MapItem::setMapPosition(Tiled::Map *map,qint16 x,qint16 y)
{
    QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        values.at(index)->setPos(x*TILE_SIZE,y*TILE_SIZE);
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
