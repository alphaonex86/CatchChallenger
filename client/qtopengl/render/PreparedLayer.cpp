#include "PreparedLayer.hpp"
#include "../background/CCMap.hpp"
#include <QDebug>
#include <QWidget>
#include <qmath.h>

PreparedLayer::PreparedLayer(Map_full * tempMapObject,QGraphicsItem *parent) :
    QGraphicsPixmapItem(parent),
    tempMapObject(tempMapObject)
{
    setAcceptHoverEvents(true);
}

PreparedLayer::PreparedLayer(Map_full * tempMapObject,const QPixmap &pixmap, QGraphicsItem *parent) :
    QGraphicsPixmapItem(pixmap,parent),
    tempMapObject(tempMapObject)
{
}

void PreparedLayer::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover move event on map at " << QString::fromStdString(tempMapObject->name) << QString::fromStdString(tempMapObject->logicalMap.map_file) << x << y;
}

void PreparedLayer::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover enter event on map at " << QString::fromStdString(tempMapObject->name) << QString::fromStdString(tempMapObject->logicalMap.map_file) << x << y;
}

void PreparedLayer::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    qDebug() << "Mouse hover leave event on map at " << QString::fromStdString(tempMapObject->name) << QString::fromStdString(tempMapObject->logicalMap.map_file) << x << y;
}

void PreparedLayer::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    qDebug() << "Mouse double click event on map at " << QString::fromStdString(tempMapObject->name) << QString::fromStdString(tempMapObject->logicalMap.map_file) << x << y;
    eventOnMap(CatchChallenger::MapEvent_DoubleClick,tempMapObject,x,y);
}

void PreparedLayer::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    Q_UNUSED(event);
    //const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    //const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    //qDebug() << "Mouse press event on map at " << tempMapObject->name << tempMapObject->logicalMap.map_file << x << y;
    clickDuration.restart();
}

void PreparedLayer::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    event->accept();
    const uint8_t &x=static_cast<uint8_t>(qCeil(event->pos().x()/tempMapObject->tiledMap->tileWidth())-1);
    const uint8_t &y=static_cast<uint8_t>(qCeil(event->pos().y()/tempMapObject->tiledMap->tileHeight())-1);
    //qDebug() << "Mouse release event on map at " << tempMapObject->name << tempMapObject->logicalMap.map_file << x << y;
    if(clickDuration.elapsed()<500)
        eventOnMap(CatchChallenger::MapEvent_SimpleClick,tempMapObject,x,y);
}

void PreparedLayer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //qDebug() << "widget size " << widget->width() << widget->height();
    const float &z=MapVisualiserPlayer::zoom();
    setScale(z);
    QGraphicsPixmapItem::paint(painter,option,widget);
}
