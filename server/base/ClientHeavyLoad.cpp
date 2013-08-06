#include "ClientHeavyLoad.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/Map.h"
#include "SqlFunction.h"
#include "LocalClientHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace CatchChallenger;

QList<quint16> ClientHeavyLoad::simplifiedIdList;

ClientHeavyLoad::ClientHeavyLoad()
{
}

ClientHeavyLoad::~ClientHeavyLoad()
{
}

void ClientHeavyLoad::setVariable(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
}

void ClientHeavyLoad::askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash)
{
    if(player_informations->isFake)
    {
        askLoginBot(query_id);
        return;
    }
    //id(0),login(1),skin(2),position_x(3),position_y(4),orientation(5),map_name(6),type(7),clan(8),cash(9)
    //rescue_map(10),rescue_x(11),rescue_y(12),rescue_orientation(13),unvalidated_rescue_map(14),unvalidated_rescue_x(15),unvalidated_rescue_y(16),unvalidated_rescue_orientation(17)
    //warehouse_cash(18),allow(19),clan_leader(20)
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,allow,clan_leader FROM player WHERE login=\"%1\" AND password=\"%2\"")
                .arg(SqlFunction::quoteSqlVariable(login))
                .arg(SqlFunction::quoteSqlVariable(QString(hash.toHex())));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,allow,clan_leader FROM player WHERE login=\"%1\" AND password=\"%2\"")
                .arg(SqlFunction::quoteSqlVariable(login))
                .arg(SqlFunction::quoteSqlVariable(QString(hash.toHex())));
        break;
    }
    QSqlQuery loginQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!loginQuery.exec(queryText))
        emit message(loginQuery.lastQuery()+": "+loginQuery.lastError().text());
    if(loginQuery.size()==0)
        loginIsWrong(query_id,"Bad login","Bad login for: "+login+", hash: "+hash.toHex());
    else
    {
        if(!loginQuery.next())
            loginIsWrong(query_id,"Wrong login/pass",QString("No login/pass found into the db, login: \"%1\", pass: \"%2\"").arg(login).arg(QString(hash.toHex())));
        else if(GlobalServerData::serverPrivateVariables.connected_players_id_list.contains(loginQuery.value(0).toUInt()))
            loginIsWrong(query_id,"Already logged","Already logged");
        else if(simplifiedIdList.size()<=0)
            loginIsWrong(query_id,"Not free id to login","Not free id to login");
        else
        {
            bool ok;
            player_informations->public_and_private_informations.clan=loginQuery.value(8).toUInt(&ok);
            if(!ok)
            {
                emit message(QString("clan id is not an number, clan disabled"));
                player_informations->public_and_private_informations.clan=0;//no clan
            }
            player_informations->public_and_private_informations.clan_leader=(loginQuery.value(20).toUInt(&ok)==1);
            if(!ok)
            {
                emit message(QString("clan_leader id is not an number, clan_leader disabled"));
                player_informations->public_and_private_informations.clan_leader=false;//no clan
            }
            player_informations->id=loginQuery.value(0).toUInt(&ok);
            if(!ok)
            {
                loginIsWrong(query_id,"Wrong account data",QString("Player is is not a number: %1").arg(loginQuery.value(0).toString()));
                return;
            }
            player_informations->public_and_private_informations.public_informations.pseudo=loginQuery.value(1).toString();
            QString skinString=loginQuery.value(2).toString();
            if(GlobalServerData::serverPrivateVariables.skinList.contains(skinString))
                player_informations->public_and_private_informations.public_informations.skinId=GlobalServerData::serverPrivateVariables.skinList[skinString];
            else
            {
                emit message(QString("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
                player_informations->public_and_private_informations.public_informations.skinId=0;
            }
            QString type=loginQuery.value(7).toString();
            if(type=="normal")
                player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
            else if(type=="premium")
                player_informations->public_and_private_informations.public_informations.type=Player_type_premium;
            else if(type=="gm")
                player_informations->public_and_private_informations.public_informations.type=Player_type_gm;
            else if(type=="dev")
                player_informations->public_and_private_informations.public_informations.type=Player_type_dev;
            else
            {
                emit message(QString("Mysql wrong type value").arg(type));
                player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
            }
            player_informations->public_and_private_informations.cash=loginQuery.value(9).toUInt(&ok);
            if(!ok)
            {
                emit message(QString("cash id is not an number, cash set to 0"));
                player_informations->public_and_private_informations.cash=0;
            }
            player_informations->public_and_private_informations.warehouse_cash=loginQuery.value(18).toUInt(&ok);
            if(!ok)
            {
                emit message(QString("warehouse cash id is not an number, warehouse cash set to 0"));
                player_informations->public_and_private_informations.warehouse_cash=0;
            }

            player_informations->public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
            if(!loadTheRawUTF8String())
            {
                loginIsWrong(query_id,"Convert into utf8 have wrong size","Convert into utf8 have wrong size");
                return;
            }
            QString orientationString=loginQuery.value(5).toString();
            Orientation orentation;
            if(orientationString=="top")
                orentation=Orientation_top;
            else if(orientationString=="bottom")
                orentation=Orientation_bottom;
            else if(orientationString=="left")
                orentation=Orientation_left;
            else if(orientationString=="right")
                orentation=Orientation_right;
            else
            {
                orentation=Orientation_bottom;
                emit message(QString("Wrong orientation corrected with bottom"));
            }
            player_informations->public_and_private_informations.allow=FacilityLib::QStringToAllow(loginQuery.value(19).toString());
            //all is rights
            if(GlobalServerData::serverPrivateVariables.map_list.contains(loginQuery.value(6).toString()))
            {
                quint8 x=loginQuery.value(3).toUInt(&ok);
                if(!ok)
                {
                    loginIsWrong(query_id,"Wrong account data","x coord is not a number");
                    return;
                }
                quint8 y=loginQuery.value(4).toUInt(&ok);
                if(!ok)
                {
                    loginIsWrong(query_id,"Wrong account data","y coord is not a number");
                    return;
                }
                if(x>=GlobalServerData::serverPrivateVariables.map_list[loginQuery.value(6).toString()]->width)
                {
                    loginIsWrong(query_id,"Wrong account data","x to out of map");
                    return;
                }
                if(y>=GlobalServerData::serverPrivateVariables.map_list[loginQuery.value(6).toString()]->height)
                {
                    loginIsWrong(query_id,"Wrong account data","y to out of map");
                    return;
                }
                loginIsRightWithRescue(query_id,
                    player_informations->id,
                    GlobalServerData::serverPrivateVariables.map_list[loginQuery.value(6).toString()],
                    x,
                    y,
                    (Orientation)orentation,
                        loginQuery.value(10),
                        loginQuery.value(11),
                        loginQuery.value(12),
                        loginQuery.value(13),
                        loginQuery.value(14),
                        loginQuery.value(15),
                        loginQuery.value(16),
                        loginQuery.value(17)
                );
            }
            else
                loginIsWrong(query_id,"Map not found","Map not found: "+loginQuery.value(6).toString());

        }
    }
}

void ClientHeavyLoad::askLoginBot(const quint8 &query_id)
{
    if(GlobalServerData::serverPrivateVariables.botSpawn.size()==0)
        loginIsWrong(query_id,"Not bot point","Not bot point");
    else
    {
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).map))
            loginIsWrong(query_id,"Bot point not resolved","Bot point not resolved");
        else if(simplifiedIdList.size()<=0)
            loginIsWrong(query_id,"Not free id to login","Not free id to login");
        else
        {
            player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
            player_informations->public_and_private_informations.clan=0;
            player_informations->public_and_private_informations.clan_leader=false;
            player_informations->id=999999999-GlobalServerData::serverPrivateVariables.number_of_bots_logged;
            player_informations->public_and_private_informations.public_informations.pseudo=QString("bot_%1").arg(player_informations->public_and_private_informations.public_informations.simplifiedId);
            player_informations->public_and_private_informations.public_informations.skinId=0x00;//use the first skin by alaphabetic order
            player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
            player_informations->public_and_private_informations.cash=0;
            player_informations->public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
            if(!loadTheRawUTF8String())
                loginIsWrong(query_id,"Convert into utf8 have wrong size",QString("Unable to convert the pseudo to utf8 at bot: %1").arg(QString("bot_%1").arg(player_informations->id)));
            else
            //all is rights
            {
                loginIsRight(query_id,
                     player_informations->id,
                     GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).map],
                     GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).x,
                     GlobalServerData::serverPrivateVariables.botSpawn.at(GlobalServerData::serverPrivateVariables.botSpawnIndex).y,
                     Orientation_bottom);

                GlobalServerData::serverPrivateVariables.botSpawnIndex++;
                if(GlobalServerData::serverPrivateVariables.botSpawnIndex>=GlobalServerData::serverPrivateVariables.botSpawn.size())
                    GlobalServerData::serverPrivateVariables.botSpawnIndex=0;
                GlobalServerData::serverPrivateVariables.number_of_bots_logged++;
            }
        }
    }
}

