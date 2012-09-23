#include "objectitem.h"

ObjectItem::ObjectItem(MapObject *mapObject, QGraphicsItem *parent) :
        QGraphicsItem(parent),mMapObject(mapObject)
{
}

ObjectItem::~ObjectItem()
{
        //qWarning() << "Item :" << this;
        qDeleteAll(childItems());
        if(mMapObject)
               mMapObject = 0;
}

QRectF ObjectItem::boundingRect() const
{
        MapRenderer* mRenderer = gameData::instance()->getMapRenderer();
        if(mMapObject!=0 && mRenderer!=0)
        {
                gameData::destroyInstanceAtTheLastCall();
                return mRenderer->boundingRect(mMapObject);
        }
        gameData::destroyInstanceAtTheLastCall();
        qWarning() << "Object bug";
        return QRectF(0,0,0,0);
}

void ObjectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *){
    MapRenderer* mRenderer = gameData::instance()->getMapRenderer();
    if(mRenderer!=0)
        mRenderer->drawMapObject(painter,mMapObject,QColor(255,255,255,0));
    gameData::destroyInstanceAtTheLastCall();
}
