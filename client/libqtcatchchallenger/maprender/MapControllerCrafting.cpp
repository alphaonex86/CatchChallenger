#include "MapController.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "QMap_client.hpp"

#include <iostream>
#include <QDebug>
#include <QFileInfo>

/* mapIdToString is commented out in the header
std::string MapController::mapIdToString(const CATCHCHALLENGER_TYPE_MAPID &mapId) const
{
    ...
}*/

bool MapController::insert_plant_full(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature)
{
    if(CatchChallenger::CommonDatapack::commonDatapack.get_plants().find(plant_id)==CatchChallenger::CommonDatapack::commonDatapack.get_plants().cend())
    {
        qDebug() << "plant_id don't exists";
        return false;
    }
    CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->get_player_informations();
    const std::pair<COORD_TYPE,COORD_TYPE> position=std::make_pair(x,y);
    if(player_private_and_public_informations.mapData.find(mapIndex)!=player_private_and_public_informations.mapData.cend())
    {
        CatchChallenger::Player_private_and_public_informations_Map &mapData=player_private_and_public_informations.mapData.at(mapIndex);
        if(mapData.plants.find(position)!=mapData.plants.cend())
        {
            qDebug() << "map have already an item at this point, remove it";
            remove_plant_full(mapIndex,x,y);
            return false;
        }
    }
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)==CatchChallenger::QMap_client::all_map.cend())
        return false;
    QMap_client * map_full=CatchChallenger::QMap_client::all_map[mapIndex];
    {
        auto oldIt=map_full->Qplants.find(position);
        if(oldIt!=map_full->Qplants.end())
        {
            delete oldIt->second;
            map_full->Qplants.erase(oldIt);
        }
    }

    uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    Tiled::MapObject *mapObj=new Tiled::MapObject();
    CatchChallenger::ClientPlantWithTimer *qplant=new CatchChallenger::ClientPlantWithTimer();
    map_full->Qplants[position]=qplant;
    qplant->mapObject=mapObj;
    qplant->setSingleShot(true);
    CatchChallenger::PlayerPlant plantData;
    plantData.plant=plant_id;
    plantData.mature_at=current_time+seconds_to_mature;
    player_private_and_public_informations.mapData[mapIndex].plants[position]=plantData;
    if(updatePlantGrowing(qplant))
        if(!connect(qplant,&QTimer::timeout,this,&MapController::getPlantTimerEvent))
            abort();
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    qplant->mapObject->setPosition(QPoint(x,y+1));

    #ifdef DEBUG_CLIENT_PLANTS
    qDebug() << QStringLiteral("insert_plant(), map: %1 at: %2,%3").arg(mapIndex).arg(x).arg(y);
    #endif
    if(ObjectGroupItem::objectGroupLink.find(map_full->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
        ObjectGroupItem::objectGroupLink[map_full->objectGroup]->addObject(qplant->mapObject);
    else
        qDebug() << "insert_plant(), all_map[map]->objectGroup not contains current_map->objectGroup";
    MapObjectItem::objectLink[qplant->mapObject]->setZValue(y);
    return true;
}

void MapController::getPlantTimerEvent()
{
    QTimer *clientPlantTimer=qobject_cast<QTimer *>(QObject::sender());
    if(clientPlantTimer==NULL)
        return;
    if(!mHaveTheDatapack || mapVisualiserThread.stopIt)
        return;
    updatePlantGrowing(static_cast<CatchChallenger::ClientPlantWithTimer *>(clientPlantTimer));
}

//return true if is growing
bool MapController::updatePlantGrowing(CatchChallenger::ClientPlantWithTimer *plant)
{
    //find which map and position this plant belongs to
    CATCHCHALLENGER_TYPE_MAPID foundMapIndex=0;
    std::pair<COORD_TYPE,COORD_TYPE> foundPos;
    bool found=false;
    for(const auto &mapEntry : CatchChallenger::QMap_client::all_map)
    {
        for(auto &plantEntry : mapEntry.second->Qplants)
        {
            if(plantEntry.second==plant)
            {
                foundMapIndex=mapEntry.first;
                foundPos=plantEntry.first;
                found=true;
                break;
            }
        }
        if(found)
            break;
    }
    if(!found)
        return false;

    CatchChallenger::Player_private_and_public_informations &playerInfo=client->get_player_informations();
    if(playerInfo.mapData.find(foundMapIndex)==playerInfo.mapData.cend())
        return false;
    const auto &plantsMap=playerInfo.mapData.at(foundMapIndex).plants;
    if(plantsMap.find(foundPos)==plantsMap.cend())
        return false;
    const CatchChallenger::PlayerPlant &plantData=plantsMap.at(foundPos);

    uint64_t currentTime=QDateTime::currentMSecsSinceEpoch()/1000;
    Tiled::Cell cell=plant->mapObject->cell();
    if(plantData.mature_at<=currentTime)
    {
        cell.setTile(QtDatapackClientLoader::datapackLoader->getPlantExtra(plantData.plant).tileset->tileAt(4));
        plant->mapObject->setCell(cell);
        return false;
    }
    int seconds_to_mature=static_cast<uint32_t>(plantData.mature_at-currentTime);
    const auto &plantDef=CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plantData.plant);
    int floweringDiff=plantDef.fruits_seconds-plantDef.flowering_seconds;
    int tallerDiff=plantDef.fruits_seconds-plantDef.taller_seconds;
    int sproutedDiff=plantDef.fruits_seconds-plantDef.sprouted_seconds;
    if(seconds_to_mature<floweringDiff)
    {
        plant->start(seconds_to_mature*1000);
        cell.setTile(QtDatapackClientLoader::datapackLoader->getPlantExtra(plantData.plant).tileset->tileAt(3));
    }
    else if(seconds_to_mature<tallerDiff)
    {
        plant->start((seconds_to_mature-floweringDiff)*1000);
        cell.setTile(QtDatapackClientLoader::datapackLoader->getPlantExtra(plantData.plant).tileset->tileAt(2));
    }
    else if(seconds_to_mature<sproutedDiff)
    {
        plant->start((seconds_to_mature-tallerDiff)*1000);
        cell.setTile(QtDatapackClientLoader::datapackLoader->getPlantExtra(plantData.plant).tileset->tileAt(1));
    }
    else
    {
        plant->start((seconds_to_mature-sproutedDiff)*1000);
        cell.setTile(QtDatapackClientLoader::datapackLoader->getPlantExtra(plantData.plant).tileset->tileAt(0));
    }
    plant->mapObject->setCell(cell);
    return true;
}

