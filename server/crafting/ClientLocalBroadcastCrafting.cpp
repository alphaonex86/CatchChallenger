#include "../base/Client.h"
#include "../base/BroadCastWithoutSender.h"
#include "../base/MapServer.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../base/GlobalServerData.h"

#include <QDateTime>

using namespace CatchChallenger;

void Client::plantSeed(const quint8 &query_id,const quint8 &plant_id)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("plantSeed(%1,%2)").arg(query_id).arg(plant_id));
    #endif
    if(!CommonDatapack::commonDatapack.plants.contains(plant_id))
    {
        errorOutput(QStringLiteral("plant_id not found: %1").arg(plant_id));
        return;
    }
    CommonMap *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the dirt
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("plantSeed() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("plantSeed() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("plantSeed() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to plant a seed");
        return;
    }
    //check if is dirt
    if(!MoveOnTheMap::isDirt(*map,x,y))
    {
        errorOutput("Try pu seed out of the dirt");
        return;
    }
    //check if is free
    const quint16 &size=static_cast<MapServer *>(map)->plants.size();
    quint16 index=0;
    while(index<size)
    {
        if(x==static_cast<MapServer *>(map)->plants.at(index).x && y==static_cast<MapServer *>(map)->plants.at(index).y)
        {
            QByteArray data;
            data[0]=0x02;
            postReply(query_id,data);
            return;
        }
        index++;
    }
    //check if have into the inventory
    PlantInWaiting plantInWaiting;
    plantInWaiting.query_id=query_id;
    plantInWaiting.plant_id=plant_id;
    plantInWaiting.map=map;
    plantInWaiting.x=x;
    plantInWaiting.y=y;

    plant_list_in_waiting << plantInWaiting;
    useSeed(plant_id);
}

