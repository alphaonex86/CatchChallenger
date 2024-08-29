#include "MapItem.hpp"
#include "PreparedLayer.hpp"
#include "../libcatchchallenger/ClientVariable.hpp"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QLabel>
#include <iostream>

MapItem::MapItem(QGraphicsItem *parent,const bool &useCache)
    : QGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug

    cache=useCache;
}

void MapItem::addMap(Map_full * tempMapObject,Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex)
{
    if(displayed_layer.find(map)!=displayed_layer.cend())
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
    int width=0;
    int height=0;
    for(int i = 0; i < loopSize; ++i)
    {
        if (Tiled::TileLayer *tileLayer = layers.at(i)->asTileLayer()) {
            TileLayerItem tileLayerItem=TileLayerItem(tileLayer, renderer, this);
            QSize size(tileLayerItem.boundingRect().size().width(),tileLayerItem.boundingRect().size().height());
            if(width < size.width()){
                width = size.width();
            }
            if(height < size.height()){
                height = size.height();
            }
        }

    }
    int index2=0;
    while(index2<loopSize)
    {
        mapNameList << layers.at(index2)->name();
        if (Tiled::TileLayer *tileLayer = layers.at(index2)->asTileLayer()) {
            graphicsItem=new TileLayerItem(tileLayer, renderer, this);
            if(cache && image.size().isNull())
            {
                image=QImage(QSize(width,height),QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
            }

            if(!cache)// || image.size()!=graphicsItem->boundingRect().size())
            {
            }
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
                    displayed_layer[map].insert(graphicsItem);
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
            displayed_layer[map].insert(graphicsItem);
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
            displayed_layer[map].insert(graphicsItem);
        }
    }

    #ifdef DEBUG_RENDER_CACHE
    if(cache)
        qDebug() << "Map: " << layers.size() << " layers (" << mapNameList.join(";") << "), but only " << displayed_layer.count(map) << " displayed";
    #endif
}

bool MapItem::haveMap(Tiled::Map *map)
{
    return displayed_layer.find(map)!=displayed_layer.cend();
}

void MapItem::removeMap(Tiled::Map *map)
{
    std::cerr << "map: " << map << std::endl;
    if(displayed_layer.find(map)==displayed_layer.cend())
            return;
    const std::unordered_set<QGraphicsItem *> &values = displayed_layer.at(map);
    for( const auto& value : values ) {
        std::cerr << "values.at(index): " << value << std::endl;
        delete value;
    }
    displayed_layer.erase(map);
}

void MapItem::setMapPosition(Tiled::Map *map, int16_t x/*pixel, need be 16Bits*/, int16_t y/*pixel, need be 16Bits*/)
{
    if(displayed_layer.find(map)==displayed_layer.cend())
            return;
    const std::unordered_set<QGraphicsItem *> &values = displayed_layer.at(map);
    for( const auto& value : values ) {
        value->setPos(static_cast<qreal>(static_cast<double>(x)),static_cast<qreal>(static_cast<double>(y)));
    }
}

QRectF MapItem::boundingRect() const
{
    return QRectF();
}

void MapItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}
