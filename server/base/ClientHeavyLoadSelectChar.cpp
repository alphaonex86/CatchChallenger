#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "SqlFunction.h"

using namespace CatchChallenger;

void Client::characterSelectionIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)returnCode;
    postReply(query_id,outputData);

    //send to server to stop the connection
    errorOutput(debugMessage);
}

void Client::selectCharacter(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_character_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query db_query_update_character_last_connect is empty, bug"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug"));
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_character_by_id.arg(characterId);
    if(!GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::selectCharacter_static))
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        characterSelectionIsWrong(query_id,0x02,queryText+QLatin1String(": ")+GlobalServerData::serverPrivateVariables.db.errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
        paramToPassToCallBack << selectCharacterParam;
}

void Client::selectCharacter_static(void *object)
{
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.takeFirst());
    static_cast<Client *>(object)->selectCharacter_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::selectCharacter_return(const quint8 &query_id,const quint32 &characterId)
{
    /*account(0),pseudo(1),skin(2),x(3),y(4),orientation(5),map(6),type(7),clan(8),cash(9),
    rescue_map(10),rescue_x(11),rescue_y(12),rescue_orientation(13),unvalidated_rescue_map(14),
    unvalidated_rescue_x(15),unvalidated_rescue_y(16),unvalidated_rescue_orientation(17),
    warehouse_cash(18),allow(19),clan_leader(20),market_cash(21),
    time_to_delete(22),*/
    if(!GlobalServerData::serverPrivateVariables.db.next())
    {
        characterSelectionIsWrong(query_id,0x02,QLatin1String("Result return query wrong"));
        return;
    }

    bool ok;
    public_and_private_informations.clan=QString(GlobalServerData::serverPrivateVariables.db.value(8)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("clan id is not an number, clan disabled"));
        public_and_private_informations.clan=0;//no clan
    }
    public_and_private_informations.clan_leader=(QString(GlobalServerData::serverPrivateVariables.db.value(20)).toUInt(&ok)==1);
    if(!ok)
    {
        normalOutput(QStringLiteral("clan_leader id is not an number, clan_leader disabled"));
        public_and_private_informations.clan_leader=false;//no clan
    }

    const quint32 &account_id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(this->account_id));
        return;
    }
    if(character_loaded)
    {
        characterSelectionIsWrong(query_id,0x03,QLatin1String("character_loaded already to true"));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.contains(characterId))
    {
        characterSelectionIsWrong(query_id,0x03,QLatin1String("Already logged"));
        return;
    }
    if(simplifiedIdList.size()<=0)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("Not free id to login"));
        return;
    }
    if(!loadTheRawUTF8String())
    {
        if(GlobalServerData::serverSettings.anonymous)
            characterSelectionIsWrong(query_id,0x04,QStringLiteral("Unable to convert the pseudo to utf8 for character id: %1").arg(character_id));
        else
            characterSelectionIsWrong(query_id,0x04,QStringLiteral("Unable to convert the pseudo to utf8: %1").arg(public_and_private_informations.public_informations.pseudo));
        return;
    }
    if(GlobalServerData::serverSettings.anonymous)
        normalOutput(QStringLiteral("Charater id is logged: %1").arg(characterId));
    else
        normalOutput(QStringLiteral("Charater is logged: %1").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
    const quint32 &time_to_delete=QString(GlobalServerData::serverPrivateVariables.db.value(22)).toUInt(&ok);
    if(!ok || time_to_delete>0)
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete.arg(characterId));
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));

    public_and_private_informations.public_informations.pseudo=QString(GlobalServerData::serverPrivateVariables.db.value(1));
    const QString &skinString=QString(GlobalServerData::serverPrivateVariables.db.value(2));
    if(GlobalServerData::serverPrivateVariables.skinList.contains(skinString))
        public_and_private_informations.public_informations.skinId=GlobalServerData::serverPrivateVariables.skinList.value(skinString);
    else
    {
        normalOutput(QStringLiteral("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
        public_and_private_informations.public_informations.skinId=0;
    }
    const QString &type=QString(GlobalServerData::serverPrivateVariables.db.value(7));
    if(type==Client::text_normal)
        public_and_private_informations.public_informations.type=Player_type_normal;
    else if(type==Client::text_premium)
        public_and_private_informations.public_informations.type=Player_type_premium;
    else if(type==Client::text_gm)
        public_and_private_informations.public_informations.type=Player_type_gm;
    else if(type==Client::text_dev)
        public_and_private_informations.public_informations.type=Player_type_dev;
    else
    {
        normalOutput(QStringLiteral("Mysql wrong type value").arg(type));
        public_and_private_informations.public_informations.type=Player_type_normal;
    }
    public_and_private_informations.cash=QString(GlobalServerData::serverPrivateVariables.db.value(9)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("cash id is not an number, cash set to 0"));
        public_and_private_informations.cash=0;
    }
    public_and_private_informations.warehouse_cash=QString(GlobalServerData::serverPrivateVariables.db.value(18)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("warehouse cash id is not an number, warehouse cash set to 0"));
        public_and_private_informations.warehouse_cash=0;
    }
    market_cash=QString(GlobalServerData::serverPrivateVariables.db.value(21)).toULongLong(&ok);
    if(!ok)
    {
        loginIsWrong(query_id,0x04,QStringLiteral("Market cash wrong: %1").arg(GlobalServerData::serverPrivateVariables.db.value(22)));
        return;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        loginIsWrong(query_id,0x04,"Convert into utf8 have wrong size");
        return;
    }
    Orientation orentation;
    const QString &orientationString=QString(GlobalServerData::serverPrivateVariables.db.value(5));
    if(orientationString==Client::text_top)
        orentation=Orientation_top;
    else if(orientationString==Client::text_bottom)
        orentation=Orientation_bottom;
    else if(orientationString==Client::text_left)
        orentation=Orientation_left;
    else if(orientationString==Client::text_right)
        orentation=Orientation_right;
    else
    {
        orentation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong orientation corrected with bottom"));
    }
    public_and_private_informations.allow=FacilityLib::StringToAllow(QString(GlobalServerData::serverPrivateVariables.db.value(19)));
    //all is rights
    const QString &map=QString(GlobalServerData::serverPrivateVariables.db.value(6));
    const QString &rescue_map=QString(GlobalServerData::serverPrivateVariables.db.value(10));
    const QString &unvalidated_rescue_map=QString(GlobalServerData::serverPrivateVariables.db.value(14));
    if(GlobalServerData::serverPrivateVariables.map_list.contains(map))
    {
        const quint8 &x=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,0x04,QLatin1String("x coord is not a number"));
            return;
        }
        const quint8 &y=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
        if(!ok)
        {
            loginIsWrong(query_id,0x04,QLatin1String("y coord is not a number"));
            return;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list.value(map)->width)
        {
            loginIsWrong(query_id,0x04,QLatin1String("x to out of map"));
            return;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list.value(map)->height)
        {
            loginIsWrong(query_id,0x04,QLatin1String("y to out of map"));
            return;
        }
        loginIsRightWithRescue(query_id,
            characterId,
            GlobalServerData::serverPrivateVariables.map_list.value(map),
            x,
            y,
            (Orientation)orentation,
                rescue_map,
                GlobalServerData::serverPrivateVariables.db.value(11),
                GlobalServerData::serverPrivateVariables.db.value(12),
                GlobalServerData::serverPrivateVariables.db.value(13),
                unvalidated_rescue_map,
                GlobalServerData::serverPrivateVariables.db.value(15),
                GlobalServerData::serverPrivateVariables.db.value(16),
                GlobalServerData::serverPrivateVariables.db.value(17)
        );
    }
    else
        loginIsWrong(query_id,0x04,QLatin1String("Map not found: ")+map);
}

