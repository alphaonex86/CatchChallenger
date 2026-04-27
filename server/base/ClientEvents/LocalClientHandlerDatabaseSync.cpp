#include "../Client.hpp"
#include "../PreparedDBQuery.hpp"
#include "../GlobalServerData.hpp"
#include "../MapServer.hpp"
#include "../MapManagement/MapVisibilityAlgorithm.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

void Client::updateObjectInDatabase()
{
    if(public_and_private_informations.items.empty())
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item.asyncWrite({
            std::string(),
            std::to_string(character_id_db)
            });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
    }
    else
    {
        uint16_t lastItemId=0;
        uint32_t pos=0;
        char item_raw[(2+4)*public_and_private_informations.items.size()];
        std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY>::iterator i=public_and_private_informations.items.begin();
        while(i!=public_and_private_informations.items.cend())
        {
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint16_t item;
            if(lastItemId<=i->first)
            {
                item=i->first-lastItemId;
                lastItemId=i->first;
            }
            else
            {
                item=static_cast<uint16_t>(65536-lastItemId)+static_cast<uint16_t>(i->first);
                lastItemId=i->first;
            }
            #else
            //ordened
            const uint16_t &item=i->first-lastItemId;
            lastItemId=i->first;
            #endif
            const uint32_t &quantity=i->second;
            *reinterpret_cast<uint16_t *>(item_raw+pos)=htole16(item);
            pos+=2;
            *reinterpret_cast<uint32_t *>(item_raw+pos)=htole32(quantity);
            pos+=4;
            ++i;
        }
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item.asyncWrite({
            binarytoHexa(item_raw,static_cast<uint32_t>(sizeof(item_raw))),
            std::to_string(character_id_db)
            });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
    }
}

/*void Client::updateMonsterInDatabase()
{
    if(public_and_private_informations.playerMonster.empty())
    {
        dbQueryWriteCommon(
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster.compose(
                        std::string(),
                    std::to_string(character_id_db)
                        )
                    );
    }
    else
    {
        std::string monster;
        uint32_t pos=0;
        char monster_raw[(2+4)*public_and_private_informations.playerMonster.size()];
        unsigned int index=0;
        while(index<public_and_private_informations.playerMonster.size())
        {
            *reinterpret_cast<uint32_t *>(monster_raw+pos)=htole32(public_and_private_informations.playerMonster.at(index).id);
            pos+=4;
            index++;
        }
        monster=binarytoHexa(monster_raw,sizeof(monster_raw));
        dbQueryWriteCommon(
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster.compose(
                        binarytoHexa(monster_raw,sizeof(monster_raw)),
                    std::to_string(character_id_db)
                        )
                    );
    }
}*/

