#include "ClientHeavyLoad.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/Map.h"
#include "SqlFunction.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <QTime>

#define WAIT(a) {QTime time;while(time.elapsed()<a){}}

using namespace Pokecraft;

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
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan,cash FROM player WHERE login=\"%1\" AND password=\"%2\"")
                .arg(SqlFunction::quoteSqlVariable(login))
                .arg(SqlFunction::quoteSqlVariable(QString(hash.toHex())));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan,cash FROM player WHERE login=\"%1\" AND password=\"%2\"")
                .arg(SqlFunction::quoteSqlVariable(login))
                .arg(SqlFunction::quoteSqlVariable(QString(hash.toHex())));
        break;
    }
    QSqlQuery loginQuery(queryText);
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
            player_informations->public_and_private_informations.public_informations.clan=loginQuery.value(8).toUInt(&ok);
            if(!ok)
            {
                emit message(QString("clan id is not an number, clan disabled"));
                player_informations->public_and_private_informations.public_informations.clan=0;//no clan
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

            player_informations->public_and_private_informations.public_informations.speed=POKECRAFT_SERVER_NORMAL_SPEED;
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
            //id(0),login(1),skin(2),position_x(3),position_y(4),orientation(5),map_name(6),type(7),clan(8)
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
                loginIsRight(query_id,
                     player_informations->id,
                     GlobalServerData::serverPrivateVariables.map_list[loginQuery.value(6).toString()],
                     x,
                     y,
                     (Orientation)orentation);
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
            player_informations->public_and_private_informations.public_informations.clan=0;
            player_informations->id=999999999-GlobalServerData::serverPrivateVariables.number_of_bots_logged;
            player_informations->public_and_private_informations.public_informations.pseudo=QString("bot_%1").arg(player_informations->public_and_private_informations.public_informations.simplifiedId);
            player_informations->public_and_private_informations.public_informations.skinId=0x00;//use the first skin by alaphabetic order
            player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
            player_informations->public_and_private_informations.cash=0;
            player_informations->public_and_private_informations.public_informations.speed=POKECRAFT_SERVER_NORMAL_SPEED;
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

void ClientHeavyLoad::loginIsRight(const quint8 &query_id,quint32 id, Map *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
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
    out << (quint64)player_informations->public_and_private_informations.cash;
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();

    //temporary variable
    quint32 index,sub_index;
    quint32 size,sub_size;

    //send recipes
    index=0;
    size=player_informations->public_and_private_informations.recipes.size();
    out << (quint32)size;
    while(index<size)
    {
        out << (quint32)player_informations->public_and_private_informations.recipes.at(index);
        index++;
    }

    //send monster
    index=0;
    size=player_informations->public_and_private_informations.playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        const PlayerMonster &monster=player_informations->public_and_private_informations.playerMonster.at(index);
        out << (quint32)monster.monster;
        out << (quint8)monster.level;
        out << (quint32)monster.remaining_xp;
        out << (quint32)monster.hp;
        out << (quint32)monster.sp;
        out << (quint32)monster.captured_with;
        out << (quint8)monster.gender;
        out << (quint32)monster.egg_step;
        sub_index=0;
        sub_size=monster.buffs.size();
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
        index++;
    }

    emit postReply(query_id,outputData);
    sendInventory();

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

//load linked data (like item, quests, ...)
void ClientHeavyLoad::loadLinkedData()
{
    if(player_informations->isFake)
        return;
    loadItems();
    loadRecipes();
    loadMonsters();
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
        if(fileName.contains(QRegExp("^[a-zA-Z]:/")) || fileName.startsWith("/"))
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
        emit sendPacket(0xC2,0x0003,fileNameRaw+outputData+content);
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
    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(queryText))
        emit message(sqlQuery.lastQuery()+": "+sqlQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.db->commit();//to have data coerancy and prevent data lost on crash
}
