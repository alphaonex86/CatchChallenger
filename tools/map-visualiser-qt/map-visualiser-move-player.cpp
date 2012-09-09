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
    Tiled::ObjectGroup *currentGroup=playerMapObject->objectGroup();
    current_map->objectGroup->addObject(playerMapObject);

    if(currentGroup!=NULL)
    {
        if(currentGroup!=current_map->objectGroup)
            qDebug() << QString("loadPlayerFromCurrentMap(), the playerMapObject group is wrong").arg(currentGroup->name());
    }

    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    playerMapObject->setPosition(QPoint(xPerso,yPerso+1));
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapVisualiserQt::unloadPlayerFromCurrentMap()
{
    //load the player sprite
    current_map->objectGroup->removeObject(playerMapObject);
}
