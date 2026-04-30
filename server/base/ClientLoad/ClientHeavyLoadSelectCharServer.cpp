#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../MapServer.hpp"
#include "../DictionaryServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../MapManagement/MapVisibilityAlgorithm.hpp"
#include <iostream>

using namespace CatchChallenger;

void Client::selectCharacterServer(const uint8_t &query_id, const uint32_t &characterId, const uint64_t &characterCreationDate)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_HARDENED
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.empty())
    {
        characterSelectionIsWrong(query_id,0x04,"selectCharacterServer() Query is empty, bug");
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_character_server_by_id.asyncRead(this,&Client::selectCharacterServer_static,{std::to_string(characterId)});
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
        #ifdef CATCHCHALLENGER_HARDENED
        paramToPassToCallBackType.push("SelectCharacterParam");
        #endif
        callbackRegistred.push(callback);
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    characterCreationDateList[characterId]=characterCreationDate;

    paramToPassToCallBack.push(selectCharacterParam);
    #ifdef CATCHCHALLENGER_HARDENED
    paramToPassToCallBackType.push("SelectCharacterParam");
    #endif
    selectCharacterServer_object();
    #elif CATCHCHALLENGER_DB_FILE
    (void)characterCreationDate;
    #else
    #error Define what do here
    #endif
}

