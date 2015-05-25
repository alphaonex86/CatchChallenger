#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "SqlFunction.h"
#include "PreparedDBQuery.h"
#include "DictionaryLogin.h"
#include "DictionaryServer.h"

using namespace CatchChallenger;

void Client::characterSelectionIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)returnCode;
    postReply(query_id,outputData.constData(),outputData.size());

    //send to server to stop the connection
    errorOutput(debugMessage);
}

void Client::selectCharacter(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_character_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_last_connect.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query db_query_update_character_last_connect is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_time_to_delete.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug"));
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const QString &queryText=PreparedDBQueryCommon::db_query_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::selectCharacter_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        characterSelectionIsWrong(query_id,0x02,queryText+QLatin1String(": ")+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack << selectCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("SelectCharacterParam");
        #endif
        callbackRegistred << callback;
    }
}

void Client::selectCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectCharacter_object();
}

void Client::selectCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacter_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::selectCharacter_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("SelectCharacterParam"))
    {
        qDebug() << "is not SelectCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    /*account(0),pseudo(1),skin(2),type(3),clan(4),cash(5),
    warehouse_cash(6),clan_leader(7),time_to_delete(8),starter(9)*/
    callbackRegistred.removeFirst();
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    {
        qDebug() << QStringLiteral("Try select %1 but not found with account %2").arg(characterId).arg(account_id);
        characterSelectionIsWrong(query_id,0x02,QLatin1String("Result return query wrong"));
        return;
    }

    bool ok;
    public_and_private_informations.clan=QString(GlobalServerData::serverPrivateVariables.db_common->value(4)).toUInt(&ok);
    if(!ok)
    {
        //normalOutput(QStringLiteral("clan id is not an number, clan disabled"));
        public_and_private_informations.clan=0;//no clan
    }
    public_and_private_informations.clan_leader=(QString(GlobalServerData::serverPrivateVariables.db_common->value(7)).toUInt(&ok)==1);
    if(!ok)
    {
        //normalOutput(QStringLiteral("clan_leader id is not an number, clan_leader disabled"));
        public_and_private_informations.clan_leader=false;//no clan
    }

    const quint32 &account_id=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,QStringLiteral("Account for character: %1 is not an id").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
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

    public_and_private_informations.public_informations.pseudo=QString(GlobalServerData::serverPrivateVariables.db_common->value(1));
    if(!loadTheRawUTF8String())
    {
        if(GlobalServerData::serverSettings.anonymous)
            characterSelectionIsWrong(query_id,0x04,QStringLiteral("Unable to convert the pseudo to utf8 for character id: %1").arg(character_id));
        else
            characterSelectionIsWrong(query_id,0x04,QStringLiteral("Unable to convert the pseudo to utf8: %1").arg(public_and_private_informations.public_informations.pseudo));
        return;
    }

    #ifndef SERVERBENCHMARK
    if(GlobalServerData::serverSettings.anonymous)
        normalOutput(QStringLiteral("Charater id is logged: %1").arg(characterId));
    else
        normalOutput(QStringLiteral("Charater is logged: %1").arg(GlobalServerData::serverPrivateVariables.db_common->value(1)));
    #endif
    const quint32 &time_to_delete=QString(GlobalServerData::serverPrivateVariables.db_common->value(8)).toUInt(&ok);
    if(!ok || time_to_delete>0)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_time_to_delete.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));

    const quint32 &skin_database_id=QString(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("Skin is not a number"));
        public_and_private_informations.public_informations.skinId=0;
    }
    else
    {
        if(skin_database_id<(quint32)DictionaryLogin::dictionary_skin_database_to_internal.size())
            public_and_private_informations.public_informations.skinId=DictionaryLogin::dictionary_skin_database_to_internal.value(skin_database_id);
        else
        {
            normalOutput(QStringLiteral("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
            public_and_private_informations.public_informations.skinId=0;
        }
    }
    const quint32 &type=QString(GlobalServerData::serverPrivateVariables.db_common->value(3)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("Player type is not a number, default to normal"));
        public_and_private_informations.public_informations.type=Player_type_normal;
    }
    else
    {
        if(type<=3)
            public_and_private_informations.public_informations.type=static_cast<Player_type>((type+1)*16);
        else
        {
            normalOutput(QStringLiteral("Mysql wrong type value").arg(type));
            public_and_private_informations.public_informations.type=Player_type_normal;
        }
    }
    public_and_private_informations.cash=QString(GlobalServerData::serverPrivateVariables.db_common->value(5)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("cash id is not an number, cash set to 0"));
        public_and_private_informations.cash=0;
    }
    public_and_private_informations.warehouse_cash=QString(GlobalServerData::serverPrivateVariables.db_common->value(6)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(QStringLiteral("warehouse cash id is not an number, warehouse cash set to 0"));
        public_and_private_informations.warehouse_cash=0;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        characterSelectionIsWrong(query_id,0x04,"Convert into utf8 have wrong size");
        return;
    }

    const quint8 &starter=QString(GlobalServerData::serverPrivateVariables.db_common->value(9)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"start from base selection");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        errorOutput(QStringLiteral("profile common and server don't match"));
        return;
    }
    #endif
    if(starter>=DictionaryLogin::dictionary_starter_database_to_internal.size())
    {
        errorOutput(QStringLiteral("starter %1 >= DictionaryLogin::dictionary_starter_database_to_internal.size() %2").arg(starter).arg(DictionaryLogin::dictionary_starter_database_to_internal.size()));
        return;
    }
    profileIndex=DictionaryLogin::dictionary_starter_database_to_internal.at(starter);
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput(QStringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        errorOutput(QStringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return;
    }

    Client::selectCharacterServer(query_id,characterId);
}





