void Client::seedValidated()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("seedValidated()"));
    #endif
    /* useless, clean the protocol
    if(!ok)
    {
        QByteArray data;
        data[0]=0x02;
        postReply(plant_list_in_waiting.first().query_id,data);
        plant_list_in_waiting.removeFirst();
        return;
    }*/
    //check if is free
    {
        const quint16 &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.size();
        quint16 index=0;
        while(index<size)
        {
            if(x==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).x && y==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).y)
            {
                addObjectAndSend(CommonDatapack::commonDatapack.plants.value(plant_list_in_waiting.first().plant_id).itemUsed);
                QByteArray data;
                data[0]=0x02;
                postReply(plant_list_in_waiting.first().query_id,data);
                plant_list_in_waiting.removeFirst();
                return;
            }
            index++;
        }
    }
    //is ok
    QByteArray data;
    data[0]=0x01;
    postReply(plant_list_in_waiting.first().query_id,data);
    quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    MapServerCrafting::PlantOnMap plantOnMap;
    if(GlobalServerData::serverPrivateVariables.plantUnusedId.isEmpty())
    {
        GlobalServerData::serverPrivateVariables.maxPlantId++;
        plantOnMap.id=GlobalServerData::serverPrivateVariables.maxPlantId;
    }
    else
        plantOnMap.id=GlobalServerData::serverPrivateVariables.plantUnusedId.takeFirst();
    plantOnMap.x=plant_list_in_waiting.first().x;
    plantOnMap.y=plant_list_in_waiting.first().y;
    plantOnMap.plant=plant_list_in_waiting.first().plant_id;
    plantOnMap.character=character_id;
    plantOnMap.mature_at=current_time+CommonDatapack::commonDatapack.plants.value(plantOnMap.plant).fruits_seconds;
    plantOnMap.player_owned_expire_at=current_time+CommonDatapack::commonDatapack.plants.value(plantOnMap.plant).fruits_seconds+CATCHCHALLENGER_SERVER_OWNER_TIMEOUT;
    static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants << plantOnMap;
    const QString &map_file=plant_list_in_waiting.first().map->map_file;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            dbQueryWrite(QStringLiteral("INSERT INTO `plant`(`id`,`map`,`x`,`y`,`plant`,`character`,`plant_timestamps`) VALUES(%1,'%2',%3,%4,%5,%6,%7);")
                         .arg(plantOnMap.id)
                         .arg(map_file)
                         .arg(plantOnMap.x)
                         .arg(plantOnMap.y)
                         .arg(plantOnMap.plant)
                         .arg(character_id)
                         .arg(current_time)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            dbQueryWrite(QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,'%2',%3,%4,%5,%6,%7);")
                     .arg(map_file)
                     .arg(plantOnMap.id)
                     .arg(plantOnMap.x)
                     .arg(plantOnMap.y)
                     .arg(plantOnMap.plant)
                     .arg(character_id)
                     .arg(current_time)
                     );
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            dbQueryWrite(QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,'%2',%3,%4,%5,%6,%7);")
                     .arg(plantOnMap.id)
                     .arg(map_file)
                     .arg(plantOnMap.x)
                     .arg(plantOnMap.y)
                     .arg(plantOnMap.plant)
                     .arg(character_id)
                     .arg(current_time)
                     );
        break;
    }

    //send to all player

    {
        QByteArray finalData;
        {
            //Insert plant on map
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint16)1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)map->id;
            else
                out << (quint32)map->id;
            out << plantOnMap.x;
            out << plantOnMap.y;
            out << plantOnMap.plant;
            if(current_time>=plantOnMap.mature_at)
                out << (quint16)0;
            else if((plantOnMap.mature_at-current_time)>65535)
            {
                normalOutput(QStringLiteral("sendNearPlant(): remaining seconds to mature is greater than the possibility: map: %1 (%2,%3), plant: %4").arg(map->map_file).arg(x).arg(y).arg(plantOnMap.plant));
                out << (quint16)(65535);
            }
            else
                out << (quint16)(plantOnMap.mature_at-current_time);
            finalData.resize(16+outputData.size());
            finalData.resize(ProtocolParsingInputOutput::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xD1,outputData.constData(),outputData.size()));
        }

        quint16 index=0;
        const quint16 &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.size();
        while(index<size)
        {
            static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData);
            index++;
        }
    }

    plant_list_in_waiting.removeFirst();
}

void Client::sendNearPlant()
{
    //Insert plant on map
    const quint16 &plant_list_size=static_cast<MapServer *>(map)->plants.size();
    if(plant_list_size==0)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << plant_list_size;
    int index=0;
    while(index<plant_list_size)
    {
        const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.at(index);
        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (quint8)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (quint16)map->id;
        else
            out << (quint32)map->id;
        out << plant.x;
        out << plant.y;
        out << plant.plant;
        quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
        if(current_time>=plant.mature_at)
            out << (quint16)0;
        else if((plant.mature_at-current_time)>65535)
        {
            normalOutput(QStringLiteral("sendNearPlant(): remaining seconds to mature is greater than the possibility: map: %1 (%2,%3), plant: %4").arg(map->map_file).arg(x).arg(y).arg(plant.plant));
            out << (quint16)(65535);
        }
        else
            out << (quint16)(plant.mature_at-current_time);
        #if defined(DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE) && defined(DEBUG_MESSAGE_MAP_PLANTS)
        int remaining_seconds_to_mature;
        if(current_time>=plant.mature_at)
            remaining_seconds_to_mature=0;
        else
            remaining_seconds_to_mature=(plant.mature_at-current_time);
        normalOutput(QStringLiteral("insert near plant: map: %1 (%2,%3), plant: %4, seconds to mature: %5 (current_time: %6, plant.mature_at: %7)").arg(map->map_file).arg(x).arg(y).arg(plant.plant).arg(remaining_seconds_to_mature).arg(current_time).arg(plant.mature_at));
        #endif
        index++;
    }
    sendPacket(0xD1,outputData);
}

