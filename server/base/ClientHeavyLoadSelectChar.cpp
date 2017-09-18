#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "../../general/base/cpp11addition.h"
#include "DatabaseFunction.h"
#include "SqlFunction.h"
#include "PreparedDBQuery.h"
#include "DictionaryLogin.h"
#include "DictionaryServer.h"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.h"
#endif

#include <chrono>

using namespace CatchChallenger;

void Client::characterSelectionIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(1);
    posOutput+=4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=returnCode;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    //send to server to stop the connection
    errorOutput(debugMessage);
}

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::selectCharacter(const uint8_t &query_id, const char * const token)
{
    const auto &time=sFrom1970();
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-LinkToMaster::maxLockAge))
        {
            if(memcmp(tokenAuth.token,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)==0)
            {
                delete tokenAuth.token;
                account_id=tokenAuth.accountIdRequester;/// \warning need take care of only write if character is selected
                selectCharacter(query_id,tokenAuth.characterId);
                tokenAuthList.erase(tokenAuthList.begin()+index);
                flags|=0x08;
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
    errorOutput("selectCharacter() Token never found to login, bug");
}

void Client::purgeTokenAuthList()
{
    const auto &time=sFrom1970();
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-LinkToMaster::maxLockAge))
            index++;
        else
        {
            LinkToMaster::linkToMaster->characterDisconnected(tokenAuth.characterId);
            tokenAuthList.erase(tokenAuthList.begin()+index);
        }
    }
}
#endif

