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
    qDebug() << QString("loadPlayerFromCurrentMap(), add player on: %1, layer: %2, count: %3")
                .arg(current_map->logicalMap.map_file)
                .arg((quint64)current_map->objectGroup)
                .arg(current_map->objectGroup->objectCount())
                        ;
    Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
    current_map->objectGroup->addObject(playerMapObject);

    if(currentGroup!=NULL)
    {
        qDebug() << QString("loadPlayerFromCurrentMap(), remove player from layer: %1, count: %2")
                    .arg((quint64)currentGroup)
                    .arg(currentGroup->objectCount());

        if(currentGroup!=current_map->objectGroup)
            qDebug() << QString("loadPlayerFromCurrentMap(), the playerMapObject group is wrong").arg(currentGroup->name());
    }

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));
    qDebug() << QString("loadPlayerFromCurrentMap(), add player at: %1,%2 or: %3,%4").arg(xPerso).arg(yPerso).arg(playerMapObject->position().x()).arg(playerMapObject->position().y());
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserQt::unloadPlayerFromCurrentMap()
{
    qDebug() << QString("unloadPlayerFromCurrentMap(), remove player on: %1, layer: %2, count: %3")
                .arg(current_map->logicalMap.map_file)
                .arg((quint64)current_map->objectGroup)
                .arg(current_map->objectGroup->objectCount())
                        ;
    //load the player sprite
    int index=current_map->objectGroup->removeObject(playerMapObject);
    qDebug() << QString("unloadPlayerFromCurrentMap(): after remove the player: %1 on map: %2").arg(index).arg(current_map->logicalMap.map_file);
    /*delete playerMapObject;
        playerMapObject = new MapObject();
        playerMapObject->setTile(playerTileset->tileAt(7));*/

    current_map->objectGroup->setVisible(true);
}
