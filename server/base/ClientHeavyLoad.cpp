#include "ClientHeavyLoad.h"
#include "GlobalData.h"

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
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan FROM player WHERE login=\"%1\" AND password=\"%2\"")
                .arg(SqlFunction::quoteSqlVariable(login))
                .arg(SqlFunction::quoteSqlVariable(QString(hash.toHex())));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,pseudo,skin,position_x,position_y,orientation,map_name,type,clan FROM player WHERE login=\"%1\" AND password=\"%2\"")
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
        else if(GlobalData::serverPrivateVariables.connected_players_id_list.contains(loginQuery.value(0).toUInt()))
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
            if(GlobalData::serverPrivateVariables.skinList.contains(skinString))
                player_informations->public_and_private_informations.public_informations.skinId=GlobalData::serverPrivateVariables.skinList[skinString];
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
            player_informations->public_and_private_informations.cash=0;
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
            }//id(0),login(1),skin(2),position_x(3),position_y(4),orientation(5),map_name(6),type(7),clan(8)
            //all is rights
            if(GlobalData::serverPrivateVariables.map_list.contains(loginQuery.value(6).toString()))
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
                             GlobalData::serverPrivateVariables.map_list[loginQuery.value(6).toString()],
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
    if(GlobalData::serverPrivateVariables.botSpawn.size()==0)
        loginIsWrong(query_id,"Not bot point","Not bot point");
    else
    {
        if(!GlobalData::serverPrivateVariables.map_list.contains(GlobalData::serverPrivateVariables.botSpawn.at(GlobalData::serverPrivateVariables.botSpawnIndex).map))
            loginIsWrong(query_id,"Bot point not resolved","Bot point not resolved");
        else if(simplifiedIdList.size()<=0)
            loginIsWrong(query_id,"Not free id to login","Not free id to login");
        else
        {
            player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
            player_informations->public_and_private_informations.public_informations.clan=0;
            player_informations->id=999999999-GlobalData::serverPrivateVariables.number_of_bots_logged;
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
                             GlobalData::serverPrivateVariables.map_list[GlobalData::serverPrivateVariables.botSpawn.at(GlobalData::serverPrivateVariables.botSpawnIndex).map],
                             GlobalData::serverPrivateVariables.botSpawn.at(GlobalData::serverPrivateVariables.botSpawnIndex).x,
                             GlobalData::serverPrivateVariables.botSpawn.at(GlobalData::serverPrivateVariables.botSpawnIndex).y,
                             Orientation_bottom);

                GlobalData::serverPrivateVariables.botSpawnIndex++;
                if(GlobalData::serverPrivateVariables.botSpawnIndex>=GlobalData::serverPrivateVariables.botSpawn.size())
                    GlobalData::serverPrivateVariables.botSpawnIndex=0;
                GlobalData::serverPrivateVariables.number_of_bots_logged++;
            }
        }
    }
}

void ClientHeavyLoad::loginIsRight(const quint8 &query_id,quint32 id, Map *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
{
    #ifdef POKECRAFT_EXTRA_CHECK
    if(map->rawMapFile.isEmpty())
    {
        loginIsWrong(query_id,"Internal error",QString("Raw map is wrong: %1").arg(map->map_file));
        return;
    }
    #endif

    //load the variables
    GlobalData::serverPrivateVariables.connected_players_id_list << id;
    player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
    simplifiedIdList.removeFirst();
    player_informations->is_logged=true;

    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)02;
    out << (quint16)GlobalData::serverSettings.max_players;
    if(GlobalData::serverSettings.max_players<=255)
        out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
    else
        out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
    out << (quint32)player_informations->public_and_private_informations.cash;
    emit postReply(query_id,outputData);

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

    loadLinkedData();
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
    loadItems();
}

