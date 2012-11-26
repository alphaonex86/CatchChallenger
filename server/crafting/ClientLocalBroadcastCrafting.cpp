#include "../base/ClientLocalBroadcast.h"
#include "../base/BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/GlobalData.h"

#include <QDateTime>

using namespace Pokecraft;

void ClientLocalBroadcast::plantSeed(const quint8 &query_id,const quint8 &plant_id)
{
    if(!GlobalData::serverPrivateVariables.plants.contains(plant_id))
    {
        emit error(QString("plant_id not found: %1").arg(plant_id));
        return;
    }
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the dirt
    switch(last_direction)
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        default:
        emit error("Wrong direction to plant a seed");
        return;
    }
    //check if is free
    quint16 size=static_cast<MapServer *>(map)->plants.size();
    quint16 index=0;
    while(index<size)
    {
        if(x==static_cast<MapServer *>(map)->plants.at(index).x && y==static_cast<MapServer *>(map)->plants.at(index).y)
        {
            QByteArray data;
            data[0]=0x02;
            emit postReply(query_id,data);
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
    emit useSeed(plant_id);
}

void ClientLocalBroadcast::seedValidated()
{
    /* useless, clean the protocol
    if(!ok)
    {
        QByteArray data;
        data[0]=0x02;
        emit postReply(plant_list_in_waiting.first().query_id,data);
        plant_list_in_waiting.removeFirst();
        return;
    }*/
    //check if is free
    quint16 size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.size();
    quint16 index=0;
    while(index<size)
    {
        if(x==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).x && y==static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants.at(index).y)
        {
            emit addObject(GlobalData::serverPrivateVariables.plants[plant_list_in_waiting.first().plant_id].itemUsed);
            QByteArray data;
            data[0]=0x02;
            emit postReply(plant_list_in_waiting.first().query_id,data);
            plant_list_in_waiting.removeFirst();
            return;
        }
        index++;
    }
    //is ok
    QByteArray data;
    data[0]=0x01;
    emit postReply(plant_list_in_waiting.first().query_id,data);
    quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    MapServerCrafting::PlantOnMap plantOnMap;
    plantOnMap.x=plant_list_in_waiting.first().x;
    plantOnMap.y=plant_list_in_waiting.first().y;
    plantOnMap.plant=plant_list_in_waiting.first().plant_id;
    plantOnMap.player_id=player_informations->id;
    plantOnMap.mature_at=current_time+GlobalData::serverPrivateVariables.plants[plantOnMap.plant].mature_mins*60;
    plantOnMap.player_owned_expire_at=current_time+GlobalData::serverPrivateVariables.plants[plantOnMap.plant].mature_mins*60+60*60*24;
    static_cast<MapServer *>(plant_list_in_waiting.first().map)->plants << plantOnMap;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("INSERT INTO plant(map,x,y,plant,player_id,plant_timestamps) VALUES('%1',%2,%3,%4,%5,%6);")
                         .arg(SqlFunction::quoteSqlVariable(plant_list_in_waiting.first().map->map_file))
                         .arg(plantOnMap.x)
                         .arg(plantOnMap.y)
                         .arg(plantOnMap.plant)
                         .arg(player_informations->id)
                         .arg(current_time)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("INSERT INTO plant(map,x,y,plant,player_id,plant_timestamps) VALUES('%1',%2,%3,%4,%5,%6);")
                     .arg(SqlFunction::quoteSqlVariable(plant_list_in_waiting.first().map->map_file))
                     .arg(plantOnMap.x)
                     .arg(plantOnMap.y)
                     .arg(plantOnMap.plant)
                     .arg(player_informations->id)
                     .arg(current_time)
                     );
        break;
    }
    plant_list_in_waiting.removeFirst();
    //send to all player
    index=0;
    size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.size();
    while(index<size)
    {
        static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.at(index)->receiveSeed(plantOnMap,current_time);
        index++;
    }
}

void ClientLocalBroadcast::receiveSeed(const MapServerCrafting::PlantOnMap &plantOnMap,const quint64 &current_time)
{
    //Insert plant on map
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)1;
    if(GlobalData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    out << plantOnMap.x;
    out << plantOnMap.y;
    out << plantOnMap.plant;
    if(current_time<=plantOnMap.mature_at)
        out << (quint16)0;
    else
        out << (quint16)current_time-plantOnMap.mature_at;
    emit sendPacket(0xD1,outputData);
}

