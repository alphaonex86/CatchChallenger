#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"
#include "DictionaryServer.h"
#include <iostream>

using namespace CatchChallenger;

void Client::selectCharacterServer(const uint8_t &query_id, const uint32_t &characterId, const uint64_t &characterCreationDate)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.empty())
    {
        characterSelectionIsWrong(query_id,0x04,"selectCharacterServer() Query is empty, bug");
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.asyncRead(this,&Client::selectCharacterServer_static,{std::to_string(characterId)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        characterSelectionIsWrong(query_id,0x02,GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.queryText()+": "+GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        characterCreationDateList[characterId]=characterCreationDate;

        paramToPassToCallBack.push(selectCharacterParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("SelectCharacterParam");
        #endif
        callbackRegistred.push(callback);
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
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.front());
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacterServer_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    paramToPassToCallBack.pop();
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db_server->clear();
}

void Client::selectCharacterServer_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="SelectCharacterParam")
    {
        std::cerr << "is not SelectCharacterParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    /*map(0),x(1),y(2),orientation(3),
    rescue_map(4),rescue_x(5),rescue_y(6),rescue_orientation(7),
    unvalidated_rescue_map(8),unvalidated_rescue_x(9),unvalidated_rescue_y(10),unvalidated_rescue_orientation(11),
    market_cash(12),botfight_id(13),itemonmap(14),quest(15),blob_version(16),date(17),plants(18)*/
    callbackRegistred.pop();
    const auto &characterIdString=std::to_string(characterId);
    const auto &sFrom1970String=std::to_string(sFrom1970());
    if(!GlobalServerData::serverPrivateVariables.db_server->next())
    {
        ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[profileIndex];

        serverProfileInternal.preparedQueryAddCharacterForServer.asyncWrite({
                               characterIdString,
                               sFrom1970String
                               });
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_last_connect.asyncWrite({
                    sFrom1970String,
                    characterIdString
                    });

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
        characterCreationDateList.erase(characterId);
        return;
    }
    else
    {
        bool ok;
        const uint64_t &server_date=GlobalServerData::serverPrivateVariables.db_server->stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(17),&ok);
        if(!ok || server_date<characterCreationDateList.at(characterId))
        {
            //drop before re-add
            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_character_server_by_id.asyncWrite({characterIdString});

            ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[profileIndex];

            serverProfileInternal.preparedQueryAddCharacterForServer.asyncWrite({
                                   characterIdString,
                                   sFrom1970String
                                   });
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_last_connect.asyncWrite({
                        sFrom1970String,
                        characterIdString
                        });

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
            characterCreationDateList.erase(characterId);
            return;
        }
    }
    characterCreationDateList.erase(characterId);

    bool ok;
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(simplifiedIdList.size()<=0)
        {
            characterSelectionIsWrong(query_id,0x04,"Not free id to login");
            return;
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }

    const uint8_t &blob_version=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(16),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version not a number");
        return;
    }
    if(blob_version!=GlobalServerData::serverPrivateVariables.server_blobversion_datapack)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version incorrect");
        return;
    }

    market_cash=GlobalServerData::serverPrivateVariables.db_server->stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(12),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"Market cash wrong: "+GlobalServerData::serverPrivateVariables.db_server->value(12));
        return;
    }

    //botfight_id
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(13),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"botfight_id not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.bot_already_beaten!=NULL)
        {
            delete public_and_private_informations.bot_already_beaten;
            public_and_private_informations.bot_already_beaten=NULL;
        }
        public_and_private_informations.bot_already_beaten=(char *)malloc(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        memset(public_and_private_informations.bot_already_beaten,0x00,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        if(data.size()>(uint16_t)(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1))
            memcpy(public_and_private_informations.bot_already_beaten,data_raw,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        else
            memcpy(public_and_private_informations.bot_already_beaten,data_raw,data.size());
    }
    //itemonmap
    {
        const std::vector<char> &itemonmap=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(14),&ok);
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        const char * const raw_itemonmap=itemonmap.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"itemonmap: "+GlobalServerData::serverPrivateVariables.db_server->value(14)+" is not a hexa");
            return;
        }
        else
        {
            if(itemonmap.size()%(2)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"item on map missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(14));
                return;
            }
            else
            {
                #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                public_and_private_informations.itemOnMap.reserve(itemonmap.size());
                #endif
                uint32_t lastItemonmapId=0;
                uint32_t pos=0;
                while(pos<itemonmap.size())
                {
                    uint32_t pointOnMapDatabaseId=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(
                                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                        itemonmap.data()
                                        #else
                                        raw_itemonmap
                                        #endif
                                        +pos))+lastItemonmapId;
                    if(pointOnMapDatabaseId>65535)
                        pointOnMapDatabaseId-=65536;
                    lastItemonmapId=static_cast<uint16_t>(pointOnMapDatabaseId);
                    if(!ok)
                    {
                        normalOutput("wrong value type for item on map, skip: "+std::to_string(pointOnMapDatabaseId));
                        pos+=2;
                        continue;
                    }
                    if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size())
                    {
                        normalOutput("item on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
                        pos+=2;
                        continue;
                    }
                    const DictionaryServer::MapAndPointItem &resolvedEntry=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(pointOnMapDatabaseId);
                    if(resolvedEntry.map==NULL)
                    {
                        normalOutput("item on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
                        pos+=2;
                        continue;
                    }
                    public_and_private_informations.itemOnMap.insert(static_cast<uint16_t>(pointOnMapDatabaseId));
                    pos+=2;
                }
            }
        }
    }
    //plants
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    {
        const std::vector<char> &plants=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(18),&ok);
        const char * const raw_plants=plants.data();
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"plants: "+GlobalServerData::serverPrivateVariables.db_server->value(17)+" is not a hexa");
            return;
        }
        else
        {
            if(plants.size()%(2/*pointOnMap*/+1/*plant*/+8/*timestamps*/)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"plants missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(17));
                return;
            }
            else
            {
                #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                public_and_private_informations.plantOnMap.reserve(plants.size()/(1+1+8));
                #endif
                PlayerPlant plant;
                uint32_t lastPlantId=0;
                uint32_t pos=0;
                while(pos<plants.size())
                {
                    uint32_t pointOnMap=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(
                                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                        plants.data()
                                        #else
                                        raw_plants
                                        #endif
                                        +pos))+lastPlantId;
                    if(pointOnMap>65535)
                        pointOnMap-=65536;
                    lastPlantId=static_cast<uint16_t>(pointOnMap);
                    pos+=2;

                    if(pointOnMap>=DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size())
                    {
                        normalOutput("dirt on map is not into the map list (1), skip: "+std::to_string(pointOnMap));
                        pos+=1+8;
                        continue;
                    }
                    if(DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.at(pointOnMap).map==NULL)
                    {
                        normalOutput("dirt on map is not into the map list (2), skip: "+std::to_string(pointOnMap));
                        pos+=1+8;
                        continue;
                    }

                    plant.plant=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            plants
                            #else
                            raw_plants
                            #endif
                            [pos];
                    ++pos;

                    if(CommonDatapack::commonDatapack.plants.find(plant.plant)==CommonDatapack::commonDatapack.plants.cend())
                    {
                        normalOutput("wrong value type for plant dirt on map, skip: "+std::to_string(plant.plant));
                        pos+=8;
                        continue;
                    }

                    uint64_t mature_at;
                    memcpy(&mature_at,
                            raw_plants
                            +pos,sizeof(uint64_t));
                    plant.mature_at=le64toh(mature_at)+CommonDatapack::commonDatapack.plants.at(plant.plant).fruits_seconds;
                    pos+=8;

                    public_and_private_informations.plantOnMap[static_cast<uint16_t>(pointOnMap)]=plant;
                }
            }
        }
    }
    #endif
    //quest
    {
        const std::vector<char> &quests=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(15),&ok);
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        const char * const raw_quests=quests.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"plants: "+GlobalServerData::serverPrivateVariables.db_server->value(15)+" is not a hexa");
            return;
        }
        else
        {
            if(quests.size()%(2/*quest incremental id*/+1/*finish_one_time*/+1/*quest.step*/)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"quests missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(15));
                return;
            }
            else
            {
                #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                public_and_private_informations.quests.reserve(quests.size()/(2/*quest incremental id*/+1/*finish_one_time*/+1/*quest.step*/));
                #endif
                PlayerQuest playerQuest;
                uint32_t lastQuestId=0;
                uint32_t pos=0;
                while(pos<quests.size())
                {
                    uint32_t questInt=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(
                                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                        quests.data()
                                        #else
                                        raw_quests
                                        #endif
                                        +pos))+lastQuestId;
                    if(questInt>65535)
                        questInt-=65536;
                    lastQuestId=questInt;
                    const uint16_t questId=static_cast<uint16_t>(questInt);
                    pos+=2;
                    playerQuest.finish_one_time=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;
                    playerQuest.step=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;

                    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==
                            CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                    {
                        normalOutput("wrong value type for quest on map, skip: "+std::to_string(questId));
                        pos+=2;
                        continue;
                    }

                    const Quest &datapackQuest=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
                    if(playerQuest.step>datapackQuest.steps.size())
                    {
                        normalOutput("step out of quest range "+std::to_string(playerQuest.step)+", fix for quest: "+std::to_string(questId));
                        playerQuest.step=1;
                    }
                    if(playerQuest.step==0 && !playerQuest.finish_one_time)
                    {
                        normalOutput("can't be to step 0 if have never finish the quest, skip: "+std::to_string(questId));
                        pos+=2;
                        continue;
                    }
                    public_and_private_informations.quests[questId]=playerQuest;
                    if(playerQuest.step>0)
                        addQuestStepDrop(questId,playerQuest.step);
                }
            }
        }
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    Orientation orientation;
    const uint32_t &orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            orientation=static_cast<Orientation>(orientationInt);
        else
        {
            orientation=Orientation_bottom;
            normalOutput("Wrong orientation corrected with bottom");
        }
    }
    else
    {
        orientation=Orientation_bottom;
        normalOutput("Wrong orientation (not number) corrected with bottom: "+GlobalServerData::serverPrivateVariables.db_server->value(3));
    }
    CommonMap * map=NULL;
    uint8_t x,y;
    //all is rights
    if(!GlobalServerData::serverSettings.teleportIfMapNotFoundOrOutOfMap || profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        const uint32_t &map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"map_database_id is not a number");
            return;
        }
        if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
        {
            characterSelectionIsWrong(query_id,0x04,"map_database_id out of range");
            return;
        }
        map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
        if(map==NULL)
        {
            characterSelectionIsWrong(query_id,0x04,"map_database_id have not reverse: "+std::to_string(map_database_id)+", mostly due to start previously start with another mainDatapackCode");
            return;
        }
        x=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"x coord is not a number");
            return;
        }
        y=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"y coord is not a number");
            return;
        }
        if(x>=map->width)
        {
            characterSelectionIsWrong(query_id,0x04,"x to out of map: "+std::to_string(x)+" >= "+std::to_string(map->width)+" ("+map->map_file+")");
            return;
        }
        if(y>=map->height)
        {
            characterSelectionIsWrong(query_id,0x04,"y to out of map: "+std::to_string(y)+" >= "+std::to_string(map->height)+" ("+map->map_file+")");
            return;
        }
    }
    else
    {
        bool goToFinal=false;
        const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);
        const uint32_t &map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
            goToFinal=true;
        if(!goToFinal)
            if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
                goToFinal=true;
        if(!goToFinal)
        {
            map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
            if(map==NULL)
                goToFinal=true;
            if(!goToFinal)
            {
                x=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
                if(!ok)
                    goToFinal=true;
                y=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                if(!ok)
                    goToFinal=true;
                if(x>=map->width)
                    goToFinal=true;
                if(y>=map->height)
                    goToFinal=true;
            }
        }
        if(goToFinal)
        {
            map=serverProfileInternal.map;
            x=serverProfileInternal.x;
            y=serverProfileInternal.y;
        }
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
