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
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.h"
#endif

using namespace CatchChallenger;

void Client::characterSelectionIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage)
{
    //network send
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)returnCode;
    postReply(query_id,outputData.constData(),outputData.size());

    //send to server to stop the connection
    errorOutput(debugMessage);
}

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::selectCharacter(const uint8_t &query_id, const char * const token)
{
    const quint64 &time=QDateTime::currentDateTime().toMSecsSinceEpoch()/1000;
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVERMAXTIME))
        {
            if(memcmp(tokenAuth.token,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)==0)
            {
                delete tokenAuth.token;
                account_id=tokenAuth.accountIdRequester;/// \warning need take care of only write if character is selected
                selectCharacter(query_id,tokenAuth.characterId);
                tokenAuthList.erase(tokenAuthList.begin()+index);
                return;
            }
            index++;
        }
        else
        {
            LinkToMaster::linkToMaster->characterDisconnected(tokenAuth.characterId);
            tokenAuthList.erase(tokenAuthList.begin()+index);
        }
    }
    //if never found
    errorOutput(std::stringLiteral("selectCharacter() Token never found to login, bug"));
}

void Client::purgeTokenAuthList()
{
    const quint64 &time=QDateTime::currentDateTime().toMSecsSinceEpoch()/1000;
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVERMAXTIME))
            index++;
        else
        {
            LinkToMaster::linkToMaster->characterDisconnected(tokenAuth.characterId);
            tokenAuthList.erase(tokenAuthList.begin()+index);
        }
    }
}
#endif

