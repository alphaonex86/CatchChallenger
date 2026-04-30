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
#include <tileset.h>

std::unordered_set<Tiled::Tileset *> MapItem::validTilesets_;

MapItem::MapItem(QGraphicsItem *parent,const bool &useCache)
    : QGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemHasNoContents);
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);just change direction without move do bug

    cache=useCache;
}

void MapItem::addMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,Tiled::Map *map, Tiled::MapRenderer *renderer,const int &playerLayerIndex)
{
    std::cerr << "MapItem::addMap() mapIndex=" << mapIndex << " map=" << map << " renderer=" << renderer << " playerLayerIndex=" << playerLayerIndex << " cache=" << cache << std::endl;
    if(map==NULL)
    {
        std::cerr << "MapItem::addMap() map is NULL (abort add)" << std::endl;
        return;
    }
    {
        std::unordered_set<Tiled::Tileset *> &perMap=tilesetsPerMap_[map];
        const QVector<Tiled::SharedTileset> &mapTilesets=map->tilesets();
        int tsIndex=0;
        while(tsIndex<mapTilesets.size())
        {
            Tiled::Tileset *ts=mapTilesets.at(tsIndex).data();
            if(ts!=NULL)
            {
                perMap.insert(ts);
                validTilesets_.insert(ts);
            }
            tsIndex++;
        }
    }
    if(renderer==NULL)
    {
        std::cerr << "MapItem::addMap() renderer is NULL (abort add)" << std::endl;
        return;
    }
    if(displayed_layer.find(map)!=displayed_layer.cend())
    {
        qDebug() << "Map already displayed";
        return;
    }
    //align zIndex to "Dyna management" Layer
    int index=-playerLayerIndex;
    const QList<Tiled::Layer *> &layers=map->layers();
    std::cerr << "MapItem::addMap() layers.size()=" << layers.size() << std::endl;

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
                    graphicsItem=new PreparedLayer(mapIndex,tempPixmap,this);
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
            graphicsItem=new PreparedLayer(mapIndex,tempPixmap,this);
            if(!connect(static_cast<PreparedLayer *>(graphicsItem),&PreparedLayer::eventOnMap,this,&MapItem::eventOnMap))
                abort();
            graphicsItem->setZValue(index);
            displayed_layer[map].insert(graphicsItem);
        }
        else
            std::cerr << "MapItem::addMap() final tempPixmap.convertFromImage failed" << std::endl;
    }
    std::cerr << "MapItem::addMap() done mapIndex=" << mapIndex << " " << layers.size() << " layers, " << displayed_layer[map].size() << " items added (width=" << width << " height=" << height << ")" << std::endl;
    if(displayed_layer[map].empty())
        std::cerr << "MapItem::addMap() WARNING: NO items added for mapIndex=" << mapIndex << " (black map will result)" << std::endl;

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
    for( QGraphicsItem * const& value : values ) {
        std::cerr << "values.at(index): " << value << std::endl;
        delete value;
    }
    displayed_layer.erase(map);
    if(tilesetsPerMap_.find(map)!=tilesetsPerMap_.cend())
    {
        const std::unordered_set<Tiled::Tileset *> &perMap=tilesetsPerMap_.at(map);
        for( Tiled::Tileset * const& ts : perMap )
        {
            //only erase from the global set when no other still-loaded map keeps it.
            bool stillUsed=false;
            for( const std::pair<Tiled::Map * const,std::unordered_set<Tiled::Tileset *> > &n : tilesetsPerMap_ )
            {
                if(n.first!=map && n.second.find(ts)!=n.second.cend())
                {
                    stillUsed=true;
                    break;
                }
            }
            if(!stillUsed)
                validTilesets_.erase(ts);
        }
        tilesetsPerMap_.erase(map);
    }
}

void MapItem::setMapPosition(Tiled::Map *map, int16_t global_x/*pixel, need be 16Bits*/, int16_t global_y/*pixel, need be 16Bits*/)
{
    std::cerr << "MapItem::setMapPosition() map=" << map << " global_x=" << global_x << " global_y=" << global_y << std::endl;
    if(displayed_layer.find(map)==displayed_layer.cend())
    {
        std::cerr << "MapItem::setMapPosition() map not in displayed_layer (skip)" << std::endl;
        return;
    }
    const std::unordered_set<QGraphicsItem *> &values = displayed_layer.at(map);
    for( QGraphicsItem * const& value : values ) {
        value->setPos(static_cast<qreal>(static_cast<double>(global_x)),static_cast<qreal>(static_cast<double>(global_y)));
    }
}

QRectF MapItem::boundingRect() const
{
    return QRectF();
}

void MapItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{
}