void Client::selectCharacterServer(const quint8 &query_id, const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_character_server_by_id.isEmpty())
    {
        errorOutput(QStringLiteral("selectCharacterServer() Query is empty, bug"));
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const QString &queryText=PreparedDBQueryServer::db_query_character_server_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::selectCharacterServer_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        characterSelectionIsWrong(query_id,0x02,queryText+QLatin1String(": ")+GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack << selectCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << QStringLiteral("SelectCharacterParam");
        #endif
        callbackRegistred << callback;
    }
}

void Client::selectCharacterServer_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectCharacterServer_object();
    GlobalServerData::serverPrivateVariables.db_server->clear();
}

void Client::selectCharacterServer_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.takeFirst());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacterServer_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db_server->clear();
}

void Client::selectCharacterServer_return(const quint8 &query_id,const quint32 &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("SelectCharacterParam"))
    {
        qDebug() << "is not SelectCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    /*x(0),y(1),orientation(2),map(3),
    rescue_map(4),rescue_x(5),rescue_y(6),rescue_orientation(7),
    unvalidated_rescue_map(8),unvalidated_rescue_x(9),unvalidated_rescue_y(10),unvalidated_rescue_orientation(11),
    market_cash(12)*/
    callbackRegistred.removeFirst();
    if(!GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);
        dbQueryWriteServer(serverProfileInternal.preparedQuerySelect.at(0)+
                     QString::number(characterId)+
                     serverProfileInternal.preparedQuerySelect.at(1)+
                     QString::number(QDateTime::currentDateTime().toTime_t())+
                     serverProfileInternal.preparedQuerySelect.at(2)
                     );
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));
        loginIsRightWithParsedRescue(query_id,
            characterId,
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //rescue
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //last unvalidated
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation
        );
        return;
    }

    bool ok;
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(simplifiedIdList.size()<=0)
        {
            characterSelectionIsWrong(query_id,0x04,QLatin1String("Not free id to login"));
            return;
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }

    market_cash=QString(GlobalServerData::serverPrivateVariables.db_server->value(12)).toULongLong(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QStringLiteral("Market cash wrong: %1").arg(GlobalServerData::serverPrivateVariables.db_server->value(22)));
        return;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        characterSelectionIsWrong(query_id,0x04,"Convert into utf8 have wrong size");
        return;
    }
    Orientation orientation;
    const quint32 &orientationInt=QString(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            orientation=static_cast<Orientation>(orientationInt);
        else
        {
            orientation=Orientation_bottom;
            normalOutput(QStringLiteral("Wrong orientation corrected with bottom"));
        }
    }
    else
    {
        orientation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong orientation (not number) corrected with bottom: %1").arg(QString(GlobalServerData::serverPrivateVariables.db_server->value(5))));
    }
    //all is rights
    const quint32 &map_database_id=QString(GlobalServerData::serverPrivateVariables.db_server->value(3)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("map_database_id is not a number"));
        return;
    }
    if(map_database_id>=(quint32)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("map_database_id out of range"));
        return;
    }
    CommonMap * const map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
    if(map==NULL)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("map_database_id have not reverse"));
        return;
    }
    const quint8 &x=QString(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("x coord is not a number"));
        return;
    }
    const quint8 &y=QString(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("y coord is not a number"));
        return;
    }
    if(x>=map->width)
    {
        characterSelectionIsWrong(query_id,0x04,QStringLiteral("x to out of map: %1 > %2 (%3)").arg(x).arg(map->width).arg(map->map_file));
        return;
    }
    if(y>=map->height)
    {
        characterSelectionIsWrong(query_id,0x04,QStringLiteral("y to out of map: %1 > %2 (%3)").arg(y).arg(map->height).arg(map->map_file));
        return;
    }
    loginIsRightWithRescue(query_id,
        characterId,
        map,
        x,
        y,
        orientation,
            GlobalServerData::serverPrivateVariables.db_server->value(4),
            GlobalServerData::serverPrivateVariables.db_server->value(5),
            GlobalServerData::serverPrivateVariables.db_server->value(6),
            GlobalServerData::serverPrivateVariables.db_server->value(7),
            GlobalServerData::serverPrivateVariables.db_server->value(8),
            GlobalServerData::serverPrivateVariables.db_server->value(9),
            GlobalServerData::serverPrivateVariables.db_server->value(10),
            GlobalServerData::serverPrivateVariables.db_server->value(11)
    );
}



