bool MapController::remove_plant_full(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    #ifdef DEBUG_CLIENT_PLANTS
    qDebug() << QStringLiteral("remove_plant(%1,%2,%3)").arg(mapIndex).arg(x).arg(y);
    #endif
    if(CatchChallenger::QMap_client::all_map.find(mapIndex)==CatchChallenger::QMap_client::all_map.cend())
    {
        qDebug() << QString("map (%1) is not into map list, ignore it").arg(mapIndex);
        return false;
    }
    if(!mapItem->haveMap(CatchChallenger::QMap_client::all_map[mapIndex]->tiledMap.get()))
    {
        qDebug() << QString("map (%1) not show, ignore it").arg(mapIndex);
        return false;
    }
    CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->get_player_informations();
    const std::pair<COORD_TYPE,COORD_TYPE> position=std::make_pair(x,y);
    if(player_private_and_public_informations.mapData.find(mapIndex)!=player_private_and_public_informations.mapData.cend())
    {
        CatchChallenger::Player_private_and_public_informations_Map &mapData=player_private_and_public_informations.mapData.at(mapIndex);
        mapData.plants.erase(position);
    }
    QMap_client * map_full=CatchChallenger::QMap_client::all_map[mapIndex];
    auto plantIt=map_full->Qplants.find(position);
    if(plantIt!=map_full->Qplants.end())
    {
        CatchChallenger::ClientPlantWithTimer *qplant=plantIt->second;
        qplant->stop();
        if(qplant->mapObject!=nullptr)
        {
            if(ObjectGroupItem::objectGroupLink.find(qplant->mapObject->objectGroup())!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink[qplant->mapObject->objectGroup()]->removeObject(qplant->mapObject);
            else
                qDebug() << "remove_plant(), ObjectGroupItem::objectGroupLink not contains mapObject->objectGroup()";
            delete qplant->mapObject;
        }
        delete qplant;
        map_full->Qplants.erase(plantIt);
    }
    return true;
}

void MapController::updateGrowing()
{
}

void MapController::seed_planted(const bool &ok)
{
    if(!mHaveTheDatapack)
    {
        qDebug() << "Prevent seed_planted before datapack is loaded";
        return;
    }
    Q_UNUSED(ok);
}

void MapController::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    if(!mHaveTheDatapack)
    {
        qDebug() << "Prevent seed_planted before datapack is loaded";
        return;
    }
    Q_UNUSED(stat);
}

void MapController::reinject_signals()
{
    MapControllerMP::reinject_signals();
    if(mHaveTheDatapack && player_informations_is_set)
    {
    }
}

void MapController::tryLoadPlantOnMapDisplayed(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    Q_UNUSED(mapIndex);
}