void ClientHeavyLoad::loadItems()
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    //do the query
    QString queryText;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=\"%1\"")
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=\"%1\"")
                .arg(player_informations->id);
        break;
    }
    bool ok;
    QSqlQuery loginQuery(queryText);
    out << (quint32)loginQuery.size();

    //parse the result
    while(loginQuery.next())
    {
        quint32 id=loginQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("item id is not a number, skip"));
            continue;
        }
        quint32 quantity=loginQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            emit message(QString("quantity is not a number, skip"));
            continue;
        }
        if(quantity==0)
        {
            switch(GlobalData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    QSqlQuery loginQuery(QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2")
                                         .arg(player_informations->id)
                                         .arg(id)
                                         );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    QSqlQuery loginQuery(QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2")
                                     .arg(player_informations->id)
                                     .arg(id)
                                     );
                break;
            }
            emit message(QString("The item %1 have been dropped because the quantity is 0").arg(item_id));
            continue;
        }
        if(!GlobalData::serverPrivateVariables.itemsId.contains(id))
        {
            emit message(QString("The item %1 is ignored because it's not into the items list").arg(item_id));
            continue;
        }
        player_informations->public_and_private_informations.items[id]=quantity;

        out << (quint32)id;
        out << (quint32)quantity;
    }

    //send the items
    emit sendPacket(0xD0,0x0001,outputData);
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
        GlobalData::serverPrivateVariables.connected_players_id_list.remove(player_informations->id);
    }
    emit isReadyToStop();
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void ClientHeavyLoad::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &timestamps)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int loopIndex=0;
    int loop_size=files.size();
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
        if(!fileName.contains(GlobalData::serverPrivateVariables.datapack_rightFileName))
        {
            //emit error(QString("file have not match the regex: %1").arg(fileName));
            //return;
            out << (quint8)0x02;
        }
        else
        {
            if(sendFileIfNeeded(GlobalData::serverPrivateVariables.datapack_basePath+fileName,fileName,mtime,true))
                out << (quint8)0x01;
            else
                out << (quint8)0x02;
        }
        loopIndex++;
    }
    //send not in the list
    listDatapack("",files);
    emit postReply(query_id,qCompress(outputData,9));
}

/** \brief send file if new or need be updated
 * \return return false if need be removed */
bool ClientHeavyLoad::sendFileIfNeeded(const QString &filePath,const QString &fileName,const quint32 &mtime,const bool &checkMtime)
{
    QFile file(filePath);
    if(file.size()>8*1024*1024)
        return false;
    quint64 localMtime=QFileInfo(file).lastModified().toTime_t();
    //the file on the client is already updated
    if(checkMtime && localMtime==mtime)
        return true;
    //the file on the client not exists on the server, then remove it
    if(!file.exists())
        return false;
    //the file need be downloaded because it's new
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray content=file.readAll();
        /*emit message(QString("send the file: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
                 .arg(fileName)
                 .arg(checkMtime)
                 .arg(mtime)
                 .arg(localMtime)
        );*/
        bool returnVal=sendFile(fileName,content,localMtime);
        file.close();
        return returnVal;
    }
    else
    {
        emit message("Unable to open: "+filePath+", error: "+file.errorString());
        return false;
    }
}

void ClientHeavyLoad::listDatapack(const QString &suffix,const QStringList &files)
{
    //do source check
    QDir finalDatapackFolder(GlobalData::serverPrivateVariables.datapack_basePath+suffix);
    QString fileName;
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            listDatapack(suffix+fileInfo.fileName()+"/",files);//put unix separator because it's transformed into that's under windows too
        else
        {
            fileName=suffix+fileInfo.fileName();
            if(fileName.contains(GlobalData::serverPrivateVariables.datapack_rightFileName))
            {
                if(!files.contains(fileName))
                    sendFileIfNeeded(GlobalData::serverPrivateVariables.datapack_basePath+fileName,fileName,0,false);
            }
        }
    }
}

bool ClientHeavyLoad::sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime)
{
    if(fileName.size()>255 || fileName.size()==0)
        return false;
    QByteArray fileNameRaw=FacilityLib::toUTF8(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.size()==0)
        return false;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << mtime;
    emit sendPacket(0xC2,0x0003,fileNameRaw+outputData+content);
    return true;
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
    return text.replace("'","\\'");
}

void ClientHeavyLoad::dbQuery(const QString &queryText)
{
    QSqlQuery sqlQuery(queryText);
    if(!sqlQuery.exec())
        emit message(sqlQuery.lastQuery()+": "+sqlQuery.lastError().text());
}

void ClientHeavyLoad::askedRandomNumber()
{
    QByteArray randomData;
    QDataStream randomDataStream(&randomData, QIODevice::WriteOnly);
    randomDataStream.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<POKECRAFT_SERVER_RANDOM_LIST_SIZE)
    {
        randomDataStream << quint8(rand()%256);
        index++;
    }
    emit newRandomNumber(randomData);
    emit sendPacket(0xD0,0x0009,randomData);
}
