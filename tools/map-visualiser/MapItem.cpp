#include "MapItem.h"
#include "Variables.h"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QDebug>

MapItem::MapItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);

    cache=true;
}

void MapItem::addMap(Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex)
{
    //align zIndex to "Dyna management" Layer
    int index=-playerLayerIndex;
    QList<Tiled::Layer *> layers=map->layers();

    QImage image;
    QGraphicsItem * graphicsItem=NULL;
    // Create a child item for each layer
    foreach (Tiled::Layer *layer, layers) {
        if (Tiled::TileLayer *tileLayer = layer->asTileLayer()) {
            graphicsItem=new TileLayerItem(tileLayer, renderer, this);
            if(cache && image.size().isNull())
            {
                image=QImage(QSize(graphicsItem->boundingRect().size().width(),graphicsItem->boundingRect().size().height()),QImage::Format_ARGB32);
                image.fill(Qt::transparent);
            }

            if(!cache || image.size()!=graphicsItem->boundingRect().size())
            {}
            else
            {
                QPainter painter(&image);
                renderer->drawTileLayer(&painter, tileLayer, graphicsItem->boundingRect());
                delete graphicsItem;
                graphicsItem=NULL;
            }
        } else if (Tiled::ObjectGroup *objectGroup = layer->asObjectGroup()) {
            if(cache && !image.size().isNull())
            {
                QPixmap tempPixmap;
                if(tempPixmap.convertFromImage(image))
                {
                    graphicsItem=new QGraphicsPixmapItem(tempPixmap,this);
                    graphicsItem->setZValue(index++);
                    displayed_layer.insert(map,graphicsItem);
                    image=QImage();
                }
            }

            ObjectGroupItem *newObject=new ObjectGroupItem(objectGroup, this);
            graphicsItem=newObject;

            ObjectGroupItem::objectGroupLink[objectGroup]=newObject;
            MapObjectItem::mRendererList[objectGroup]=renderer;
        }
        if(graphicsItem!=NULL)
        {
            graphicsItem->setZValue(index++);
            displayed_layer.insert(map,graphicsItem);
        }
    }
    if(cache && !image.size().isNull())
    {
        QPixmap tempPixmap;
        if(tempPixmap.convertFromImage(image))
        {
            graphicsItem=new QGraphicsPixmapItem(tempPixmap,this);
            graphicsItem->setZValue(index++);
            displayed_layer.insert(map,graphicsItem);
        }
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
