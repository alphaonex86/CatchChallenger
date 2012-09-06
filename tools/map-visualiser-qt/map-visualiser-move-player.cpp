#include "map-visualiser-qt.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/MoveOnTheMap.h"

using namespace Tiled;

//call after enter on new map
void MapVisualiserQt::loadPlayerFromCurrentMap()
{
    qDebug() << QString("loadCurrentMap(), add player on: %1").arg(current_map->logicalMap.map_file);
    current_map->objectGroup->addObject(playerMapObject);
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserQt::unloadPlayerFromCurrentMap()
{
    //load the player sprite
    int index=current_map->objectGroup->removeObject(playerMapObject);
    /*delete playerMapObject;
    playerMapObject = new MapObject();
    playerMapObject->setTile(playerTileset->tileAt(7));*/
    qDebug() << QString("unloadCurrentMap(): after remove the player: %1").arg(index);
}
