#include "CCMap.hpp"
#include "../ConnexionManager.hpp"
#include "../../qt/QtDatapackClientLoader.hpp"
#include <QPainter>
#include <QTime>
#include <chrono>
#include <iostream>

CCMap::CCMap()
{
    if(!connect(&mapController,&MapController::pathFindingNotFound,this,&CCMap::pathFindingNotFound))
        abort();
    if(!connect(&mapController,&MapController::searchPath,this,&CCMap::searchPath))
        abort();
    if(!connect(&mapController,&MapController::repelEffectIsOver,this,&CCMap::repelEffectIsOver))
        abort();
    if(!connect(&mapController,&MapController::teleportConditionNotRespected,this,&CCMap::teleportConditionNotRespected))
        abort();
    if(!connect(&mapController,&MapController::send_player_direction,this,&CCMap::send_player_direction))
        abort();
    if(!connect(&mapController,&MapController::stopped_in_front_of,this,&CCMap::stopped_in_front_of))
        abort();
    if(!connect(&mapController,&MapController::actionOn,this,&CCMap::actionOn))
        abort();
    if(!connect(&mapController,&MapController::actionOnNothing,this,&CCMap::actionOnNothing))
        abort();
    if(!connect(&mapController,&MapController::blockedOn,this,&CCMap::blockedOn))
        abort();
    if(!connect(&mapController,&MapController::wildFightCollision,this,&CCMap::wildFightCollision))
        abort();
    if(!connect(&mapController,&MapController::botFightCollision,this,&CCMap::botFightCollision))
        abort();
    if(!connect(&mapController,&MapController::error,this,&CCMap::error))
        abort();
    if(!connect(&mapController,&MapController::errorWithTheCurrentMap,this,&CCMap::errorWithTheCurrentMap))
        abort();
    if(!connect(&mapController,&MapController::inWaitingOfMap,this,&CCMap::inWaitingOfMap))
        abort();
    if(!connect(&mapController,&MapController::currentMapLoaded,this,&CCMap::currentMapLoaded))
        abort();
    if(!connect(&mapController,&MapController::loadOtherMapAsync,this,&CCMap::loadOtherMapAsync))
        abort();
    if(!connect(&mapController,&MapController::mapDisplayed,this,&CCMap::mapDisplayed))
        abort();
}

void CCMap::setVar(ConnexionManager *connexionManager)
{
    mapController.fightEngine=connexionManager->client->fightEngine;
    mapController.resetAll();
    mapController.connectAllSignals(connexionManager->client);
    mapController.datapackParsed();
    mapController.datapackParsedMainSub();
}

QRectF CCMap::boundingRect() const
{
    return QRectF();
}

void CCMap::paintChildItems(QList<QGraphicsItem *> childItems,qreal parentX,qreal parentY,QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    int index=0;
    while(index<childItems.size())
    {
        //take care of childs.at(index)->zValue()
        //take care of x,y
        QGraphicsItem * child=childItems.at(index);
        qreal x=child->x()+parentX;
        qreal y=child->y()+parentY;
        painter->translate(x,y);
        //painter->translate(child->x(),child->y());
        child->paint(painter,option,widget);
        painter->translate(-x,-y);
        //painter->translate(-child->x(),-child->y());
        //return;
        paintChildItems(child->childItems(),x,y,painter,option,widget);
        index++;
    }
}

void CCMap::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    float scale=5.0;
    painter->scale(scale,scale);//work
    /* unordened paint, failed
     * for( const auto& n : mapController.mapItem->displayed_layer ) {
        Tiled::Map * key=n.first;
        std::unordered_set<QGraphicsItem *> values=n.second;
        for( const auto& value : values ) {
            value->paint(painter,option,widget);
        }
        return;
    }*/
    //mapController.mapItem.paint() not work
    const Tiled::MapObject * p=mapController.getPlayerMapObject();
    qreal x=(widget->width()/2/scale-(p->x()*16+p->width()/2));
    qreal y=(widget->height()/2/scale-((p->y()-1)*16+p->height()/2));
    paintChildItems(mapController.mapItem->childItems(),x,y,painter,option,widget);
}