void Client::selectCharacter(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_character_by_id.isEmpty())
    {
        errorOutput(std::stringLiteral("selectCharacter() Query is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_last_connect.isEmpty())
    {
        errorOutput(std::stringLiteral("selectCharacter() Query db_query_update_character_last_connect is empty, bug"));
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_time_to_delete.isEmpty())
    {
        errorOutput(std::stringLiteral("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug"));
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const std::string &queryText=PreparedDBQueryCommon::db_query_character_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::selectCharacter_static);
    if(callback==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        characterSelectionIsWrong(query_id,0x02,queryText+QLatin1String(": ")+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack << selectCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << std::stringLiteral("SelectCharacterParam");
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

void Client::selectCharacter_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=std::stringLiteral("SelectCharacterParam"))
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
        qDebug() << std::stringLiteral("Try select %1 but not found with account %2").arg(characterId).arg(account_id);
        characterSelectionIsWrong(query_id,0x02,QLatin1String("Result return query wrong"));
        return;
    }

    bool ok;
    public_and_private_informations.clan=std::string(GlobalServerData::serverPrivateVariables.db_common->value(4)).toUInt(&ok);
    if(!ok)
    {
        //normalOutput(std::stringLiteral("clan id is not an number, clan disabled"));
        public_and_private_informations.clan=0;//no clan
    }
    public_and_private_informations.clan_leader=(std::string(GlobalServerData::serverPrivateVariables.db_common->value(7)).toUInt(&ok)==1);
    if(!ok)
    {
        //normalOutput(std::stringLiteral("clan_leader id is not an number, clan_leader disabled"));
        public_and_private_informations.clan_leader=false;//no clan
    }

    if(character_loaded)
    {
        characterSelectionIsWrong(query_id,0x03,QLatin1String("character_loaded already to true"));
        return;
    }
    const uint32_t &account_id=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,std::stringLiteral("Account for character: %1 is not an id").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,std::stringLiteral("Character: %1 is not owned by the account: %2").arg(characterId).arg(this->account_id));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.contains(characterId))
    {
        characterSelectionIsWrong(query_id,0x03,QLatin1String("Already logged"));
        return;
    }

    public_and_private_informations.public_informations.pseudo=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1));
    if(!loadTheRawUTF8String())
    {
        if(GlobalServerData::serverSettings.anonymous)
            characterSelectionIsWrong(query_id,0x04,std::stringLiteral("Unable to convert the pseudo to utf8 for character id: %1").arg(character_id));
        else
            characterSelectionIsWrong(query_id,0x04,std::stringLiteral("Unable to convert the pseudo to utf8: %1").arg(public_and_private_informations.public_informations.pseudo));
        return;
    }

    #ifndef SERVERBENCHMARK
    if(GlobalServerData::serverSettings.anonymous)
        normalOutput(std::stringLiteral("Charater id is logged: %1").arg(characterId));
    else
        normalOutput(std::stringLiteral("Charater is logged: %1").arg(GlobalServerData::serverPrivateVariables.db_common->value(1)));
    #endif
    const uint32_t &time_to_delete=std::string(GlobalServerData::serverPrivateVariables.db_common->value(8)).toUInt(&ok);
    if(!ok || time_to_delete>0)
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_time_to_delete.arg(characterId));
    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));

    const uint32_t &skin_database_id=std::string(GlobalServerData::serverPrivateVariables.db_common->value(2)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("Skin is not a number"));
        public_and_private_informations.public_informations.skinId=0;
    }
    else
    {
        if(skin_database_id<(uint32_t)DictionaryLogin::dictionary_skin_database_to_internal.size())
            public_and_private_informations.public_informations.skinId=DictionaryLogin::dictionary_skin_database_to_internal.value(skin_database_id);
        else
        {
            normalOutput(std::stringLiteral("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have"));
            public_and_private_informations.public_informations.skinId=0;
        }
    }
    const uint32_t &type=std::string(GlobalServerData::serverPrivateVariables.db_common->value(3)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("Player type is not a number, default to normal"));
        public_and_private_informations.public_informations.type=Player_type_normal;
    }
    else
    {
        if(type<=3)
            public_and_private_informations.public_informations.type=static_cast<Player_type>((type+1)*16);
        else
        {
            normalOutput(std::stringLiteral("Mysql wrong type value").arg(type));
            public_and_private_informations.public_informations.type=Player_type_normal;
        }
    }
    public_and_private_informations.cash=std::string(GlobalServerData::serverPrivateVariables.db_common->value(5)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("cash id is not an number, cash set to 0"));
        public_and_private_informations.cash=0;
    }
    public_and_private_informations.warehouse_cash=std::string(GlobalServerData::serverPrivateVariables.db_common->value(6)).toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("warehouse cash id is not an number, warehouse cash set to 0"));
        public_and_private_informations.warehouse_cash=0;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        characterSelectionIsWrong(query_id,0x04,"Convert into utf8 have wrong size");
        return;
    }

    const uint8_t &starter=std::string(GlobalServerData::serverPrivateVariables.db_common->value(9)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"start from base selection");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("profile common and server don't match"));
        return;
    }
    #endif
    if(starter>=DictionaryLogin::dictionary_starter_database_to_internal.size())
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("starter %1 >= DictionaryLogin::dictionary_starter_database_to_internal.size() %2").arg(starter).arg(DictionaryLogin::dictionary_starter_database_to_internal.size()));
        return;
    }
    profileIndex=DictionaryLogin::dictionary_starter_database_to_internal.at(starter);
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput(std::stringLiteral("profile index: %1 out of range (profileList size: %2)").arg(profileIndex).arg(CommonDatapack::commonDatapack.profileList.size()));
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(characterId);
        #endif
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("profile index: %1 profil not valid").arg(profileIndex));
        return;
    }

    Client::selectCharacterServer(query_id,characterId);
}





























