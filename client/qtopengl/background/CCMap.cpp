#include "CCMap.hpp"
#include "../../libqtcatchchallenger/maprender/QMap_client.hpp"
#include "../ConnexionManager.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libcatchchallenger/DatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/MoveOnTheMap.hpp"
#include <QPainter>
#include <QElapsedTimer>
#include <chrono>
#include <iostream>
#include <math.h>

CCMap::CCMap()
{
    if(!connect(&mapController,&MapController::pathFindingNotFound,this,&CCMap::pathFindingNotFound))
        abort();
    if(!connect(&mapController,&MapController::repelEffectIsOver,this,&CCMap::repelEffectIsOver))
        abort();
    if(!connect(&mapController,&MapController::teleportConditionNotRespected,this,&CCMap::teleportConditionNotRespected))
        abort();
    if(!connect(&mapController,&MapController::stopped_in_front_of,this,&CCMap::onMapControllerStoppedInFrontOf))
        abort();
    if(!connect(&mapController,&MapController::actionOn,this,&CCMap::onMapControllerActionOn))
        abort();
    if(!connect(&mapController,&MapController::actionOnNothing,this,&CCMap::actionOnNothing))
        abort();
    if(!connect(&mapController,&MapController::blockedOn,this,&CCMap::blockedOn))
        abort();
    if(!connect(&mapController,&MapController::wildFightCollision,this,&CCMap::onMapControllerWildFightCollision))
        abort();
    if(!connect(&mapController,&MapController::botFightCollision,this,&CCMap::onMapControllerBotFightCollision))
        abort();
    if(!connect(&mapController,&MapController::error,this,&CCMap::error))
        abort();
    if(!connect(&mapController,&MapController::errorWithTheCurrentMap,this,&CCMap::errorWithTheCurrentMap))
        abort();
    if(!connect(&mapController,&MapController::currentMapLoaded,this,&CCMap::currentMapLoaded))
        abort();
    clicked=false;
    scale=1.0;
    x=0.0;
    y=0.0;
}

void CCMap::setVar(ConnexionManager *connexionManager)
{
    mapController.resetAll();
    mapController.connectAllSignals(connexionManager->client);
    /* need be call after select the char:
    mapController.setDatapackPath(connexionManager->client->datapackPathBase(),connexionManager->client->mainDatapackCode());
    mapController.datapackParsed();
    mapController.datapackParsedMainSub();*/
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
        /*MapObjectItem::x=x;
        MapObjectItem::y=y;*/
        painter->translate(x,y);
        child->paint(painter,option,widget);
        painter->translate(-x,-y);
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
    zoomFinal*=CatchChallenger::CommonDatapack::commonDatapack.get_layersOptions().zoom;
    scale=ceil(zoomFinal);
    /*MapObjectItem::scale=scale;
    MapObjectItem::playerObject=mapController.getPlayerMapObject();*/
    painter->scale(scale,scale);

    const Tiled::MapObject * p=mapController.getPlayerMapObject();
    x=(widget->width()/2/scale-(p->x()*16+p->width()/2));
    y=(widget->height()/2/scale-((p->y()-1)*16+p->height()/2));
    paintChildItems(mapController.mapItem->childItems(),x,y,painter,option,widget);
}

void CCMap::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &/*callParentClass*/)
{
    if(clicked)
        return;
    clicked=true;
    lastClickedPos=p;
    pressValidated=true;
}

void CCMap::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &/*callParentClass*/)
{
    if(!clicked)
        return;
    clicked=false;
    pressValidated=true;
    //mapController.keyPressReset();

    //convert pixel scaled -> pixel -> tile
    qreal diffX=(p.x()/scale-x)/16;
    qreal diffY=(p.y()/scale-y)/16;
    //std::cout << "CCMap clicked: " << p.x() << "," << p.y() << " - " << diffX << "," << diffY << " - " << x << "," << y << " * " << scale << std::endl;

    std::unordered_set<CATCHCHALLENGER_TYPE_MAPID> mapToScan;
    CATCHCHALLENGER_TYPE_MAPID currentMapId=mapController.currentMap();
    const CatchChallenger::CommonMap &currentLogicalMap=QtDatapackClientLoader::datapackLoader->getMap(currentMapId);
    mapToScan.insert(currentMapId);
    if(currentLogicalMap.border.left.mapIndex!=65535)
        mapToScan.insert(currentLogicalMap.border.left.mapIndex);
    if(currentLogicalMap.border.right.mapIndex!=65535)
        mapToScan.insert(currentLogicalMap.border.right.mapIndex);
    if(currentLogicalMap.border.top.mapIndex!=65535)
        mapToScan.insert(currentLogicalMap.border.top.mapIndex);
    if(currentLogicalMap.border.bottom.mapIndex!=65535)
        mapToScan.insert(currentLogicalMap.border.bottom.mapIndex);

    //locate the right map
    for( const auto& n : CatchChallenger::QMap_client::all_map ) {
        CATCHCHALLENGER_TYPE_MAPID mapId=n.first;
        CatchChallenger::QMap_client *map=n.second;
        if(mapToScan.find(mapId)!=mapToScan.cend())
        {
            const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(mapId);
            if(diffX>=map->relative_x && diffX<=(map->relative_x+logicalMap.width))
                if(diffY>=map->relative_y && diffY<=(map->relative_y+logicalMap.height))
                {
                    std::cout << "click on map " << mapId << " " << diffX << "," << diffY << std::endl;
                    mapController.eventOnMap(CatchChallenger::MapEvent_SimpleClick,mapId,(int)floor(diffX)-map->relative_x,(int)floor(diffY)-map->relative_y);
                    return;
                }
        }
    }
    std::cerr << "CCMap not found: " << p.x() << "," << p.y() << std::endl;
}

void CCMap::keyPressReset()
{
    //mapController.keyPressReset();
}

void CCMap::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    mapController.keyPressEvent(event);
    eventTriggerGeneral=false;
}

void CCMap::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    mapController.keyReleaseEvent(event);
    eventTriggerGeneral=false;
}

void CCMap::onMapControllerStoppedInFrontOf(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    emit stopped_in_front_of(map->id,x,y);
}

void CCMap::onMapControllerActionOn(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    emit actionOn(map->id,x,y);
}

void CCMap::onMapControllerWildFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    emit wildFightCollision(map->id,x,y);
}

void CCMap::onMapControllerBotFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    emit botFightCollision(map->id,x,y);
}