void ClientLocalBroadcast::removeSeed(const MapServerCrafting::PlantOnMap &plantOnMap)
{
    //Remove plant on map
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)1;
    if(GlobalData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    out << plantOnMap.x;
    out << plantOnMap.y;
    emit sendPacket(0xD2,outputData);
}

void ClientLocalBroadcast::sendNearPlant()
{
    //Insert plant on map
    quint16 plant_list_size=static_cast<MapServer *>(map)->plants.size();
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << plant_list_size;
    int index=0;
    while(index<plant_list_size)
    {
        const MapServerCrafting::PlantOnMap &plant=static_cast<MapServer *>(map)->plants.at(index);
        if(GlobalData::serverPrivateVariables.map_list.size()<=255)
            out << (quint8)map->id;
        else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
            out << (quint16)map->id;
        else
            out << (quint32)map->id;
        out << plant.x;
        out << plant.y;
        out << plant.plant;
        quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
        if(current_time<=plant.mature_at)
            out << (quint16)0;
        else
            out << (quint16)current_time-plant.mature_at;
        index++;
    }
    emit sendPacket(0xD1,outputData);
}

void ClientLocalBroadcast::collectPlant(const quint8 &query_id)
{
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the dirt
    switch(last_direction)
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
                MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y);
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        default:
        emit error("Wrong direction to plant a seed");
        return;
    }
    //check if is free
    quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
    quint16 size=static_cast<MapServer *>(map)->plants.size();
    quint16 index=0;
    while(index<size)
    {
        if(x==static_cast<MapServer *>(map)->plants.at(index).x && y==static_cast<MapServer *>(map)->plants.at(index).y)
        {
            if(current_time<static_cast<MapServer *>(map)->plants.at(index).mature_at)
            {
                QByteArray data;
                data[0]=0x04;
                emit postReply(query_id,data);
                return;
            }
            if(static_cast<MapServer *>(map)->plants.at(index).player_id==player_informations->id || current_time<static_cast<MapServer *>(map)->plants.at(index).player_owned_expire_at)
            {
                //remove plant from db
                switch(GlobalData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("DELETE FROM plant WHERE map=\'%1\' AND x=%2 AND y=%3")
                                     .arg(SqlFunction::quoteSqlVariable(plant_list_in_waiting.first().map->map_file))
                                     .arg(x)
                                     .arg(y)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("DELETE FROM plant WHERE map=\'%1\' AND x=%2 AND y=%3")
                                 .arg(SqlFunction::quoteSqlVariable(plant_list_in_waiting.first().map->map_file))
                                 .arg(x)
                                 .arg(y)
                                 );
                    break;
                }

                //remove plan from all player display
                index=0;
                size=static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.size();
                while(index<size)
                {
                    static_cast<MapServer *>(plant_list_in_waiting.first().map)->clientsForBroadcast.at(index)->removeSeed(static_cast<MapServer *>(map)->plants.at(index));
                    index++;
                }

                //add into the inventory
                QByteArray data;
                data[0]=0x01;
                emit postReply(query_id,data);
                float quantity=GlobalData::serverPrivateVariables.plants[static_cast<MapServer *>(map)->plants.at(index).plant].quantity;
                int integer_part=quantity;
                float random_part=quantity-integer_part;
                random_part*=10000;//random_part is 0 to 99
                if(random_part<=(rand()%10000))
                    quantity++;
                emit addObject(GlobalData::serverPrivateVariables.plants[static_cast<MapServer *>(map)->plants.at(index).plant].itemUsed,quantity);
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << (quint32)1;
                out << (quint32)GlobalData::serverPrivateVariables.plants[static_cast<MapServer *>(map)->plants.at(index).plant].itemUsed;
                out << (quint32)quantity;
                emit sendPacket(0xD0,0x0002,outputData);

                static_cast<MapServer *>(map)->plants.removeAt(index);
                return;
            }
            else
            {
                QByteArray data;
                data[0]=0x03;
                emit postReply(query_id,data);
                return;
            }
        }
        index++;
    }
    QByteArray data;
    data[0]=0x02;
    emit postReply(query_id,data);
}