void Client::selectCharacterServer_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectCharacterServer_object();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::selectCharacterServer_object()
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.front());
    #ifdef CATCHCHALLENGER_HARDENED
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacterServer_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    paramToPassToCallBack.pop();
    delete selectCharacterParam;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::selectCharacterServer_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_HARDENED
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
    quest(12),date(13)*/
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    const std::string &characterIdString=std::to_string(characterId);
    const std::string &sFrom1970String=std::to_string(sFrom1970());
    if(!GlobalServerData::serverPrivateVariables.db_server->next())
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    {
        if(profileIndex>=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
        {
            errorOutput("out of bound");
            std::cerr << "out of bound (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[profileIndex];

        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        serverProfileInternal.preparedQueryAddCharacterForServer.asyncWrite({
                               characterIdString,
                               sFrom1970String
                               });
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_last_connect.asyncWrite({
                    sFrom1970String,
                    characterIdString
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif

        characterIsRightWithParsedRescue(query_id,
            characterId,
            serverProfileInternal.mapIndex,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //rescue
            serverProfileInternal.mapIndex,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //last unvalidated
            serverProfileInternal.mapIndex,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation
        );
        characterCreationDateList.erase(characterId);
        return;
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    else
    {
        bool ok;
        const uint64_t &server_date=GlobalServerData::serverPrivateVariables.db_server->stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(13),&ok);
        if(!ok || server_date<characterCreationDateList.at(characterId))
        {
            //drop before re-add
            GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_character_server_by_id.asyncWrite({characterIdString});

            if(profileIndex>=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
            {
                errorOutput("out of bound");
                std::cerr << "out of bound (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
                return;
            }
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
                serverProfileInternal.mapIndex,
                serverProfileInternal.x,
                serverProfileInternal.y,
                serverProfileInternal.orientation,
                //rescue
                serverProfileInternal.mapIndex,
                serverProfileInternal.x,
                serverProfileInternal.y,
                serverProfileInternal.orientation,
                //last unvalidated
                serverProfileInternal.mapIndex,
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

    //quest
    {
        const std::vector<char> &quests=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(12),&ok);
        #ifndef CATCHCHALLENGER_HARDENED
        const char * const raw_quests=quests.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"quest: "+GlobalServerData::serverPrivateVariables.db_server->value(12)+" is not a hexa");
            return;
        }
        else
        {
            if(quests.size()%(2/*quest incremental id*/+1/*finish_one_time*/+1/*quest.step*/)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"quests missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(12));
                return;
            }
            else
            {
                PlayerQuest playerQuest;
                uint32_t lastQuestId=0;
                uint32_t pos=0;
                while(pos<quests.size())
                {
                    uint32_t questInt=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(
                                        #ifdef CATCHCHALLENGER_HARDENED
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
                            #ifdef CATCHCHALLENGER_HARDENED
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;
                    playerQuest.step=
                            #ifdef CATCHCHALLENGER_HARDENED
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;

                    if(!CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
                    {
                        normalOutput("wrong value type for quest on map, skip: "+std::to_string(questId));
                        pos+=2;
                        continue;
                    }

                    const Quest &datapackQuest=CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId);
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
                    #ifdef CATCHCHALLENGER_HARDENED
                    if(public_and_private_informations.quests.find(questId)!=public_and_private_informations.quests.cend())
                    {
                        normalOutput("can't be to step 0 if have never finish the quest, skip: "+std::to_string(questId));
                        pos+=2;
                        continue;
                    }
                    #endif
                    public_and_private_informations.quests[questId]=playerQuest;
                    if(playerQuest.step>0)
                        addQuestStepDrop(questId,playerQuest.step);
                }
            }
        }
    }

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
    CATCHCHALLENGER_TYPE_MAPID mapIndex=0;
    uint8_t x,y;
    //all is rights
    if(!GlobalServerData::serverSettings.teleportIfMapNotFoundOrOutOfMap || profileIndex>=CommonDatapack::commonDatapack.get_profileList().size())
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
        mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(map_database_id);
        if(mapIndex>=MapVisibilityAlgorithm::flat_map_list.size())
        {
            characterSelectionIsWrong(query_id,0x04,"map_database_id have not reverse: "+std::to_string(map_database_id)+", mostly due to start previously start with another mainDatapackCode");
            return;
        }
        const MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list.at(mapIndex);
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
        if(x>=map.width)
        {
            characterSelectionIsWrong(query_id,0x04,"x to out of map: "+std::to_string(x)+" >= "+std::to_string(map.width));
            return;
        }
        if(y>=map.height)
        {
            characterSelectionIsWrong(query_id,0x04,"y to out of map: "+std::to_string(y)+" >= "+std::to_string(map.height));
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
            mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(map_database_id);
            if(mapIndex>=MapVisibilityAlgorithm::flat_map_list.size())
                goToFinal=true;
            if(!goToFinal)
            {
                const MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list.at(mapIndex);
                x=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
                if(!ok)
                    goToFinal=true;
                y=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
                if(!ok)
                    goToFinal=true;
                if(x>=map.width)
                    goToFinal=true;
                if(y>=map.height)
                    goToFinal=true;
                if(ok)
                    if(!MoveOnTheMap::isWalkable(map,x,y))
                        goToFinal=true;
            }
        }
        if(goToFinal)
        {
            normalOutput("Problem with map spawn, fixed by rescue");
            mapIndex=serverProfileInternal.mapIndex;
            x=serverProfileInternal.x;
            y=serverProfileInternal.y;
        }
    }
    characterIsRightWithRescue(query_id,
        characterId,
        mapIndex,
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
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::loadCharacterByMap()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_select_character_bymap.asyncRead(this,&Client::loadCharacterByMap_static,{std::to_string(character_id_db)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_select_character_bymap.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        characterIsRightSendData();
        return;
    }
    else
        callbackRegistred.push(callback);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    loadCharacterByMap_return();
    #elif CATCHCHALLENGER_DB_FILE
    loadCharacterByMap_return();
    #else
    #error Define what do here
    #endif
}

void Client::loadCharacterByMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadCharacterByMap_return();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::loadCharacterByMap_return()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint32_t &map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("character_bymap: map is not a number, skip row");
            continue;
        }
        if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
        {
            normalOutput("character_bymap: map_database_id out of range: "+std::to_string(map_database_id));
            continue;
        }
        const CATCHCHALLENGER_TYPE_MAPID &mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(map_database_id);
        if(mapIndex>=MapVisibilityAlgorithm::flat_map_list.size())
        {
            normalOutput("character_bymap: mapIndex out of range: "+std::to_string(mapIndex));
            continue;
        }
        const MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list.at(mapIndex);
        Player_private_and_public_informations_Map &playerMapData=public_and_private_informations.mapData[mapIndex];

        //items blob: [x(1B), y(1B)] pairs
        {
            const std::vector<char> &items_data=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
            if(!ok)
                normalOutput("character_bymap: items is not hexa for map: "+std::to_string(map_database_id));
            else
            {
                if(items_data.size()%2!=0)
                    normalOutput("character_bymap: items blob has wrong size for map: "+std::to_string(map_database_id));
                else
                {
                    const char * const raw=items_data.data();
                    uint32_t pos=0;
                    while(pos<items_data.size())
                    {
                        const uint8_t ix=static_cast<uint8_t>(raw[pos]);
                        pos+=1;
                        const uint8_t iy=static_cast<uint8_t>(raw[pos]);
                        pos+=1;
                        if(ix>=map.width || iy>=map.height)
                        {
                            normalOutput("character_bymap: item coords out of range on map: "+std::to_string(map_database_id));
                            continue;
                        }
                        if(map.items.find(std::make_pair(ix,iy))==map.items.cend())
                        {
                            normalOutput("character_bymap: no item at position on map: "+std::to_string(map_database_id));
                            continue;
                        }
                        playerMapData.items.insert(std::make_pair(ix,iy));
                    }
                }
            }
        }

        //plants blob: [x(1B), y(1B), plant_id(1B), mature_at(8B LE)] entries
        {
            const std::vector<char> &plants_data=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
            if(!ok)
                normalOutput("character_bymap: plants is not hexa for map: "+std::to_string(map_database_id));
            else
            {
                if(plants_data.size()%(1+1+1+8)!=0)
                    normalOutput("character_bymap: plants blob has wrong size for map: "+std::to_string(map_database_id));
                else
                {
                    const char * const raw=plants_data.data();
                    uint32_t pos=0;
                    while(pos<plants_data.size())
                    {
                        const uint8_t px=static_cast<uint8_t>(raw[pos]);
                        pos+=1;
                        const uint8_t py=static_cast<uint8_t>(raw[pos]);
                        pos+=1;
                        const uint8_t plant_id=static_cast<uint8_t>(raw[pos]);
                        pos+=1;
                        uint64_t mature_at;
                        memcpy(&mature_at,raw+pos,sizeof(uint64_t));
                        mature_at=le64toh(mature_at);
                        pos+=8;
                        if(px>=map.width || py>=map.height)
                        {
                            normalOutput("character_bymap: plant coords out of range on map: "+std::to_string(map_database_id));
                            continue;
                        }
                        if(!CommonDatapack::commonDatapack.has_plant(plant_id))
                        {
                            normalOutput("character_bymap: unknown plant_id: "+std::to_string(plant_id)+" on map: "+std::to_string(map_database_id));
                            continue;
                        }
                        PlayerPlant playerPlant;
                        playerPlant.plant=plant_id;
                        playerPlant.mature_at=mature_at;
                        playerMapData.plants[std::make_pair(px,py)]=playerPlant;
                    }
                }
            }
        }

        //fights blob: [bot_id(1B)] entries
        {
            const std::vector<char> &fights_data=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
            if(!ok)
                normalOutput("character_bymap: fights is not hexa for map: "+std::to_string(map_database_id));
            else
            {
                const char * const raw=fights_data.data();
                uint32_t pos=0;
                while(pos<fights_data.size())
                {
                    const uint8_t bot_id=static_cast<uint8_t>(raw[pos]);
                    pos+=1;
                    if(map.botFights.find(bot_id)==map.botFights.cend())
                    {
                        normalOutput("character_bymap: unknown bot_id: "+std::to_string(bot_id)+" on map: "+std::to_string(map_database_id));
                        continue;
                    }
                    playerMapData.bots_beaten.insert(bot_id);
                }
            }
        }

        //industries blob: for each industry [last_update(8B LE), resources_count(1B), [item_id(2B LE) quantity(4B LE)]..., products_count(1B), [item_id(2B LE) quantity(4B LE)]...]
        {
            const std::vector<char> &industries_data=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(4),&ok);
            if(!ok)
                normalOutput("character_bymap: industries is not hexa for map: "+std::to_string(map_database_id));
            else
            {
                const char * const raw=industries_data.data();
                uint32_t pos=0;
                uint8_t industryIndex=0;
                while(pos<industries_data.size() && industryIndex<map.industries.size())
                {
                    if(pos+8>industries_data.size())
                    {
                        normalOutput("character_bymap: industries blob truncated at last_update for map: "+std::to_string(map_database_id));
                        break;
                    }
                    IndustryStatus status;
                    memcpy(&status.last_update,raw+pos,sizeof(uint64_t));
                    status.last_update=le64toh(status.last_update);
                    pos+=8;

                    if(pos+1>industries_data.size())
                    {
                        normalOutput("character_bymap: industries blob truncated at resources_count for map: "+std::to_string(map_database_id));
                        break;
                    }
                    const uint8_t resources_count=static_cast<uint8_t>(raw[pos]);
                    pos+=1;
                    uint8_t ri=0;
                    while(ri<resources_count)
                    {
                        if(pos+2+4>industries_data.size())
                        {
                            normalOutput("character_bymap: industries blob truncated at resource entry for map: "+std::to_string(map_database_id));
                            break;
                        }
                        uint16_t item_id;
                        memcpy(&item_id,raw+pos,sizeof(uint16_t));
                        item_id=le16toh(item_id);
                        pos+=2;
                        uint32_t quantity;
                        memcpy(&quantity,raw+pos,sizeof(uint32_t));
                        quantity=le32toh(quantity);
                        pos+=4;
                        status.resources[item_id]=quantity;
                        ri++;
                    }
                    if(ri<resources_count)
                        break;

                    if(pos+1>industries_data.size())
                    {
                        normalOutput("character_bymap: industries blob truncated at products_count for map: "+std::to_string(map_database_id));
                        break;
                    }
                    const uint8_t products_count=static_cast<uint8_t>(raw[pos]);
                    pos+=1;
                    uint8_t pi=0;
                    while(pi<products_count)
                    {
                        if(pos+2+4>industries_data.size())
                        {
                            normalOutput("character_bymap: industries blob truncated at product entry for map: "+std::to_string(map_database_id));
                            break;
                        }
                        uint16_t item_id;
                        memcpy(&item_id,raw+pos,sizeof(uint16_t));
                        item_id=le16toh(item_id);
                        pos+=2;
                        uint32_t quantity;
                        memcpy(&quantity,raw+pos,sizeof(uint32_t));
                        quantity=le32toh(quantity);
                        pos+=4;
                        status.products[item_id]=quantity;
                        pi++;
                    }
                    if(pi<products_count)
                        break;

                    playerMapData.industriesStatus.push_back(status);
                    industryIndex++;
                }
            }
        }
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    characterIsRightSendData();
}
