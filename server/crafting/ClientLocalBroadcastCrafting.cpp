#include "../base/Client.h"
#include "../base/BroadCastWithoutSender.h"
#include "../base/MapServer.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../base/GlobalServerData.h"
#include "../base/PreparedDBQuery.h"

#include <QDateTime>

using namespace CatchChallenger;

void Client::plantSeed(
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        const quint8 &query_id,
        #endif
        const quint8 &plant_id)
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
    if(!static_cast<MapServer *>(map)->plants.contains(QPair<quint8,quint8>(x,y)))
    {
        errorOutput("Try put seed out of the dirt");
        return;
    }
    //check if is free
    {
        const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(map)->plants.value(QPair<quint8,quint8>(x,y));
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        if(public_and_private_informations.plantOnMap.contains(plantOnMap.indexOfOnMap))
        {
            errorOutput("Have already a plant in plantOnlyVisibleByPlayer==true");
            return;
        }
        #else
        if(plantOnMap.character!=0)
        {
            QByteArray data;
            data[0]=0x02;
            postReply(query_id,data.constData(),data.size());
            return;
        }
        #endif
    }
    //check if have into the inventory
    PlantInWaiting plantInWaiting;
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    plantInWaiting.query_id=query_id;
    #endif
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
    /* check if is free already done into Client::plantSeed()
    {
        const quint16 &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.size();
        quint16 index=0;
        while(index<size)
        {
            if(x==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).x && y==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).y)
            {
                addObject(CommonDatapack::commonDatapack.plants.value(plant_list_in_waiting.first().plant_id).itemUsed);
                QByteArray data;
                data[0]=0x02;
                postReply(plant_list_in_waiting.first().query_id,data.constData(),data.size());
                plant_list_in_waiting.removeFirst();
                return;
            }
            index++;
        }
    }*/
    //post the reply
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    QByteArray data;
    data[0]=0x01;
    postReply(plant_list_in_waiting.first().query_id,data.constData(),data.size());
    #endif

    const quint64 &current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    const QPair<quint8,quint8> pos(plant_list_in_waiting.first().x,plant_list_in_waiting.first().y);
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    const MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.value(pos);
    {
        PlayerPlant plantOnMapPlayer;
        plantOnMapPlayer.plant=plant_list_in_waiting.first().plant_id;
        plantOnMapPlayer.mature_at=current_time+CommonDatapack::commonDatapack.plants.value(plantOnMapPlayer.plant).fruits_seconds;
        public_and_private_informations.plantOnMap[plantOnMap.indexOfOnMap]=plantOnMapPlayer;
    }
    #else
    MapServer::PlantOnMap &plantOnMap=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants[pos];
    {
        plantOnMap.plant=plant_list_in_waiting.first().plant_id;
        plantOnMap.character=character_id;
        plantOnMap.mature_at=current_time+CommonDatapack::commonDatapack.plants.value(plantOnMap.plant).fruits_seconds;
        plantOnMap.player_owned_expire_at=current_time+CommonDatapack::commonDatapack.plants.value(plantOnMap.plant).fruits_seconds+CATCHCHALLENGER_SERVER_OWNER_TIMEOUT;
        static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants[pos]=plantOnMap;
    }
    #endif
    dbQueryWriteServer(PreparedDBQueryServer::db_query_insert_plant
                 .arg(character_id)
                 .arg(plantOnMap.pointOnMapDbCode)
                 .arg(plant_list_in_waiting.first().plant_id)
                 .arg(current_time)
                 );

    //send to all player
    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    {
        QByteArray finalData;
        {
            //Insert plant on map
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (quint16)1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)map->id;
            else
                out << (quint32)map->id;
            out << pos.first;
            out << pos.second;
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
            finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xD1,outputData.constData(),outputData.size()));
        }

        quint16 index=0;
        const quint16 &size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.size();
        while(index<size)
        {
            static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData.constData(),finalData.size());
            index++;
        }
    }
    #endif

    plant_list_in_waiting.removeFirst();
}

#ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void Client::sendNearPlant()
{
    //Insert plant on map
    if(static_cast<MapServer *>(map)->plants.isEmpty())
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << static_cast<MapServer *>(map)->plants.size();

    QHashIterator<QPair<quint8,quint8>,MapServerCrafting::PlantOnMap> i(static_cast<MapServer *>(map)->plants);
    while (i.hasNext()) {
        i.next();

        const MapServerCrafting::PlantOnMap &plant=i.value();
        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (quint8)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (quint16)map->id;
        else
            out << (quint32)map->id;
        out << i.key().first;
        out << i.key().second;
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
    }
    sendPacket(0xD1,outputData.constData(),outputData.size());
}