void Client::characterIsRightWithRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                  const std::string &rescue_map, const std::string &rescue_x, const std::string &rescue_y, const std::string &rescue_orientation,
                  const std::string &unvalidated_rescue_map, const std::string &unvalidated_rescue_x, const std::string &unvalidated_rescue_y, const std::string &unvalidated_rescue_orientation
                                             )
{
    bool ok;
    const uint32_t &rescue_map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_map,&ok);
    if(!ok)
    {
        normalOutput("rescue_map_database_id is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput("rescue_map_database_id out of range");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id)==NULL)
    {
        normalOutput("rescue_map_database_id have not reverse (fixed by current map)");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id);
    if(rescue_map_final==NULL)
    {
        normalOutput("rescue map not resolved");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_x=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_x,&ok);
    if(!ok)
    {
        normalOutput("rescue x coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_y=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_y,&ok);
    if(!ok)
    {
        normalOutput("rescue y coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=rescue_map_final->width)
    {
        normalOutput("rescue x to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=rescue_map_final->height)
    {
        normalOutput("rescue y to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint32_t &orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_orientation,&ok);
    Orientation rescue_new_orientation;
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            rescue_new_orientation=static_cast<Orientation>(orientationInt);
        else
        {
            rescue_new_orientation=Orientation_bottom;
            normalOutput("Wrong rescue orientation corrected with bottom");
        }
    }
    else
    {
        rescue_new_orientation=Orientation_bottom;
        normalOutput("Wrong rescue orientation (not number) corrected with bottom");
    }





    const uint32_t &unvalidated_rescue_map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_map,&ok);
    if(!ok)
    {
        normalOutput("unvalidated_rescue_map_database_id is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput("unvalidated_rescue_map_database_id out of range");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id)==NULL)
    {
        normalOutput("unvalidated_rescue_map_database_id have not reverse (fixed by current map)");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *unvalidated_rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id);
    if(unvalidated_rescue_map_final==NULL)
    {
        normalOutput("unvalidated rescue map not resolved");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_x=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_x,&ok);
    if(!ok)
    {
        normalOutput("unvalidated rescue x coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_y=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_y,&ok);
    if(!ok)
    {
        normalOutput("unvalidated rescue y coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=unvalidated_rescue_map_final->width)
    {
        normalOutput("unvalidated rescue x to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=unvalidated_rescue_map_final->height)
    {
        normalOutput("unvalidated rescue y to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    Orientation unvalidated_rescue_new_orientation;
    const uint32_t &unvalidated_orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_orientation,&ok);
    if(ok)
    {
        if(unvalidated_orientationInt>=1 && unvalidated_orientationInt<=4)
            unvalidated_rescue_new_orientation=static_cast<Orientation>(unvalidated_orientationInt);
        else
        {
            unvalidated_rescue_new_orientation=Orientation_bottom;
            normalOutput("Wrong unvalidated orientation corrected with bottom");
        }
    }
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        normalOutput("Wrong unvalidated orientation (not number) corrected with bottom");
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
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_clan.empty())
    {
        errorOutput("loginIsRightWithParsedRescue() Query is empty, bug");
        return;
    }
    #endif
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(simplifiedIdList.empty())
        {
            errorOutput("Client::characterIsRightWithParsedRescue(): simplifiedIdList is empty, no more id");
            return;
        }
        public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.front();
        simplifiedIdList.erase(simplifiedIdList.begin());
        break;
        case MapVisibilityAlgorithmSelection_None:
        public_and_private_informations.public_informations.simplifiedId = 0;
        break;
    }
    //load the variables
    character_id=characterId;
    stat=ClientStat::CharacterSelecting;
    GlobalServerData::serverPrivateVariables.connected_players_id_list.insert(characterId);
    connectedSince=sFrom1970();
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
    selectCharacterQueryId.push_back(query_id);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(public_and_private_informations.clan!=0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.find(public_and_private_informations.clan)!=GlobalServerData::serverPrivateVariables.clanList.cend())
        {
            GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan]->players.push_back(this);
            loadMonsters();
            return;
        }
        else
        {
            normalOutput("First client of the clan: "+std::to_string(public_and_private_informations.clan)+", get the info");
            //do the query
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_clan.asyncRead(this,&Client::selectClan_static,{std::to_string(public_and_private_informations.clan)});
            if(callback==NULL)
            {
                std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_clan.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
                loadMonsters();
                return;
            }
            else
                callbackRegistred.push(callback);
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
        loadMonsters();
        return;
    }
}

/*void Client::loadItemOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_itemOnMap.empty())
    {
        errorOutput("loadBotAlreadyBeaten() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_itemOnMap;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadItemOnMap_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        loadPlantOnMap();
        #else
        characterIsRightFinalStep();
        #endif
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadItemOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItemOnMap_return();
}

void Client::loadItemOnMap_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &pointOnMapDatabaseId=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for item on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput("item on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput("item on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        const uint16_t &indexOfItemOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).indexOfItemOnMap;
        if(indexOfItemOnMap==255)
        {
            normalOutput("item on map is not into the map list (3), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfItemOnMap>=Client::indexOfItemOnMap)
        {
            normalOutput("item on map is not into the map list (4), skip: "+std::to_string(indexOfItemOnMap)+" on "+std::to_string(Client::indexOfItemOnMap));
            continue;
        }
        public_and_private_informations.itemOnMap.insert(indexOfItemOnMap);
    }
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    loadPlantOnMap();
    #else
    characterIsRightFinalStep();
    #endif
}*/


/*#ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void Client::loadPlantOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_plant.empty())
    {
        errorOutput("loadBotAlreadyBeaten() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_plant;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadPlantOnMap_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        characterIsRightFinalStep();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadPlantOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlantOnMap_return();
}

void Client::loadPlantOnMap_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &pointOnMapDatabaseId=GlobalServerData::serverPrivateVariables.db_server->stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for dirt on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput("dirt on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput("dirt on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        const uint8_t &indexOfDirtOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).indexOfDirtOnMap;
        if(indexOfDirtOnMap==255)
        {
            normalOutput("dirt on map is not into the map list (3), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfDirtOnMap>=Client::indexOfDirtOnMap)
        {
            normalOutput("dirt on map is not into the map list (4), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }

        const uint32_t &plant=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for dirt plant id on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(CommonDatapack::commonDatapack.plants.find(plant)==CommonDatapack::commonDatapack.plants.cend())
        {
            normalOutput("wrong value type for plant dirt on map, skip: "+std::to_string(plant));
            continue;
        }
        const uint32_t &plant_timestamps=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for plant timestamps on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        {
            PlayerPlant playerPlant;
            playerPlant.plant=plant;
            playerPlant.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.at(plant).fruits_seconds;
            public_and_private_informations.plantOnMap[indexOfDirtOnMap]=playerPlant;
        }
    }
    characterIsRightFinalStep();
}
#endif*/

void Client::selectClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectClan_return();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::selectClan_return()
{
    callbackRegistred.pop();
    //parse the result
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        uint64_t cash=GlobalServerData::serverPrivateVariables.db_common->stringtouint64(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            cash=0;
            normalOutput("Warning: clan linked: have wrong cash value, then reseted to 0");
        }
        haveClanInfo(public_and_private_informations.clan,GlobalServerData::serverPrivateVariables.db_common->value(0),cash);
    }
    else
    {
        public_and_private_informations.clan=0;
        normalOutput("Warning: clan linked: %1 is not found into db");
    }
    loadMonsters();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::loginIsWrong(const uint8_t &query_id, const uint8_t &returnCode, const std::string &debugMessage)
{
    //network send
    *(Client::loginIsWrongBuffer+1)=query_id;
    *(Client::loginIsWrongBuffer+1+1+4)=returnCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    inputQueryNumberToPacketCode[query_id]=0;
    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));

    //send to server to stop the connection
    errorOutput(debugMessage);
}
#endif

/*void Client::loadPlayerAllow()
{
    if(!GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_allow.empty())
    {
        std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_allow;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadPlayerAllow_static);
        if(callback==NULL)
        {
            std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_select_allow << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            loadItems();
            return;
        }
        else
            callbackRegistred.push(callback);
    }
}

void Client::loadPlayerAllow_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerAllow_return();
}

void Client::loadPlayerAllow_return()
{
    callbackRegistred.pop();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &allowCode=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(allowCode<(uint32_t)DictionaryLogin::dictionary_allow_database_to_internal.size())
            {
                const ActionAllow &allow=DictionaryLogin::dictionary_allow_database_to_internal.at(allowCode);
                if(allow!=ActionAllow_Nothing)
                    public_and_private_informations.allow.insert(allow);
                else
                {
                    ok=false;
                    normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not reverse");
                }
            }
            else
            {
                ok=false;
                normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" out of reverse list");
            }
        }
        else
            normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
    }
    loadItems();
}*/
