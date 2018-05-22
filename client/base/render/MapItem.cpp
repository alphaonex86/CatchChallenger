#include "MapItem.h"
#include "PreparedLayer.h"
#include "../ClientVariable.h"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QLabel>

MapItem::MapItem(QGraphicsItem *parent,const bool &useCache)
    : QGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug

    cache=useCache;
}

void MapItem::addMap(MapVisualiserThread::Map_full * tempMapObject,Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex)
{
    if(displayed_layer.contains(map))
    {
        qDebug() << "Map already displayed";
        return;
    }
    //align zIndex to "Dyna management" Layer
    int index=-playerLayerIndex;
    const QList<Tiled::Layer *> &layers=map->layers();

    QImage image;
    QGraphicsItem * graphicsItem=NULL;
    QStringList mapNameList;
    // Create a child item for each layer
    const int &loopSize=layers.size();
    int index2=0;
    while(index2<loopSize)
    {
        mapNameList << layers.at(index2)->name();
        if (Tiled::TileLayer *tileLayer = layers.at(index2)->asTileLayer()) {
            graphicsItem=new TileLayerItem(tileLayer, renderer, this);
            if(cache && image.size().isNull())
            {
                image=QImage(QSize(graphicsItem->boundingRect().size().width(),graphicsItem->boundingRect().size().height()),QImage::Format_ARGB32_Premultiplied);
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
        } else if(Tiled::ObjectGroup *objectGroup = layers.at(index2)->asObjectGroup()) {
            if(cache && !image.size().isNull())
            {
                QPixmap tempPixmap;
                if(tempPixmap.convertFromImage(image))
                {
                    #ifdef DEBUG_RENDER_DISPLAY_INDIVIDUAL_LAYER
                    QLabel *temp=new QLabel();
                    temp->setPixmap(tempPixmap);
                    temp->show();
                    #endif
                    graphicsItem=new PreparedLayer(tempMapObject,tempPixmap,this);
                    if(!connect(static_cast<PreparedLayer *>(graphicsItem),&PreparedLayer::eventOnMap,this,&MapItem::eventOnMap))
                        abort();
                    graphicsItem->setZValue(index-1);
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
            graphicsItem->setZValue(index);
            displayed_layer.insert(map,graphicsItem);
        }
        index++;
        index2++;
    }
    if(cache && !image.size().isNull())
    {
        QPixmap tempPixmap;
        if(tempPixmap.convertFromImage(image))
        {
            #ifdef DEBUG_RENDER_DISPLAY_INDIVIDUAL_LAYER
            QLabel *temp=new QLabel();
            temp->setPixmap(tempPixmap);
            temp->show();
            #endif
            graphicsItem=new PreparedLayer(tempMapObject,tempPixmap,this);
            if(!connect(static_cast<PreparedLayer *>(graphicsItem),&PreparedLayer::eventOnMap,this,&MapItem::eventOnMap))
                abort();
            graphicsItem->setZValue(index);
            displayed_layer.insert(map,graphicsItem);
        }
    }

    #ifdef DEBUG_RENDER_CACHE
    if(cache)
        qDebug() << "Map: " << layers.size() << " layers (" << mapNameList.join(";") << "), but only " << displayed_layer.count(map) << " displayed";
    #endif
}

bool MapItem::haveMap(Tiled::Map *map)
{
    return displayed_layer.contains(map);
}

void MapItem::removeMap(Tiled::Map *map)
{
    const QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        delete values.at(index);
        index++;
    }
    displayed_layer.remove(map);
}

void MapItem::setMapPosition(Tiled::Map *map, int16_t x/*pixel, need be 16Bits*/, int16_t y/*pixel, need be 16Bits*/)
{
    const QList<QGraphicsItem *> values = displayed_layer.values(map);
    int index=0;
    while(index<values.size())
    {
        values.at(index)->setPos(static_cast<qreal>(static_cast<double>(x)),static_cast<qreal>(static_cast<double>(y)));
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