void Client::loginIsRightWithRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(rescue_map.toString()))
    {
        normalOutput(QStringLiteral("rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    bool ok;
    const quint8 &rescue_new_x=rescue_x.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &rescue_new_y=rescue_y.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->width)
    {
        normalOutput(QStringLiteral("rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->height)
    {
        normalOutput(QStringLiteral("rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const QString &orientationString=rescue_orientation.toString();
    Orientation rescue_new_orientation;
    if(orientationString==Client::text_top)
        rescue_new_orientation=Orientation_top;
    else if(orientationString==Client::text_bottom)
        rescue_new_orientation=Orientation_bottom;
    else if(orientationString==Client::text_left)
        rescue_new_orientation=Orientation_left;
    else if(orientationString==Client::text_right)
        rescue_new_orientation=Orientation_right;
    else
    {
        rescue_new_orientation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong rescue orientation corrected with bottom"));
    }
    if(!GlobalServerData::serverPrivateVariables.map_list.contains(unvalidated_rescue_map.toString()))
    {
        normalOutput(QStringLiteral("unvalidated rescue map ,not found"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &unvalidated_rescue_new_x=unvalidated_rescue_x.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("unvalidated rescue x coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint8 &unvalidated_rescue_new_y=unvalidated_rescue_y.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("unvalidated rescue y coord is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->width)
    {
        normalOutput(QStringLiteral("unvalidated rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString())->height)
    {
        normalOutput(QStringLiteral("unvalidated rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const QString &unvalidated_orientationString=unvalidated_rescue_orientation.toString();
    Orientation unvalidated_rescue_new_orientation;
    if(unvalidated_orientationString==Client::text_top)
        unvalidated_rescue_new_orientation=Orientation_top;
    else if(unvalidated_orientationString==Client::text_bottom)
        unvalidated_rescue_new_orientation=Orientation_bottom;
    else if(unvalidated_orientationString==Client::text_left)
        unvalidated_rescue_new_orientation=Orientation_left;
    else if(unvalidated_orientationString==Client::text_right)
        unvalidated_rescue_new_orientation=Orientation_right;
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong unvalidated rescue orientation corrected with bottom"));
    }
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 GlobalServerData::serverPrivateVariables.map_list.value(rescue_map.toString()),rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 GlobalServerData::serverPrivateVariables.map_list.value(unvalidated_rescue_map.toString()),unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void Client::loginIsRight(const quint8 &query_id,quint32 characterId, CommonMap *map, const quint8 &x, const quint8 &y, const Orientation &orientation)
{
    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void Client::loginIsRightWithParsedRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  CommonMap* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                  CommonMap *unvalidated_rescue_map, const quint8 &unvalidated_rescue_x, const quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.db_query_clan.isEmpty())
    {
        errorOutput(QStringLiteral("loginIsRightWithParsedRescue() Query is empty, bug"));
        return;
    }
    #endif
    //load the variables
    character_id=characterId;
    GlobalServerData::serverPrivateVariables.connected_players_id_list << characterId;
    public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
    simplifiedIdList.removeFirst();
    character_loaded=true;
    connectedSince=QDateTime::currentDateTime();
    this->map=map;
    this->x=x;
    this->y=y;
    this->last_direction=static_cast<Direction>(orientation);
    this->rescue.map=rescue_map;
    this->rescue.x=rescue_x;
    this->rescue.y=rescue_y;
    this->rescue.orientation=rescue_orientation;
    this->unvalidated_rescue.map=unvalidated_rescue_map;
    this->unvalidated_rescue.x=unvalidated_rescue_x;
    this->unvalidated_rescue.y=unvalidated_rescue_y;
    this->unvalidated_rescue.orientation=unvalidated_rescue_orientation;
    selectCharacterQueryId << query_id;

    if(public_and_private_informations.clan!=0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.contains(public_and_private_informations.clan))
        {
            GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan]->players << this;
            loginIsRightAfterClan();
            return;
        }
        else
        {
            normalOutput(QStringLiteral("First client of the clan: %1, get the info").arg(public_and_private_informations.clan));
            //do the query
            const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_clan.arg(public_and_private_informations.clan);
            if(!GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::selectClan_static))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                loginIsRightAfterClan();
                return;
            }
        }
    }
    else
    {
        loginIsRightAfterClan();
        return;
    }
}

void Client::loginIsRightAfterClan()
{
    loadLinkedData();
}

void Client::loginIsRightFinalStep()
{
    const quint8 &query_id=selectCharacterQueryId.takeFirst();
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)01;
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (quint8)public_and_private_informations.public_informations.simplifiedId;
    else
        out << (quint16)public_and_private_informations.public_informations.simplifiedId;
    out << public_and_private_informations.public_informations.pseudo;
    out << FacilityLib::allowToString(public_and_private_informations.allow);
    out << (quint32)public_and_private_informations.clan;

    if(public_and_private_informations.clan_leader)
        out << (quint8)0x01;
    else
        out << (quint8)0x00;
    //send the event
    {
        QList<QPair<quint8,quint8> > events;
        int index=0;
        while(index<GlobalServerData::serverPrivateVariables.events.size())
        {
            const quint8 &value=GlobalServerData::serverPrivateVariables.events.at(index);
            if(value!=0)
                events << QPair<quint8,quint8>(index,value);
            index++;
        }
        index=0;
        out << (quint8)events.size();
        while(index<events.size())
        {
            const QPair<quint8,quint8> &event=events.at(index);
            out << (quint8)event.first;
            out << (quint8)event.second;
            index++;
        }
    }
    out << (quint64)public_and_private_informations.cash;
    out << (quint64)public_and_private_informations.warehouse_cash;
    out << (quint32)GlobalServerData::serverPrivateVariables.map_list.size();

    //temporary variable
    quint32 index;
    quint32 size;

    //send recipes
    {
        index=0;
        out << (quint32)public_and_private_informations.recipes.size();
        QSetIterator<quint32> k(public_and_private_informations.recipes);
        while (k.hasNext())
            out << k.next();
    }

    //send monster
    index=0;
    size=public_and_private_informations.playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=FacilityLib::privateMonsterToBinary(public_and_private_informations.playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }
    index=0;
    size=public_and_private_informations.warehouse_playerMonster.size();
    out << (quint32)size;
    while(index<size)
    {
        QByteArray data=FacilityLib::privateMonsterToBinary(public_and_private_informations.warehouse_playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }

    /// \todo force to 255 max
    //send reputation
    {
        out << (quint8)public_and_private_informations.reputation.size();
        QHashIterator<QString,PlayerReputation> i(public_and_private_informations.reputation);
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
        out << (quint8)public_and_private_informations.quests.size();
        QHashIterator<quint32,PlayerQuest> j(public_and_private_informations.quests);
        while (j.hasNext()) {
            j.next();
            out << j.key();
            out << j.value().step;
            out << j.value().finish_one_time;
        }
    }

    //send bot_already_beaten
    {
        out << (quint32)public_and_private_informations.bot_already_beaten.size();
        QSetIterator<quint32> k(public_and_private_informations.bot_already_beaten);
        while (k.hasNext())
            out << k.next();
    }

    postReply(query_id,outputData);
    sendInventory();
    updateCanDoFight();

    //send signals into the server
    normalOutput(QStringLiteral("Logged: %1 on the map: %2 (%3,%4)").arg(public_and_private_informations.public_informations.pseudo).arg(map->map_file).arg(x).arg(y));

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("load the normal player id: %1, simplified id: %2").arg(character_id).arg(public_and_private_informations.public_informations.simplifiedId));
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_player_is_connected(public_and_private_informations);
    #endif
    GlobalServerData::serverPrivateVariables.connected_players++;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        GlobalServerData::serverPrivateVariables.player_updater.addConnectedPlayer();
    playerByPseudo[public_and_private_informations.public_informations.pseudo]=this;
    clientBroadCastList << this;

    put_on_the_map(
                map,//map pointer
        x,
        y,
        static_cast<Orientation>(last_direction)
    );
}

void Client::selectClan_static(void *object)
{
    static_cast<Client *>(object)->selectClan_return();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::selectClan_return()
{
    //parse the result
    if(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        quint64 cash=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toULongLong(&ok);
        if(!ok)
        {
            cash=0;
            normalOutput(QStringLiteral("Warning: clan linked: %1 have wrong cash value, then reseted to 0"));
        }
        haveClanInfo(public_and_private_informations.clan,GlobalServerData::serverPrivateVariables.db.value(0),cash);
    }
    else
    {
        public_and_private_informations.clan=0;
        normalOutput(QStringLiteral("Warning: clan linked: %1 is not found into db"));
    }
    loginIsRightAfterClan();
}

void Client::loginIsWrong(const quint8 &query_id, const quint8 &returnCode, const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)returnCode;
    postReply(query_id,outputData);

    //send to server to stop the connection
    errorOutput(debugMessage);
}