void Client::updateObjectInDatabaseAndEncyclopedia()
{
    std::string item;
    if(!public_and_private_informations.items.empty())
    {
        uint16_t lastItemId=0;
        uint32_t pos=0;
        char item_raw[(2+4)*public_and_private_informations.items.size()];
        std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY>::iterator i=public_and_private_informations.items.begin();
        while(i!=public_and_private_informations.items.cend())
        {
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint16_t item;
            if(lastItemId<=i->first)
            {
                item=i->first-lastItemId;
                lastItemId=i->first;
            }
            else
            {
                item=static_cast<uint16_t>(65536-lastItemId)+static_cast<uint16_t>(i->first);
                lastItemId=i->first;
            }
            #else
            //ordened
            const uint16_t &item=i->first-lastItemId;
            lastItemId=i->first;
            #endif
            const uint32_t &quantity=i->second;
            *reinterpret_cast<uint16_t *>(item_raw+pos)=htole16(item);
            pos+=2;
            *reinterpret_cast<uint32_t *>(item_raw+pos)=htole32(quantity);
            pos+=4;
            ++i;
        }
        item=binarytoHexa(item_raw,static_cast<uint32_t>(sizeof(item_raw)));
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item_and_encyclopedia.asyncWrite({
        item,
        binarytoHexa(public_and_private_informations.encyclopedia_item,
        static_cast<uint32_t>(CommonDatapack::commonDatapack.get_items_size())/8+1),
        std::to_string(character_id_db)
        });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::updateMonsterInDatabaseEncyclopedia()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster_encyclopedia.asyncWrite({
        binarytoHexa(public_and_private_informations.encyclopedia_monster,
        static_cast<uint32_t>(CommonDatapack::commonDatapack.get_monsters_size())/8+1),
        std::to_string(character_id_db)
        });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::syncDatabaseAllow()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    std::string allow_create_clanStr;
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
    if(GlobalServerData::serverPrivateVariables.db_common->databaseType()==DatabaseBase::DatabaseType::PostgreSQL)
        allow_create_clanStr=public_and_private_informations.allow_create_clan?"true":"false";
    else
    #endif
        allow_create_clanStr=public_and_private_informations.allow_create_clan?"1":"0";
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_allow.asyncWrite({
                allow_create_clanStr,
                std::to_string(character_id_db)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::syncDatabaseReputation()
{
    char *buffer=NULL;
    if(public_and_private_informations.reputation.size()*(4+1+1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        buffer=(char *)malloc(public_and_private_informations.reputation.size()*(4+1+1));
    else
        buffer=ProtocolParsingBase::tempBigBufferForOutput;

    uint8_t lastReputationId=0;
    uint32_t posOutput=0;
    std::map<uint8_t,PlayerReputation>::iterator i=public_and_private_informations.reputation.begin();
    while(i!=public_and_private_informations.reputation.cend())
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(i->first>=CommonDatapack::commonDatapack.get_reputation().size())
        {
            std::cerr << "public_and_private_informations.reputation internal id is out of range to save: " << i->first << std::endl;
            abort();
        }
        #endif
        const uint8_t &databaseType=static_cast<uint8_t>(CommonDatapack::commonDatapack.get_reputation().at(i->first).reverse_database_id);
        #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        //not ordened
        uint8_t type;
        if(lastReputationId<=databaseType)
        {
            type=databaseType-lastReputationId;
            lastReputationId=databaseType;
        }
        else
        {
            type=static_cast<uint8_t>(256-lastReputationId+databaseType);
            lastReputationId=databaseType;
        }
        #else
        //ordened
        const uint8_t &type=databaseType-lastReputationId;
        lastReputationId=databaseType;
        #endif
        const PlayerReputation &reputation=i->second;
        *reinterpret_cast<uint32_t *>(buffer+posOutput)=htole32(reputation.point);
        posOutput+=4;
        buffer[posOutput]=type;
        posOutput+=1;
        buffer[posOutput]=reputation.level;
        posOutput+=1;
        ++i;
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_reputations.asyncWrite({
                binarytoHexa(buffer,posOutput),
                std::to_string(character_id_db)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    if(public_and_private_informations.reputation.size()*(4+1+1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        delete buffer;
}

void Client::syncBotAlreadyBeaten(const CATCHCHALLENGER_TYPE_MAPID &mapId)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(public_and_private_informations.mapData.find(mapId)==public_and_private_informations.mapData.cend())
        return;
    const Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData.at(mapId);
    const uint32_t blobSize=mapData.bots_beaten.size();
    char buffer[blobSize];
    uint32_t posOutput=0;
    for(const CATCHCHALLENGER_TYPE_BOTID &bot_id : mapData.bots_beaten)
    {
        buffer[posOutput]=bot_id;
        posOutput+=1;
    }
    const uint32_t map_database_id=MapVisibilityAlgorithm::flat_map_list.at(mapId).id_db;
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_character_bot_already_beaten.asyncWrite({
                binarytoHexa(buffer,posOutput),
                std::to_string(character_id_db),
                std::to_string(map_database_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)mapId;
    #elif CATCHCHALLENGER_DB_FILE
    (void)mapId;
    #else
    #error Define what do here
    #endif
}

void Client::syncIndustries(const CATCHCHALLENGER_TYPE_MAPID &mapId)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(public_and_private_informations.mapData.find(mapId)==public_and_private_informations.mapData.cend())
        return;
    const Player_private_and_public_informations_Map &mapData=public_and_private_informations.mapData.at(mapId);
    //industries blob: for each industry [last_update(8B LE), resources_count(1B), [item_id(2B LE) quantity(4B LE)]..., products_count(1B), [item_id(2B LE) quantity(4B LE)]...]
    uint32_t posOutput=0;
    for(const IndustryStatus &ind : mapData.industriesStatus)
    {
        const uint64_t last_update_le=htole64(ind.last_update);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&last_update_le,sizeof(uint64_t));
        posOutput+=8;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(ind.resources.size());
        posOutput+=1;
        for(const std::pair<const CATCHCHALLENGER_TYPE_ITEM,uint32_t> &res : ind.resources)
        {
            const uint16_t item_id_le=htole16(res.first);
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&item_id_le,sizeof(uint16_t));
            posOutput+=2;
            const uint32_t quantity_le=htole32(res.second);
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&quantity_le,sizeof(uint32_t));
            posOutput+=4;
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(ind.products.size());
        posOutput+=1;
        for(const std::pair<const CATCHCHALLENGER_TYPE_ITEM,uint32_t> &prod : ind.products)
        {
            const uint16_t item_id_le=htole16(prod.first);
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&item_id_le,sizeof(uint16_t));
            posOutput+=2;
            const uint32_t quantity_le=htole32(prod.second);
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&quantity_le,sizeof(uint32_t));
            posOutput+=4;
        }
    }
    const uint32_t map_database_id=MapVisibilityAlgorithm::flat_map_list.at(mapId).id_db;
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_industries.asyncWrite({
                binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput),
                std::to_string(character_id_db),
                std::to_string(map_database_id)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)mapId;
    #elif CATCHCHALLENGER_DB_FILE
    (void)mapId;
    #else
    #error Define what do here
    #endif
}

void Client::savePosition()
{
    //virtual stop the player
    //Orientation orientation;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput(
                "map->map_file: "+
                map->map_file+
                ",x: "+std::to_string(x)+
                ",y: "+std::to_string(y)+
                ", orientation: "+std::to_string((int)last_direction)
                );
    #endif
    std::cerr << "[savePosition] character=" << character_id_db
              << " mapIndex=" << getMapId()
              << " x=" << std::to_string(x) << " y=" << std::to_string(y)
              << " last_direction=" << std::to_string(getLastDirection()) << std::endl;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
    const MapVisibilityAlgorithm &tmap=MapVisibilityAlgorithm::flat_map_list.at(getMapId());
    const MapVisibilityAlgorithm &trescue_map=MapVisibilityAlgorithm::flat_map_list.at(rescue.mapIndex);
    const MapVisibilityAlgorithm &tunvalidated_rescue_map=MapVisibilityAlgorithm::flat_map_list.at(unvalidated_rescue.mapIndex);
    const uint32_t &map_file_database_id=tmap.id_db;
    const uint32_t &rescue_map_file_database_id=trescue_map.id_db;
    const uint32_t &unvalidated_rescue_map_file_database_id=tunvalidated_rescue_map.id_db;
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_character_forserver_map.asyncWrite({
                std::to_string(map_file_database_id),
                std::to_string(x),
                std::to_string(y),
                //force into orientation because it can be kicked while move in progress
                directionToStringToSave(getLastDirection()),
                std::to_string(rescue_map_file_database_id),
                std::to_string(rescue.x),
                std::to_string(rescue.y),
                std::to_string((uint8_t)rescue.orientation),
                std::to_string(unvalidated_rescue_map_file_database_id),
                std::to_string(unvalidated_rescue.x),
                std::to_string(unvalidated_rescue.y),
                std::to_string((uint8_t)unvalidated_rescue.orientation),
                std::to_string(character_id_db)
                });
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
/* do at moveDownMonster and moveUpMonster
 *     const std::vector<PlayerMonster> &playerMonsterList=getPlayerMonster();
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(playerMonster.id,index+1);
        index++;
    }*/
}