void ClientHeavyLoad::loginIsRightWithRescue(const quint8 &query_id, quint32 id, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(rescue_map.toString()))
    {
        emit message(QString("rescue map ,not found"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    bool ok;
    quint8 rescue_new_x=rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("rescue x coord is not a number"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    quint8 rescue_new_y=rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("rescue y coord is not a number"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->width)
    {
        emit message(QString("rescue x to out of map"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->height)
    {
        emit message(QString("rescue y to out of map"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    QString orientationString=rescue_orientation.toString();
    Orientation rescue_new_orientation;
    if(orientationString=="top")
        rescue_new_orientation=Orientation_top;
    else if(orientationString=="bottom")
        rescue_new_orientation=Orientation_bottom;
    else if(orientationString=="left")
        rescue_new_orientation=Orientation_left;
    else if(orientationString=="right")
        rescue_new_orientation=Orientation_right;
    else
    {
        rescue_new_orientation=Orientation_bottom;
        emit message(QString("Wrong rescue orientation corrected with bottom"));
    }
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(unvalidated_rescue_map.toString()))
    {
        emit message(QString("unvalidated rescue map ,not found"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    quint8 unvalidated_rescue_new_x=unvalidated_rescue_x.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("unvalidated rescue x coord is not a number"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    quint8 unvalidated_rescue_new_y=unvalidated_rescue_y.toUInt(&ok);
    if(!ok)
    {
        emit message(QString("unvalidated rescue y coord is not a number"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->width)
    {
        emit message(QString("unvalidated rescue x to out of map"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()]->height)
    {
        emit message(QString("unvalidated rescue y to out of map"));
        loginIsRight(query_id,id,map,x,y,orientation);
        return;
    }
    QString unvalidated_orientationString=unvalidated_rescue_orientation.toString();
    Orientation unvalidated_rescue_new_orientation;
    if(unvalidated_orientationString=="top")
        unvalidated_rescue_new_orientation=Orientation_top;
    else if(unvalidated_orientationString=="bottom")
        unvalidated_rescue_new_orientation=Orientation_bottom;
    else if(unvalidated_orientationString=="left")
        unvalidated_rescue_new_orientation=Orientation_left;
    else if(unvalidated_orientationString=="right")
        unvalidated_rescue_new_orientation=Orientation_right;
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        emit message(QString("Wrong unvalidated rescue orientation corrected with bottom"));
    }
    loginIsRightWithParsedRescue(query_id,id,map,x,y,orientation,
                                 GlobalServerData::serverPrivateVariables.map_list[rescue_map.toString()],rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 GlobalServerData::serverPrivateVariables.map_list[unvalidated_rescue_map.toString()],unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void ClientHeavyLoad::loginIsRight(const quint8 &query_id,quint32 id, Map *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
{
    loginIsRightWithParsedRescue(query_id,id,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void ClientHeavyLoad::loginIsRightWithParsedRescue(const quint8 &query_id, quint32 id, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  Map* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                  Map *unvalidated_rescue_map, const quint8 &unvalidated_rescue_x, const quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    loadLinkedData();

    //load the variables
    GlobalServerData::serverPrivateVariables.connected_players_id_list << id;
    player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
    simplifiedIdList.removeFirst();
    player_informations->is_logged=true;

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)02;
    out << (quint16)GlobalServerData::serverSettings.max_players;
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
    else
        out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        qint64 time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    out << (quint8)GlobalServerData::serverSettings.city.capture.frenquency;
    out << FacilityLib::allowToQString(player_informations->public_and_private_informations.allow);
    out << (quint32)player_informations->public_and_private_informations.clan;
    if(player_informations->public_and_private_informations.clan_leader)
        out << (quint8)0x01;
    else
        out << (quint8)0x00;
    out << (quint64)player_informations->public_and_private_informations.cash;
    out << (quint64)player_informations->public_and_private_informations.warehouse_cash;
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();

    //temporary variable
    quint32 index;
    quint32 size;

    //send recipes
    {
        index=0;
        out << (quint32)player_informations->public_and_private_informations.recipes.size();
        QSetIterator<quint32> k(player_informations->public_and_private_informations.recipes);
        while (k.hasNext())
            out << k.next();
    }

    //send monster
    index=0;
    size=player_informations->public_and_private_informations.playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=privateMonsterToBinary(player_informations->public_and_private_informations.playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }
    index=0;
    size=player_informations->public_and_private_informations.warehouse_playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=privateMonsterToBinary(player_informations->public_and_private_informations.warehouse_playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }

    /// \todo force to 255 max
    //send reputation
    {
        out << (quint8)player_informations->public_and_private_informations.reputation.size();
        QHashIterator<QString,PlayerReputation> i(player_informations->public_and_private_informations.reputation);
        while (i.hasNext()) {
            i.next();
            out << i.key();
            out << i.value().level;
            out << i.value().point;
        }
    }

    /// \todo force to 255 max
    //send quest
    {
        out << (quint8)player_informations->public_and_private_informations.quests.size();
        QHashIterator<quint32,PlayerQuest> j(player_informations->public_and_private_informations.quests);
        while (j.hasNext()) {
            j.next();
            out << j.key();
            out << j.value().step;
            out << j.value().finish_one_time;
        }
    }

    //send bot_already_beaten
    {
        out << (quint32)player_informations->public_and_private_informations.bot_already_beaten.size();
        QSetIterator<quint32> k(player_informations->public_and_private_informations.bot_already_beaten);
        while (k.hasNext())
            out << k.next();
    }

    emit postReply(query_id,outputData);
    sendInventory();

    player_informations->rescue.map=rescue_map;
    player_informations->rescue.x=rescue_x;
    player_informations->rescue.y=rescue_y;
    player_informations->rescue.orientation=rescue_orientation;
    player_informations->unvalidated_rescue.map=unvalidated_rescue_map;
    player_informations->unvalidated_rescue.x=unvalidated_rescue_x;
    player_informations->unvalidated_rescue.y=unvalidated_rescue_y;
    player_informations->unvalidated_rescue.orientation=unvalidated_rescue_orientation;

    //send signals into the server
    emit message(QString("Logged: %1 on the map: %2 (%3,%4)").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(map->map_file).arg(x).arg(y));
    emit send_player_informations();
    emit isLogged();
    emit put_on_the_map(
                map,//map pointer
        x,//position_x
        y,//position_y
        orientation
    );
}

void ClientHeavyLoad::loginIsWrong(const quint8 &query_id,const QString &messageToSend,const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)01;
    out << QString(messageToSend);
    emit postReply(query_id,outputData);

    //send to server to stop the connection
    emit error(debugMessage);
}

QByteArray ClientHeavyLoad::privateMonsterToBinary(const PlayerMonster &monster)
{
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint32)monster.id;
    out << (quint32)monster.monster;
    out << (quint8)monster.level;
    out << (quint32)monster.remaining_xp;
    out << (quint32)monster.hp;
    out << (quint32)monster.sp;
    out << (quint32)monster.captured_with;
    out << (quint8)monster.gender;
    out << (quint32)monster.egg_step;
    int sub_index=0;
    int sub_size=monster.buffs.size();
    out << (quint32)sub_size;
    while(sub_index<sub_size)
    {
        out << (quint32)monster.buffs.at(sub_index).buff;
        out << (quint8)monster.buffs.at(sub_index).level;
        sub_index++;
    }
    sub_index=0;
    sub_size=monster.skills.size();
    out << (quint32)sub_size;
    while(sub_index<sub_size)
    {
        out << (quint32)monster.skills.at(sub_index).skill;
        out << (quint8)monster.skills.at(sub_index).level;
        sub_index++;
    }
    return outputData;
}

//load linked data (like item, quests, ...)
void ClientHeavyLoad::loadLinkedData()
{
    if(player_informations->isFake)
        return;
    loadItems();
    loadRecipes();
    loadMonsters();
    loadReputation();
    loadQuests();
    loadBotAlreadyBeaten();
}

bool ClientHeavyLoad::loadTheRawUTF8String()
{
    player_informations->rawPseudo=FacilityLib::toUTF8(player_informations->public_and_private_informations.public_informations.pseudo);
    if(player_informations->rawPseudo.size()==0)
    {
        emit message(QString("Unable to convert the pseudo to utf8: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
        return false;
    }
    return true;
}

void ClientHeavyLoad::askIfIsReadyToStop()
{
    if(player_informations->is_logged)
    {
        simplifiedIdList << player_informations->public_and_private_informations.public_informations.simplifiedId;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.remove(player_informations->id);
    }
    emit isReadyToStop();
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void ClientHeavyLoad::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps)
{
    QHash<QString,quint64> filesList=GlobalServerData::serverPrivateVariables.filesList;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int loopIndex=0;
    int loop_size=files.size();
    //validate, remove or update the file actualy on the client
    while(loopIndex<loop_size)
    {
        QString fileName=files.at(loopIndex);
        quint32 mtime=timestamps.at(loopIndex);
        if(fileName.contains("./") || fileName.contains("\\") || fileName.contains("//"))
        {
            emit error(QString("file name contains illegale char: %1").arg(fileName));
            return;
        }
        if(fileName.contains(QRegularExpression("^[a-zA-Z]:/")) || fileName.startsWith("/"))
        {
            emit error(QString("start with wrong string: %1").arg(fileName));
            return;
        }
        if(!filesList.contains(fileName))
            out << (quint8)0x02;//to delete
        //the file on the client is already updated
        else
        {
            if(filesList[fileName]==mtime)
                out << (quint8)0x01;//found
            else
            {
                if(sendFile(fileName,filesList[fileName]))
                    out << (quint8)0x01;//found but updated
                else
                {
                    //disconnect to prevent desync of datapack
                    emit error("Unable to open datapack file, disconnect to prevent desync of datapack");
                    return;
                }
            }
            filesList.remove(fileName);
        }
        loopIndex++;
    }
    //send not in the list
    QHashIterator<QString,quint64> i(filesList);
    while (i.hasNext()) {
        i.next();
        sendFile(i.key(),i.value());
    }
    emit postReply(query_id,outputData);
}

bool ClientHeavyLoad::sendFile(const QString &fileName,const quint64 &mtime)
{
    if(fileName.size()>255 || fileName.size()==0)
        return false;
    QByteArray fileNameRaw=FacilityLib::toUTF8(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.size()==0)
        return false;
    QFile file(GlobalServerData::serverPrivateVariables.datapack_basePath+fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray content=file.readAll();
        /*emit message(QString("send the file: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
                 .arg(fileName)
                 .arg(checkMtime)
                 .arg(mtime)
                 .arg(localMtime)
        );*/
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << mtime;
        emit sendFullPacket(0xC2,0x0003,fileNameRaw+outputData+content);
        file.close();
        return true;
    }
    else
        return false;
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
    return text.replace("'","\\'");
}

void ClientHeavyLoad::dbQuery(const QString &queryText)
{
    if(player_informations->isFake)
    {
        emit message(QString("Query canceled because is fake: %1").arg(queryText));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    emit message("Do mysql query: "+queryText);
    #endif
    QSqlQuery sqlQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!sqlQuery.exec(queryText))
        emit message(sqlQuery.lastQuery()+": "+sqlQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.db->commit();//to have data coerancy and prevent data lost on crash
}

void ClientHeavyLoad::loadReputation()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT `type`,`point`,level FROM reputation WHERE player=%1")
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT `type`,`point`,level FROM reputation WHERE player=%1")
                .arg(player_informations->id);
        break;
    }
    bool ok;
    QSqlQuery reputationQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!reputationQuery.exec(queryText))
        emit message(reputationQuery.lastQuery()+": "+reputationQuery.lastError().text());
    //parse the result
    while(reputationQuery.next())
    {
        QString type=reputationQuery.value(0).toString();
        qint32 point=reputationQuery.value(1).toInt(&ok);
        if(!ok)
        {
            emit message(QString("point is not a number, skip: %1").arg(type));
            continue;
        }
        qint32 level=reputationQuery.value(2).toInt(&ok);
        if(!ok)
        {
            emit message(QString("level is not a number, skip: %1").arg(type));
            continue;
        }
        if(level<-100 || level>100)
        {
            emit message(QString("level is <100 or >100, skip: %1").arg(type));
            continue;
        }
        if(!CommonDatapack::commonDatapack.reputation.contains(type))
        {
            emit message(QString("The reputation: %1 don't exist").arg(type));
            continue;
        }
        if(level>=0)
        {
            if(level>=CommonDatapack::commonDatapack.reputation[type].reputation_positive.size())
            {
                emit message(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 > max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation[type].reputation_positive.size()));
                continue;
            }
        }
        else
        {
            if((-level)>CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                emit message(QString("The reputation level %1 is wrong because is out of range (reputation level: %2 < max level: %3)").arg(type).arg(level).arg(CommonDatapack::commonDatapack.reputation[type].reputation_negative.size()));
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation[type].reputation_positive.size()==(level+1))//start at level 0 in positive
            {
                emit message(QString("The reputation level is already at max, drop point"));
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(level+1))//start at level 0 in positive
            {
                emit message(QString("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation[type].reputation_negative.size()==-level)//start at level -1 in negative
            {
                emit message(QString("The reputation level is already at min, drop point"));
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-level))//start at level -1 in negative
            {
                emit message(QString("The reputation point %1 is greater than max %2").arg(point).arg(CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(level)));
                continue;
            }
        }
        player_informations->public_and_private_informations.reputation[type].level=level;
        player_informations->public_and_private_informations.reputation[type].point=point;
    }
}

void ClientHeavyLoad::loadQuests()
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT quest,finish_one_time,step FROM quest WHERE player=%1")
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT quest,finish_one_time,step FROM quest WHERE player=%1")
                .arg(player_informations->id);
        break;
    }
    bool ok,ok2;
    QSqlQuery questsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!questsQuery.exec(queryText))
        emit message(questsQuery.lastQuery()+": "+questsQuery.lastError().text());
    //parse the result
    while(questsQuery.next())
    {
        PlayerQuest playerQuest;
        quint32 id=questsQuery.value(0).toUInt(&ok);
        playerQuest.finish_one_time=questsQuery.value(1).toBool();
        playerQuest.step=questsQuery.value(2).toUInt(&ok2);
        if(!ok || !ok2)
        {
            emit message(QString("wrong value type for quest, skip: %1").arg(id));
            continue;
        }
        if(!CommonDatapack::commonDatapack.quests.contains(id))
        {
            emit message(QString("quest is not into the quests list, skip: %1").arg(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapack::commonDatapack.quests[id].steps.size())
        {
            emit message(QString("step out of quest range, skip: %1").arg(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            emit message(QString("can't be to step 0 if have never finish the quest, skip: %1").arg(id));
            continue;
        }
        player_informations->public_and_private_informations.quests[id]=playerQuest;
        LocalClientHandler::addQuestStepDrop(player_informations,id,playerQuest.step);
    }
}

void ClientHeavyLoad::askClan(const quint32 &clanId)
{
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
        queryText=QString("SELECT name,cash FROM clan WHERE id=%1")
                .arg(clanId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        queryText=QString("SELECT name,cash FROM clan WHERE id=%1")
                .arg(clanId);
        break;
    }
    QSqlQuery clanQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!clanQuery.exec(queryText))
        emit message(clanQuery.lastQuery()+": "+clanQuery.lastError().text());
    //parse the result
    if(clanQuery.next())
    {
        bool ok;
        quint64 cash=clanQuery.value(1).toULongLong(&ok);
        if(!ok)
        {
            cash=0;
            emit message("Warning: clan linked: %1 have wrong cash value, then reseted to 0");
        }
        emit haveClanInfo(clanQuery.value(0).toString(),cash);
    }
    else
        emit message("Warning: clan linked: %1 is not found into db");
}
