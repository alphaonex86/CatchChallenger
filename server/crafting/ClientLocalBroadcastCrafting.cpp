void ClientLocalBroadcast::plantSeed(const quint8 &query_id,const quint8 &plant_id)
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

void ClientLocalBroadcast::seedValidated(const bool &ok)
{
    if(!ok)
    {
        QByteArray data;
        data[0]=0x02;
        emit postReply(plant_list_in_waiting.first().query_id,data);
        plant_list_in_waiting.removeFirst();
        return;
    }
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
    //send the plant
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)1;
    out << plantOnMap.x;
    out << plantOnMap.y;
    out << plantOnMap.plant;
    if(current_time<=plantOnMap.mature_at)
        out << (quint16)0;
    else
        out << (quint16)current_time-plantOnMap.mature_at;
    emit sendPacket(0xD1,outputData);
}

void ClientLocalBroadcast::collectPlant(const quint8 &query_id)
{
}