void Client::selectCharacterServer(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_character_server_by_id.isEmpty())
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("selectCharacterServer() Query is empty, bug"));
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const std::string &queryText=PreparedDBQueryServer::db_query_character_server_by_id.arg(characterId);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::selectCharacterServer_static);
    if(callback==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        characterSelectionIsWrong(query_id,0x02,queryText+QLatin1String(": ")+GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack << selectCharacterParam;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType << std::stringLiteral("SelectCharacterParam");
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

void Client::selectCharacterServer_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=std::stringLiteral("SelectCharacterParam"))
    {
        qDebug() << "is not SelectCharacterParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    /*map(0),x(1),y(2),orientation(3),
    rescue_map(4),rescue_x(5),rescue_y(6),rescue_orientation(7),
    unvalidated_rescue_map(8),unvalidated_rescue_x(9),unvalidated_rescue_y(10),unvalidated_rescue_orientation(11),
    market_cash(12)*/
    callbackRegistred.removeFirst();
    if(!GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);
        dbQueryWriteServer(serverProfileInternal.preparedQuerySelect.at(0)+
                     std::string::number(characterId)+
                     serverProfileInternal.preparedQuerySelect.at(1)+
                     std::string::number(QDateTime::currentDateTime().toTime_t())+
                     serverProfileInternal.preparedQuerySelect.at(2)
                     );
        dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_character_last_connect.arg(characterId).arg(QDateTime::currentDateTime().toTime_t()));
        characterIsRightWithParsedRescue(query_id,
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

    market_cash=std::string(GlobalServerData::serverPrivateVariables.db_server->value(12)).toULongLong(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("Market cash wrong: %1").arg(GlobalServerData::serverPrivateVariables.db_server->value(22)));
        return;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    if(!loadTheRawUTF8String())
    {
        characterSelectionIsWrong(query_id,0x04,"Convert into utf8 have wrong size");
        return;
    }
    Orientation orientation;
    const uint32_t &orientationInt=std::string(GlobalServerData::serverPrivateVariables.db_server->value(3)).toUInt(&ok);
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            orientation=static_cast<Orientation>(orientationInt);
        else
        {
            orientation=Orientation_bottom;
            normalOutput(std::stringLiteral("Wrong orientation corrected with bottom"));
        }
    }
    else
    {
        orientation=Orientation_bottom;
        normalOutput(std::stringLiteral("Wrong orientation (not number) corrected with bottom: %1").arg(std::string(GlobalServerData::serverPrivateVariables.db_server->value(5))));
    }
    //all is rights
    const uint32_t &map_database_id=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("map_database_id is not a number"));
        return;
    }
    if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
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
    const uint8_t &x=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("x coord is not a number"));
        return;
    }
    const uint8_t &y=std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,QLatin1String("y coord is not a number"));
        return;
    }
    if(x>=map->width)
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("x to out of map: %1 > %2 (%3)").arg(x).arg(map->width).arg(map->map_file));
        return;
    }
    if(y>=map->height)
    {
        characterSelectionIsWrong(query_id,0x04,std::stringLiteral("y to out of map: %1 > %2 (%3)").arg(y).arg(map->height).arg(map->map_file));
        return;
    }
    characterIsRightWithRescue(query_id,
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



































void Client::characterIsRightWithRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                  const QVariant &rescue_map, const QVariant &rescue_x, const QVariant &rescue_y, const QVariant &rescue_orientation,
                  const QVariant &unvalidated_rescue_map, const QVariant &unvalidated_rescue_x, const QVariant &unvalidated_rescue_y, const QVariant &unvalidated_rescue_orientation
                                             )
{
    bool ok;
    const uint32_t &rescue_map_database_id=rescue_map.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QLatin1String("rescue_map_database_id is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput(QLatin1String("rescue_map_database_id out of range"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id)==NULL)
    {
        normalOutput(QLatin1String("rescue_map_database_id have not reverse"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id);
    if(rescue_map_final==NULL)
    {
        normalOutput(std::stringLiteral("rescue map not resolved"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_x=rescue_x.toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("rescue x coord is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_y=rescue_y.toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("rescue y coord is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=rescue_map_final->width)
    {
        normalOutput(std::stringLiteral("rescue x to out of map"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=rescue_map_final->height)
    {
        normalOutput(std::stringLiteral("rescue y to out of map"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint32_t &orientationInt=rescue_orientation.toUInt(&ok);
    Orientation rescue_new_orientation;
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            rescue_new_orientation=static_cast<Orientation>(orientationInt);
        else
        {
            rescue_new_orientation=Orientation_bottom;
            normalOutput(std::stringLiteral("Wrong rescue orientation corrected with bottom"));
        }
    }
    else
    {
        rescue_new_orientation=Orientation_bottom;
        normalOutput(std::stringLiteral("Wrong rescue orientation (not number) corrected with bottom"));
    }





    const uint32_t &unvalidated_rescue_map_database_id=unvalidated_rescue_map.toUInt(&ok);
    if(!ok)
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id out of range"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id)==NULL)
    {
        normalOutput(QLatin1String("unvalidated_rescue_map_database_id have not reverse"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *unvalidated_rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id);
    if(unvalidated_rescue_map_final==NULL)
    {
        normalOutput(std::stringLiteral("unvalidated rescue map not resolved"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_x=unvalidated_rescue_x.toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("unvalidated rescue x coord is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_y=unvalidated_rescue_y.toUInt(&ok);
    if(!ok)
    {
        normalOutput(std::stringLiteral("unvalidated rescue y coord is not a number"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=unvalidated_rescue_map_final->width)
    {
        normalOutput(std::stringLiteral("unvalidated rescue x to out of map"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=unvalidated_rescue_map_final->height)
    {
        normalOutput(std::stringLiteral("unvalidated rescue y to out of map"));
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    Orientation unvalidated_rescue_new_orientation;
    const uint32_t &unvalidated_orientationInt=unvalidated_rescue_orientation.toUInt(&ok);
    if(ok)
    {
        if(unvalidated_orientationInt>=1 && unvalidated_orientationInt<=4)
            unvalidated_rescue_new_orientation=static_cast<Orientation>(unvalidated_orientationInt);
        else
        {
            unvalidated_rescue_new_orientation=Orientation_bottom;
            normalOutput(std::stringLiteral("Wrong unvalidated orientation corrected with bottom"));
        }
    }
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        normalOutput(std::stringLiteral("Wrong unvalidated orientation (not number) corrected with bottom"));
    }





    characterIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 rescue_map_final,rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 unvalidated_rescue_map_final,unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void Client::characterIsRight(const uint8_t &query_id,uint32_t characterId, CommonMap *map, const uint8_t &x, const uint8_t &y, const Orientation &orientation)
{
    characterIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void Client::characterIsRightWithParsedRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                  CommonMap* rescue_map, const /*COORD_TYPE*/ uint8_t &rescue_x, const /*COORD_TYPE*/ uint8_t &rescue_y, const Orientation &rescue_orientation,
                  CommonMap *unvalidated_rescue_map, const uint8_t &unvalidated_rescue_x, const uint8_t &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_clan.isEmpty())
    {
        errorOutput(std::stringLiteral("loginIsRightWithParsedRescue() Query is empty, bug"));
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
        std::string mapToDebug=this->map->map_file;
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
        std::string mapToDebug=this->map->map_file;
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
            normalOutput(std::stringLiteral("First client of the clan: %1, get the info").arg(public_and_private_informations.clan));
            //do the query
            const std::string &queryText=PreparedDBQueryCommon::db_query_clan.arg(public_and_private_informations.clan);
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText.toLatin1(),this,&Client::selectClan_static);
            if(callback==NULL)
            {
                qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
            std::string mapToDebug=this->map->map_file;
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
        errorOutput(std::stringLiteral("loadBotAlreadyBeaten() Query is empty, bug"));
        return;
    }
    #endif
    const std::string &queryText=PreparedDBQueryServer::db_query_select_itemOnMap.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::loadItemOnMap_static);
    if(callback==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        loadPlantOnMap();
        #else
        characterIsRightFinalStep();
        #endif
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
        const uint16_t &pointOnMapDatabaseId=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(std::stringLiteral("wrong value type for item on map, skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput(std::stringLiteral("item on map is not into the map list (1), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.value(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput(std::stringLiteral("item on map is not into the map list (2), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        const uint8_t &indexOfItemOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.value(pointOnMapDatabaseId).indexOfItemOnMap;
        if(indexOfItemOnMap==255)
        {
            normalOutput(std::stringLiteral("item on map is not into the map list (3), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfItemOnMap>=Client::indexOfItemOnMap)
        {
            normalOutput(std::stringLiteral("item on map is not into the map list (4), skip: %1 on %2").arg(indexOfItemOnMap).arg(Client::indexOfItemOnMap));
            continue;
        }
        public_and_private_informations.itemOnMap << indexOfItemOnMap;
    }
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    loadPlantOnMap();
    #else
    characterIsRightFinalStep();
    #endif
}


#ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void Client::loadPlantOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_plant.isEmpty())
    {
        errorOutput(std::stringLiteral("loadBotAlreadyBeaten() Query is empty, bug"));
        return;
    }
    #endif
    const std::string &queryText=PreparedDBQueryServer::db_query_select_plant.arg(character_id);
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText.toLatin1(),this,&Client::loadPlantOnMap_static);
    if(callback==NULL)
    {
        qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        characterIsRightFinalStep();
        return;
    }
    else
        callbackRegistred << callback;
}

void Client::loadPlantOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlantOnMap_return();
}

void Client::loadPlantOnMap_return()
{
    callbackRegistred.removeFirst();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &pointOnMapDatabaseId=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(std::stringLiteral("wrong value type for dirt on map, skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput(std::stringLiteral("dirt on map is not into the map list (1), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.value(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput(std::stringLiteral("dirt on map is not into the map list (2), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        const uint8_t &indexOfDirtOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.value(pointOnMapDatabaseId).indexOfDirtOnMap;
        if(indexOfDirtOnMap==255)
        {
            normalOutput(std::stringLiteral("dirt on map is not into the map list (3), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfDirtOnMap>=Client::indexOfDirtOnMap)
        {
            normalOutput(std::stringLiteral("dirt on map is not into the map list (4), skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }

        const uint32_t &plant=std::string(GlobalServerData::serverPrivateVariables.db_server->value(1)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(std::stringLiteral("wrong value type for dirt plant id on map, skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        if(!CommonDatapack::commonDatapack.plants.contains(plant))
        {
            normalOutput(std::stringLiteral("wrong value type for plant dirt on map, skip: %1").arg(plant));
            continue;
        }
        const uint32_t &plant_timestamps=std::string(GlobalServerData::serverPrivateVariables.db_server->value(2)).toUInt(&ok);
        if(!ok)
        {
            normalOutput(std::stringLiteral("wrong value type for plant timestamps on map, skip: %1").arg(pointOnMapDatabaseId));
            continue;
        }
        {
            PlayerPlant playerPlant;
            playerPlant.plant=plant;
            playerPlant.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds;
            public_and_private_informations.plantOnMap[indexOfDirtOnMap]=playerPlant;
        }
    }
    characterIsRightFinalStep();
}
#endif



void Client::characterIsRightFinalStep()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    character_loaded_in_progress=false;
    character_loaded=true;

    const uint8_t &query_id=selectCharacterQueryId.takeFirst();
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)01;// all is good

    if(GlobalServerData::serverSettings.sendPlayerNumber)
        out << (uint16_t)GlobalServerData::serverSettings.max_players;
    else
    {
        if(GlobalServerData::serverSettings.max_players<=255)
            out << (uint16_t)255;
        else
            out << (uint16_t)65535;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
        out << (uint32_t)0x00000000;
    else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
    {
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        out << (uint32_t)time/1000;
    }
    else
        out << (uint32_t)0x00000000;
    #else
    out << (uint32_t)0x00000000;
    #endif
    out << (uint8_t)GlobalServerData::serverSettings.city.capture.frenquency;

    //common settings
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer;
    out << (uint32_t)CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.forcedSpeed;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.useSP;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.tcpCork;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.autoLearn;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.dontSendPseudo;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_gold;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    out << (float)CommonSettingsServer::commonSettingsServer.rates_drop;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.chat_allow_all;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.chat_allow_local;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.chat_allow_private;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    out << (uint8_t)CommonSettingsServer::commonSettingsServer.factoryPriceChange;

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
    if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=28)
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=28";
        abort();
    }
    //main hash
    outputData+=CommonSettingsServer::commonSettingsServer.datapackHashServerMain;
    out.device()->seek(out.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size());
    //sub hash
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
    {
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=28)
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=28";
            abort();
        }
        outputData+=CommonSettingsServer::commonSettingsServer.datapackHashServerSub;
        out.device()->seek(out.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size());
    }
    //mirror
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }


    //temporary character id
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (uint8_t)public_and_private_informations.public_informations.simplifiedId;
    else
        out << (uint16_t)public_and_private_informations.public_informations.simplifiedId;
    //pseudo
    {
        const QByteArray &httpDatapackMirrorRaw=FacilityLibGeneral::toUTF8WithHeader(public_and_private_informations.public_informations.pseudo);
        outputData+=httpDatapackMirrorRaw;
        out.device()->seek(out.device()->pos()+httpDatapackMirrorRaw.size());
    }
    out << (uint8_t)public_and_private_informations.allow.size();
    {
        Std::unordered_SetIterator<ActionAllow> i(public_and_private_informations.allow);
        while (i.hasNext())
            out << (uint8_t)i.next();
    }

    //clan related
    out << (uint32_t)public_and_private_informations.clan;
    if(public_and_private_informations.clan_leader)
        out << (uint8_t)0x01;
    else
        out << (uint8_t)0x00;

    //send the event
    {
        std::vector<std::pair<uint8_t,uint8_t> > events;
        int index=0;
        while(index<GlobalServerData::serverPrivateVariables.events.size())
        {
            const uint8_t &value=GlobalServerData::serverPrivateVariables.events.at(index);
            if(value!=0)
                events << std::pair<uint8_t,uint8_t>(index,value);
            index++;
        }
        index=0;
        out << (uint8_t)events.size();
        while(index<events.size())
        {
            const std::pair<uint8_t,uint8_t> &event=events.at(index);
            out << (uint8_t)event.first;
            out << (uint8_t)event.second;
            index++;
        }
    }

    out << (quint64)public_and_private_informations.cash;
    out << (quint64)public_and_private_informations.warehouse_cash;
    out << (uint8_t)public_and_private_informations.itemOnMap.size();
    {
        Std::unordered_SetIterator<uint8_t> i(public_and_private_informations.itemOnMap);
        while (i.hasNext())
            out << i.next();
    }

    //send plant on map
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    const quint64 &time=QDateTime::currentDateTime().toMSecsSinceEpoch()/1000;
    out << (uint8_t)public_and_private_informations.plantOnMap.size();
    std::unordered_mapIterator<uint8_t/*dirtOnMap*/,PlayerPlant> i(public_and_private_informations.plantOnMap);
    while (i.hasNext()) {
        i.next();
        out << i.key();
        out << i.value().plant;
        if(time<i.value().mature_at)
            out << (uint16_t)(i.value().mature_at-time);
        else
            out << (uint16_t)0;
    }
    #endif

    //temporary variable
    uint32_t index;
    uint32_t size;

    //send recipes
    {
        index=0;
        out << (uint16_t)public_and_private_informations.recipes.size();
        Std::unordered_SetIterator<uint16_t> k(public_and_private_informations.recipes);
        while (k.hasNext())
            out << k.next();
    }

    //send monster
    index=0;
    size=public_and_private_informations.playerMonster.size();
    out << (uint8_t)size;
    while(index<size)
    {
        const QByteArray &data=FacilityLib::privateMonsterToBinary(public_and_private_informations.playerMonster.at(index));
        outputData+=data;
        out.device()->seek(out.device()->pos()+data.size());
        index++;
    }
    index=0;
    size=public_and_private_informations.warehouse_playerMonster.size();
    out << (uint8_t)size;
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
        out << (uint8_t)public_and_private_informations.reputation.size();
        QMapIterator<uint8_t,PlayerReputation> i(public_and_private_informations.reputation);
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
        out << (uint16_t)public_and_private_informations.quests.size();
        std::unordered_mapIterator<uint16_t,PlayerQuest> j(public_and_private_informations.quests);
        while (j.hasNext()) {
            j.next();
            out << j.key();
            out << j.value().step;
            out << j.value().finish_one_time;
        }
    }

    //send bot_already_beaten
    {
        out << (uint16_t)public_and_private_informations.bot_already_beaten.size();
        Std::unordered_SetIterator<uint16_t> k(public_and_private_informations.bot_already_beaten);
        while (k.hasNext())
            out << k.next();
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    postReply(query_id,outputData.constData(),outputData.size());
    sendInventory();
    updateCanDoFight();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    //send signals into the server
    #ifndef SERVERBENCHMARK
    normalOutput(std::stringLiteral("Logged: %1 on the map: %2 (%3,%4)")
                 .arg(public_and_private_informations.public_informations.pseudo)
                 .arg(map->map_file)
                 .arg(x)
                 .arg(y)
                 );
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(std::stringLiteral("load the normal player id: %1, simplified id: %2").arg(character_id).arg(public_and_private_informations.public_informations.simplifiedId));
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
        std::string mapToDebug=this->map->map_file;
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
        std::string mapToDebug=this->map->map_file;
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
        quint64 cash=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1)).toULongLong(&ok);
        if(!ok)
        {
            cash=0;
            normalOutput(std::stringLiteral("Warning: clan linked: %1 have wrong cash value, then reseted to 0"));
        }
        haveClanInfo(public_and_private_informations.clan,GlobalServerData::serverPrivateVariables.db_common->value(0),cash);
    }
    else
    {
        public_and_private_informations.clan=0;
        normalOutput(std::stringLiteral("Warning: clan linked: %1 is not found into db"));
    }
    loadLinkedData();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::loginIsWrong(const uint8_t &query_id, const uint8_t &returnCode, const std::string &debugMessage)
{
    //network send
    *(Client::loginIsWrongBuffer+1)=query_id;
    *(Client::loginIsWrongBuffer+3)=returnCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    replyOutputSize.remove(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));

    //send to server to stop the connection
    errorOutput(debugMessage);
}
#endif

void Client::loadPlayerAllow()
{
    if(!PreparedDBQueryCommon::db_query_select_allow.isEmpty())
    {
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(PreparedDBQueryCommon::db_query_select_allow.arg(character_id).toLatin1(),this,&Client::loadPlayerAllow_static);
        if(callback==NULL)
        {
            qDebug() << std::stringLiteral("Sql error for: %1, error: %2").arg(PreparedDBQueryCommon::db_query_select_allow).arg(GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
        const uint32_t &allowCode=std::string(GlobalServerData::serverPrivateVariables.db_common->value(0)).toUInt(&ok);
        if(ok)
        {
            if(allowCode<(uint32_t)DictionaryLogin::dictionary_allow_database_to_internal.size())
            {
                const ActionAllow &allow=DictionaryLogin::dictionary_allow_database_to_internal.at(allowCode);
                if(allow!=ActionAllow_Nothing)
                    public_and_private_informations.allow << allow;
                else
                {
                    ok=false;
                    normalOutput(std::stringLiteral("allow id: %1 is not reverse").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
                }
            }
            else
            {
                ok=false;
                normalOutput(std::stringLiteral("allow id: %1 out of reverse list").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
            }
        }
        else
            normalOutput(std::stringLiteral("allow id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db_common->value(0)));
    }
    loadItems();
}