void Client::loginIsRightWithRescue(const quint8 &query_id, quint32 characterId, CommonMap* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    bool ok;
    const quint32 &rescue_map_database_id=rescue_map.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QLatin1String("rescue_map_database_id is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_map_database_id>=(quint32)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput(QLatin1String("rescue_map_database_id out of range"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id)==NULL)
    {
        normalOutput(QLatin1String("rescue_map_database_id have not reverse"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id);
    if(rescue_map_final==NULL)
    {
        normalOutput(QStringLiteral("rescue map not resolved"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
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
    if(rescue_new_x>=rescue_map_final->width)
    {
        normalOutput(QStringLiteral("rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=rescue_map_final->height)
    {
        normalOutput(QStringLiteral("rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const quint32 &orientationInt=rescue_orientation.toUInt(&ok);
    Orientation rescue_new_orientation;
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            rescue_new_orientation=static_cast<Orientation>(orientationInt);
        else
        {
            rescue_new_orientation=Orientation_bottom;
            normalOutput(QStringLiteral("Wrong rescue orientation corrected with bottom"));
        }
    }
    else
    {
        rescue_new_orientation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong rescue orientation (not number) corrected with bottom"));
    }





    const quint32 &unvalidated_rescue_map_database_id=unvalidated_rescue_map.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id is not a number"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_map_database_id>=(quint32)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id out of range"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id)==NULL)
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id have not reverse"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *unvalidated_rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id);
    if(unvalidated_rescue_map_final==NULL)
    {
        normalOutput(QStringLiteral("unvalidated rescue map not resolved"));
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
    if(unvalidated_rescue_new_x>=unvalidated_rescue_map_final->width)
    {
        normalOutput(QStringLiteral("unvalidated rescue x to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=unvalidated_rescue_map_final->height)
    {
        normalOutput(QStringLiteral("unvalidated rescue y to out of map"));
        loginIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    Orientation unvalidated_rescue_new_orientation;
    const quint32 &unvalidated_orientationInt=unvalidated_rescue_orientation.toUInt(&ok);
    if(ok)
    {
        if(unvalidated_orientationInt>=1 && unvalidated_orientationInt<=4)
            unvalidated_rescue_new_orientation=static_cast<Orientation>(unvalidated_orientationInt);
        else
        {
            unvalidated_rescue_new_orientation=Orientation_bottom;
            normalOutput(QStringLiteral("Wrong unvalidated orientation corrected with bottom"));
        }
    }
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        normalOutput(QStringLiteral("Wrong unvalidated orientation (not number) corrected with bottom"));
    }





    loginIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 rescue_map_final,rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 unvalidated_rescue_map_final,unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
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
    if(PreparedDBQueryCommon::db_query_clan.isEmpty())
    {
        errorOutput(QStringLiteral("loginIsRightWithParsedRescue() Query is empty, bug"));
        return;
    }
    #endif
    //load the variables
    character_id=characterId;
    character_loaded_in_progress=true;
    GlobalServerData::serverPrivateVariables.connected_players_id_list << characterId;
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.takeFirst();
        break;
        case MapVisibilityAlgorithmSelection_None:
        public_and_private_informations.public_informations.simplifiedId = 0;
        break;
    }
    connectedSince=QDateTime::currentDateTime();
    this->map=map;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif
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

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(public_and_private_informations.clan!=0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.contains(public_and_private_informations.clan))
        {
            GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan]->players << this;
            loadLinkedData();
            return;
        }
        else
        {
            normalOutput(QStringLiteral("First client of the clan: %1, get the info").arg(public_and_private_informations.clan));
            //do the query
            const QString &queryText=PreparedDBQueryCommon::db_query_clan.arg(public_and_private_informations.clan);
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::selectClan_static);
            if(callback==NULL)
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
                loadLinkedData();
                return;
            }
            else
                callbackRegistred << callback;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            QString mapToDebug=this->map->map_file;
            mapToDebug+=this->map->map_file;
        }
        #endif
        loadLinkedData();
        return;
    }
}

void Client::loadItemOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_itemOnMap.isEmpty())
    {
        errorOutput(QStringLiteral("loadBotAlreadyBeaten() Query is empty, bug"));
        return;
    }
    #endif
    const QString &queryText=PreparedDBQueryServer::db_query_select_itemOnMap.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::loadItemOnMap_static);
    if(callback==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        loginIsRightFinalStep();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadItemOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItemOnMap_return();
}

void Client::loadItemOnMap_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const quint16 &itemDbCode=QString(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(QStringLiteral("wrong value type for item on map, skip: %1").arg(itemDbCode));
            continue;
        }
        if(itemDbCode>=DictionaryServer::dictionary_itemOnMap_database_to_internal.size())
        {
            normalOutput(QStringLiteral("item on map is not into the map list (1), skip: %1").arg(itemDbCode));
            continue;
        }
        if(DictionaryServer::dictionary_itemOnMap_database_to_internal[itemDbCode]==255/*-1*/)
        {
            normalOutput(QStringLiteral("item on map is not into the map list (2), skip: %1").arg(itemDbCode));
            continue;
        }
        public_and_private_informations.itemOnMap << DictionaryServer::dictionary_itemOnMap_database_to_internal[itemDbCode];
    }
    loginIsRightFinalStep();
}

void Client::loginIsRightFinalStep()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    character_loaded_in_progress=false;
    character_loaded=true;

    const quint8 &query_id=selectCharacterQueryId.takeFirst();
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)01;// all is good

    if(GlobalServerData::serverSettings.sendPlayerNumber)
        out << (quint16)GlobalServerData::serverSettings.max_players;
    else
    {
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (quint16)255;
        else
            out << (quint16)65535;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (quint32)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (quint32)time/1000;
    }
    else
        out << (quint32)0x00000000;
    #else
    out << (quint32)0x00000000;
    #endif
    out << (quint8)GlobalServerData::serverSettings.city.capture.frenquency;

    //common settings
    out << (quint32)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    out << (quint8)CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
    out << (quint8)CommonSettingsServer::commonSettingsServer.forcedSpeed;
    out << (quint8)CommonSettingsServer::commonSettingsServer.useSP;
    out << (quint8)CommonSettingsServer::commonSettingsServer.tcpCork;
    out << (quint8)CommonSettingsServer::commonSettingsServer.autoLearn;
    out << (quint8)CommonSettingsServer::commonSettingsServer.dontSendPseudo;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_gold;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_drop;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_all;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_local;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_private;
    out << (quint8)CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    out << (quint8)CommonSettingsServer::commonSettingsServer.factoryPriceChange;

    //main hash
    outputData+=CommonSettingsServer::commonSettingsServer.datapackHashServerMain;
    out.device()->seek(out.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size());
    //sub hash
    outputData+=CommonSettingsServer::commonSettingsServer.datapackHashServerSub;
    out.device()->seek(out.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size());
    //mirror
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }
    //Main type code
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(CommonSettingsServer::commonSettingsServer.mainDatapackCode);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }
    //Sub type cod
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(CommonSettingsServer::commonSettingsServer.subDatapackCode);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }

    //temporary character id
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (quint8)public_and_private_informations.public_informations.simplifiedId;
    else
        out << (quint16)public_and_private_informations.public_informations.simplifiedId;
    //pseudo
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(public_and_private_informations.public_informations.pseudo);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }
    out << (quint8)public_and_private_informations.allow.size();
    {
        QSetIterator<ActionAllow> i(public_and_private_informations.allow);
        while (i.hasNext())
            out << (quint8)i.next();
    }

    //clan related
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
    out << (quint8)public_and_private_informations.itemOnMap.size();
    {
        int index=0;
        while(index<public_and_private_informations.itemOnMap.size())
        {
            out << (quint8)public_and_private_informations.itemOnMap.at(index);
            index++;
        }
    }

    //temporary variable
    quint32 index;
    quint32 size;

    //send recipes
    {
        index=0;
        out << (quint16)public_and_private_informations.recipes.size();
        QSetIterator<quint16> k(public_and_private_informations.recipes);
        while (k.hasNext())
            out << k.next();
    }

    //send monster
    index=0;
    size=public_and_private_informations.playerMonster.size();
    out << (quint8)size;
    while(index<size)
    {
        const QByteArray &data=FacilityLib::privateMonsterToBinary(public_and_private_informations.playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }
    index=0;
    size=public_and_private_informations.warehouse_playerMonster.size();
    out << (quint8)size;
    while(index<size)
    {
        const QByteArray &data=FacilityLib::privateMonsterToBinary(public_and_private_informations.warehouse_playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }

    /// \todo force to 255 max
    //send reputation
    {
        out << (quint8)public_and_private_informations.reputation.size();
        QMapIterator<quint8,PlayerReputation> i(public_and_private_informations.reputation);
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
        out << (quint16)public_and_private_informations.quests.size();
        QHashIterator<quint16,PlayerQuest> j(public_and_private_informations.quests);
        while (j.hasNext()) {
            j.next();
            out << j.key();
            out << j.value().step;
            out << j.value().finish_one_time;
        }
    }

    //send bot_already_beaten
    {
        out << (quint16)public_and_private_informations.bot_already_beaten.size();
        QSetIterator<quint16> k(public_and_private_informations.bot_already_beaten);
        while (k.hasNext())
            out << k.next();
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    postReply(query_id,outputData.constData(),outputData.size());
    sendInventory();
    updateCanDoFight();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    //send signals into the server
    #ifndef SERVERBENCHMARK
    normalOutput(QStringLiteral("Logged: %1 on the map: %2 (%3,%4)")
                 .arg(public_and_private_informations.public_informations.pseudo)
                 .arg(map->map_file)
                 .arg(x)
                 .arg(y)
                 );
    #endif

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

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    put_on_the_map(
                map,//map pointer
        x,
        y,
        static_cast<Orientation>(last_direction)
    );

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        QString mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif
}

void Client::selectClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectClan_return();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::selectClan_return()
{
    callbackRegistred.removeFirst();
    //parse the result
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        quint64 cash=QString(GlobalServerData::serverPrivateVariables.db_common->value(1)).toULongLong(&ok);
        if(!ok)
        {
            cash=0;
            normalOutput(QStringLiteral("Warning: clan linked: %1 have wrong cash value, then reseted to 0"));
        }
        haveClanInfo(public_and_private_informations.clan,GlobalServerData::serverPrivateVariables.db_common->value(0),cash);
    }
    else
    {
        public_and_private_informations.clan=0;
        normalOutput(QStringLiteral("Warning: clan linked: %1 is not found into db"));
    }
    loadLinkedData();
}

void Client::loginIsWrong(const quint8 &query_id, const quint8 &returnCode, const QString &debugMessage)
{
    //network send
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    *(Client::loginIsWrongBuffer+1)=query_id;
    *(Client::loginIsWrongBuffer+3)=returnCode;
    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));

    //send to server to stop the connection
    errorOutput(debugMessage);
}

void Client::loadPlayerAllow()
{
    if(!PreparedDBQueryCommon::db_query_select_allow.isEmpty())
    {
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(PreparedDBQueryCommon::db_query_select_allow.arg(character_id).toLatin1(),this,&Client::loadPlayerAllow_static);
        if(callback==NULL)
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(PreparedDBQueryCommon::db_query_select_allow).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
            loadItems();
            return;
        }
        else
        {
            callbackRegistred << callback;
        }
    }
}

void Client::loadPlayerAllow_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerAllow_return();
}

void Client::loadPlayerAllow_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const quint32 &allowCode=QString(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            if(allowCode<(quint32)DictionaryLogin::dictionary_allow_database_to_internal.size())
            {
                const ActionAllow &allow=DictionaryLogin::dictionary_allow_database_to_internal.at(allowCode);
                if(allow!=ActionAllow_Nothing)
                    public_and_private_informations.allow << allow;
                else
                {
                    ok=false;
                    normalOutput(QStringLiteral("allow id: %1 is not reverse").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
                }
            }
            else
            {
                ok=false;
                normalOutput(QStringLiteral("allow id: %1 out of reverse list").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
            }
        }
        else
            normalOutput(QStringLiteral("allow id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
    }
    loadItems();
}