/// \todo if confirmed to useless remove
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
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << plant_list_size;
    QHashIterator<QPair<quint8,quint8>,MapServerCrafting::PlantOnMap> i(static_cast<MapServer *>(map)->plants);
    while (i.hasNext()) {
        i.next();

        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            out << (quint8)map->id;
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
            out << (quint16)map->id;
        else
            out << (quint32)map->id;
        out << i.key().first;
        out << i.key().second;
        #if defined(DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE) && defined(DEBUG_MESSAGE_MAP_PLANTS)
        normalOutput(QStringLiteral("remove near plant: map: %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
        #endif
    }
    sendPacket(0xD2,outputData.constData(),outputData.size());
}
#endif

void Client::collectPlant(
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        const quint8 &query_id
        #endif
        )
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
        errorOutput("Try collect plant out of the dirt");
        return;
    }
    //check if is free
    const quint64 &current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.value(QPair<quint8,quint8>(x,y));
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    if(public_and_private_informations.plantOnMap.contains(plant.indexOfOnMap))
    {
        const PlayerPlant &playerPlant=public_and_private_informations.plantOnMap.value(plant.indexOfOnMap);
        if(current_time<playerPlant.mature_at)
        {
            errorOutput("current_time<plant.mature_at");
            return;
        }
        //remove from db
        dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_plant_by_index.arg(character_id).arg(plant.pointOnMapDbCode));

        //add into the inventory
        float quantity=CommonDatapack::commonDatapack.plants.value(playerPlant.plant).fix_quantity;
        if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.plants.value(playerPlant.plant).random_quantity)
            quantity++;

        //send the object collected to the current character
        addObjectAndSend(CommonDatapack::commonDatapack.plants.value(playerPlant.plant).itemUsed,quantity);

        //clear the server dirt
        public_and_private_informations.plantOnMap.remove(plant.indexOfOnMap);
        return;
    }
    else
    {
        errorOutput("!public_and_private_informations.plantOnMap.contains(plant.indexOfOnMap)");
        return;
    }
    #else
    if(current_time<plant.mature_at)
    {
        QByteArray data;
        data[0]=Plant_collect_impossible;
        postReply(query_id,data.constData(),data.size());
        return;
    }
    if(plant.character==0)
    {
        QByteArray data;
        data[0]=Plant_collect_empty_dirt;
        postReply(query_id,data.constData(),data.size());
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
        dbQueryWriteServer(PreparedDBQueryServer::db_query_delete_plant_by_index.arg(character_id).arg(plant.pointOnMapDbCode));

        QByteArray finalData;
        {
            //Remove plant on map
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (quint16)1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)map->id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)map->id;
            else
                out << (quint32)map->id;
            out << x;
            out << y;
            finalData.resize(16+outputData.size());
            finalData.resize(ProtocolParsingBase::computeOutcommingData(
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
                static_cast<MapServer *>(map)->clientsForBroadcast.at(sub_index)->sendRawSmallPacket(finalData.constData(),finalData.size());
                sub_index++;
            }
        }

        //add into the inventory
        float quantity=CommonDatapack::commonDatapack.plants.value(plant.plant).fix_quantity;
        if((rand()%RANDOM_FLOAT_PART_DIVIDER)<=CommonDatapack::commonDatapack.plants.value(plant.plant).random_quantity)
            quantity++;

        //send the object collected to the current character
        QByteArray data;
        data[0]=Plant_collect_correctly_collected;
        postReply(query_id,data.constData(),data.size());
        addObjectAndSend(CommonDatapack::commonDatapack.plants.value(plant.plant).itemUsed,quantity);

        //clear the server dirt
        GlobalServerData::serverPrivateVariables.plantUnusedId << plant.pointOnMapDbCode;

        static_cast<MapServer *>(map)->plants[QPair<quint8,quint8>(x,y)].character=0;
        return;
    }
    else
    {
        QByteArray data;
        data[0]=Plant_collect_owned_by_another_player;
        postReply(query_id,data.constData(),data.size());
        return;
    }
    #endif
}
