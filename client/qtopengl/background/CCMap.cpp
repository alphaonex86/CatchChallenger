#include "CCMap.hpp"
#include "../ConnexionManager.hpp"
#include "../../qt/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <QTime>
#include <chrono>
#include <iostream>
#include <math.h>

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
    clicked=false;
    scale=1.0;
    x=0.0;
    y=0.0;
}

void CCMap::setVar(ConnexionManager *connexionManager)
{
    mapController.fightEngine=connexionManager->client->fightEngine;
    mapController.resetAll();
    mapController.connectAllSignals(connexionManager->client);
    mapController.datapackParsed();
    mapController.datapackParsedMainSub();
    clicked=false;
    scale=1.0;
    x=0.0;
    y=0.0;
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
    qreal zoomW=(qreal)widget->width()/(32.0*16.0);
    qreal zoomH=(qreal)widget->height()/(32.0*16.0);
    qreal zoomFinal=zoomW;
    //keep the greatest value
    if(zoomFinal>zoomH)
        zoomFinal=zoomH;
    zoomFinal*=CatchChallenger::CommonDatapack::commonDatapack.layersOptions.zoom;
    scale=ceil(zoomFinal);
    painter->scale(scale,scale);

    const Tiled::MapObject * p=mapController.getPlayerMapObject();
    x=(widget->width()/2/scale-(p->x()*16+p->width()/2));
    y=(widget->height()/2/scale-((p->y()-1)*16+p->height()/2));
    paintChildItems(mapController.mapItem->childItems(),x,y,painter,option,widget);
}

void CCMap::mousePressEventXY(const QPointF &p, bool &pressValidated)
{
    if(clicked)
        return;
    clicked=true;
    lastClickedPos=p;
    pressValidated=true;
}

void CCMap::mouseReleaseEventXY(const QPointF &p,bool &pressValidated)
{
    if(!clicked)
        return;
    clicked=false;
    pressValidated=true;

    //convert pixel scaled -> pixel -> tile
    qreal diffX=(p.x()/scale-x)/16;
    qreal diffY=(p.y()/scale-y)/16;
    //std::cout << "CCMap clicked: " << p.x() << "," << p.y() << " - " << diffX << "," << diffY << " - " << x << "," << y << " * " << scale << std::endl;

    std::unordered_set<std::string> mapToScan;
    Map_full * current_map=mapController.currentMapFull();
    mapToScan.insert(current_map->logicalMap.map_file);
    if(current_map->logicalMap.border.left.map!=nullptr)
        mapToScan.insert(current_map->logicalMap.border.left.map->map_file);
    if(current_map->logicalMap.border.right.map!=nullptr)
        mapToScan.insert(current_map->logicalMap.border.right.map->map_file);
    if(current_map->logicalMap.border.top.map!=nullptr)
        mapToScan.insert(current_map->logicalMap.border.top.map->map_file);
    if(current_map->logicalMap.border.bottom.map!=nullptr)
        mapToScan.insert(current_map->logicalMap.border.bottom.map->map_file);

    std::unordered_map<std::string,Map_full *> all_map=mapController.all_map;
    //locate the right map
    for( const auto& n : all_map ) {
        Map_full *map=n.second;
        if(mapToScan.find(map->logicalMap.map_file)!=mapToScan.cend())
        {
            /*std::cout << map->logicalMap.map_file << ": "
                      << std::to_string(map->relative_x) << "," << std::to_string(map->relative_y) << ","
                      << std::to_string(map->logicalMap.width) << "," << std::to_string(map->logicalMap.height)
                      << std::endl;*/
            if(diffX>=map->relative_x && diffX<=(map->relative_x+map->logicalMap.width))
                if(diffY>=map->relative_y && diffY<=(map->relative_y+map->logicalMap.height))
                {
                    std::cout << "click on " << map->logicalMap.map_file << " " << diffX << "," << diffY << std::endl;
                    mapController.eventOnMap(CatchChallenger::MapEvent_SimpleClick,map,diffX,diffY);
                    return;
                }
        }
    }
    std::cerr << "CCMap not found: " << p.x() << "," << p.y() << std::endl;
}
