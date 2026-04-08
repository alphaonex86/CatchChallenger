#include "PreparedLayer.hpp"
#include <QDebug>
#include <qmath.h>

PreparedLayer::PreparedLayer(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QGraphicsItem *parent) :
    QGraphicsPixmapItem(parent),
    mapIndex(mapIndex)
{
    setAcceptHoverEvents(true);
}

PreparedLayer::PreparedLayer(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const QPixmap &pixmap, QGraphicsItem *parent) :
    QGraphicsPixmapItem(pixmap,parent),
    mapIndex(mapIndex)
{
}

void PreparedLayer::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover move event on map at " << mapIndex << x << y;
}

void PreparedLayer::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover enter event on map at " << mapIndex << x << y;
}

void PreparedLayer::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover leave event on map at " << mapIndex << x << y;
}

void PreparedLayer::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    qDebug() << "Mouse double click event on map at " << mapIndex << x << y;
    eventOnMap(CatchChallenger::MapEvent_DoubleClick,mapIndex,x,y);
}

void PreparedLayer::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    Q_UNUSED(event);
    //const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    //const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    //qDebug() << "Mouse press event on map at " << mapIndex << x << y;
    clickDuration.restart();
}

void PreparedLayer::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/CatchChallenger::QMap_client::all_map.at(mapIndex)->tiledMap->tileHeight())-1);
    //qDebug() << "Mouse release event on map at " << mapIndex << x << y;
    if(clickDuration.elapsed()<500)
        eventOnMap(CatchChallenger::MapEvent_SimpleClick,mapIndex,x,y);
}