void Client::removeNearPlant()
{
    #if defined(DEBUG_MESSAGE_MAP_PLANTS)
    normalOutput("removeNearPlant()");
    #endif
    //send the remove plant
    const quint16 &plant_list_size=static_cast<MapServer *>(map)->plants.size();
    if(plant_list_size==0)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << plant_list_size;
    int index=0;
    while(index<plant_list_size)
    {
        const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.at(index);
        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (quint8)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (quint16)map->id;
        else
            out << (quint32)map->id;
        out << plant.x;
        out << plant.y;
        #if defined(DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE) && defined(DEBUG_MESSAGE_MAP_PLANTS)
        normalOutput(QStringLiteral("remove near plant: map: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
        #endif
        index++;
    }
    sendPacket(0xD2,outputData);
}

void Client::collectPlant(const quint8 &query_id)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("collectPlant(%1)").arg(query_id));
    #endif
    CommonMap *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the dirt
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("collectPlant() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("collectPlant() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("collectPlant() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    errorOutput(QStringLiteral("collectPlant() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to plant a seed");
        return;
    }
    //check if is dirt
    if(!MoveOnTheMap::isDirt(*map,x,y))
    {
        errorOutput("Try pu seed out of the dirt");
        return;
    }
    //check if is free
    const quint64 &current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    const quint16 &size=static_cast<MapServer *>(map)->plants.size();
    quint16 index=0;
    while(index<size)
    {
        const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.at(index);
        if(x==plant.x && y==plant.y)
        {
            if(current_time<plant.mature_at)
            {
                QByteArray data;
                data[0]=Plant_collect_impossible;
                postReply(query_id,data);
                return;
            }
            //check if owned
            if(plant.character==character_id ||
                    current_time>plant.player_owned_expire_at ||
                    public_and_private_informations.public_informations.type==Player_type_gm ||
                    public_and_private_informations.public_informations.type==Player_type_dev
                    )
            {
                //remove plant from db
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("DELETE FROM `plant` WHERE `id`=%1").arg(plant.id));
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("DELETE FROM plant WHERE id=%1").arg(plant.id));
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("DELETE FROM plant WHERE id=%1").arg(plant.id));
                    break;
                }

                QByteArray finalData;
                {
                    //Remove plant on map
                    QByteArray outputData;
                    QDataStream out(&outputData, QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_4);
                    out << (quint16)1;
                    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                        out << (quint8)map->id;
                    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                        out << (quint16)map->id;
                    else
                        out << (quint32)map->id;
                    out << plant.x;
                    out << plant.y;
                    finalData.resize(16+outputData.size());
                    finalData.resize(ProtocolParsingInputOutput::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xD2,outputData.data(),outputData.size()));
                }

                {
                    //remove plan from all player display
                    int sub_index=0;
                    const quint16 &size=static_cast<MapServer *>(map)->clientsForBroadcast.size();
                    while(sub_index<size)
                    {
                        static_cast<MapServer *>(map)->clientsForBroadcast.at(sub_index)->sendRawSmallPacket(finalData);
                        sub_index++;
                    }
                }

                //add into the inventory
                float quantity=CommonDatapack::commonDatapack.plants.value(plant.plant).fix_quantity;
                if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.plants.value(plant.plant).random_quantity)
                    quantity++;

                QByteArray data;
                data[0]=Plant_collect_correctly_collected;
                postReply(query_id,data);
                addObjectAndSend(CommonDatapack::commonDatapack.plants.value(plant.plant).itemUsed,quantity);

                GlobalServerData::serverPrivateVariables.plantUnusedId << plant.id;
                static_cast<MapServer *>(map)->plants.removeAt(index);
                return;
            }
            else
            {
                QByteArray data;
                data[0]=Plant_collect_owned_by_another_player;
                postReply(query_id,data);
                return;
            }
        }
        index++;
    }
    QByteArray data;
    data[0]=Plant_collect_empty_dirt;
    postReply(query_id,data);
}
