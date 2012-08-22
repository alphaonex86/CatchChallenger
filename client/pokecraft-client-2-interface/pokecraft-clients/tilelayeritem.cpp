#include "tilelayeritem.h"
#include "gamedata.h"

TileLayerItem::TileLayerItem(TileLayer *tileLayer,
              QGraphicsItem *parent): QGraphicsItem(parent)
, mTileLayer(tileLayer)
{
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
        if(mTileLayer == 0)
            qWarning()<<"Layer bugging ....";;
}
TileLayerItem::~TileLayerItem(){
}
QRectF TileLayerItem::boundingRect() const
{
    if(mTileLayer!=0)
    {
            MapRenderer* renderer = gameData::instance()->getMapRenderer();
            gameData::destroyInstanceAtTheLastCall();
            if(renderer!=0)
                return renderer->boundingRect(mTileLayer->bounds());
            else
                return QRect();
    }
    qWarning() << "TileLayer not detected";
    return QRectF();
}

void TileLayerItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *)
{
         gameData::instance()->getMapRenderer()->drawTileLayer(p, mTileLayer, option->rect);
         gameData::destroyInstanceAtTheLastCall();
}
QString TileLayerItem::Name()
{
        return mTileLayer->name();
}
