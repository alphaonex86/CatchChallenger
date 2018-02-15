#include "Client.h"

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLib.h"
#include "PreparedDBQuery.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::updateObjectInDatabase()
{
    if(public_and_private_informations.items.empty())
    {
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item.asyncWrite({
            std::string(),
            std::to_string(character_id)
            });
    }
    else
    {
        uint16_t lastItemId=0;
        uint32_t pos=0;
        char item_raw[(2+4)*public_and_private_informations.items.size()];
        auto i=public_and_private_informations.items.begin();
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
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item.asyncWrite({
            binarytoHexa(item_raw,static_cast<uint32_t>(sizeof(item_raw))),
            std::to_string(character_id)
            });
    }
}

/*void Client::updateMonsterInDatabase()
{
    if(public_and_private_informations.playerMonster.empty())
    {
        dbQueryWriteCommon(
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster.compose(
                        std::string(),
                    std::to_string(character_id)
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
                    std::to_string(character_id)
                        )
                    );
    }
}*/

void Client::updateObjectInWarehouseDatabase()
{
    if(public_and_private_informations.items.empty())
    {
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item_warehouse.asyncWrite({
            std::string(),
            std::to_string(character_id)
            });
    }
    else
    {
        uint16_t lastItemId=0;
        uint32_t pos=0;
        char item_raw[(2+4)*public_and_private_informations.warehouse_items.size()];
        auto i=public_and_private_informations.warehouse_items.begin();
        while(i!=public_and_private_informations.warehouse_items.cend())
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
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item_warehouse.asyncWrite({
            binarytoHexa(item_raw,static_cast<uint32_t>(sizeof(item_raw))),
            std::to_string(character_id)
            });
    }
}

/*void Client::updateMonsterInWarehouseDatabase()
{
    if(public_and_private_informations.warehouse_playerMonster.empty())
    {
        dbQueryWriteCommon(
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster_warehouse.compose(
                        std::string()
                        )
                    );
    }
    else
    {
        std::string monster;
        uint32_t pos=0;
        char monster_raw[(2+4)*public_and_private_informations.warehouse_playerMonster.size()];
        unsigned int index=0;
        while(index<public_and_private_informations.warehouse_playerMonster.size())
        {
            *reinterpret_cast<uint32_t *>(monster_raw+pos)=htole32(public_and_private_informations.warehouse_playerMonster.at(index).id);
            pos+=4;
            index++;
        }
        monster=binarytoHexa(monster_raw,sizeof(monster_raw));
        dbQueryWriteCommon(
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster_warehouse.compose(
                        binarytoHexa(monster_raw,sizeof(monster_raw)),
                    std::to_string(character_id)
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
        auto i=public_and_private_informations.items.begin();
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
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_item_and_encyclopedia.asyncWrite({
        item,
        binarytoHexa(public_and_private_informations.encyclopedia_item,
        static_cast<uint32_t>(CommonDatapack::commonDatapack.items.item.size())/8+1),
        std::to_string(character_id)
        });
}

void Client::updateMonsterInDatabaseEncyclopedia()
{
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_monster_encyclopedia.asyncWrite({
        binarytoHexa(public_and_private_informations.encyclopedia_monster,
        static_cast<uint32_t>(CommonDatapack::commonDatapack.monsters.size())/8+1),
        std::to_string(character_id)
        });
}

void Client::syncDatabaseAllow()
{
    uint8_t allowflat[public_and_private_informations.allow.size()];
    uint32_t index=0;
    auto i=public_and_private_informations.allow.begin();
    while(i!=public_and_private_informations.allow.cend())
    {
        allowflat[index]=*i;
        index++;
        ++i;
    }
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_allow.asyncWrite({
                binarytoHexa(allowflat,static_cast<uint32_t>(sizeof(allowflat))),
                std::to_string(character_id)
                });
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
    auto i=public_and_private_informations.reputation.begin();
    while(i!=public_and_private_informations.reputation.cend())
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(i->first>=CommonDatapack::commonDatapack.reputation.size())
        {
            std::cerr << "public_and_private_informations.reputation internal id is out of range to save: " << i->first << std::endl;
            abort();
        }
        #endif
        const uint8_t &databaseType=static_cast<uint8_t>(CommonDatapack::commonDatapack.reputation.at(i->first).reverse_database_id);
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
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_reputations.asyncWrite({
                binarytoHexa(buffer,posOutput),
                std::to_string(character_id)
                });

    if(public_and_private_informations.reputation.size()*(4+1+1)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        delete buffer;
}

void Client::syncBotAlreadyBeaten()
{
    if(public_and_private_informations.bot_already_beaten==NULL)
    {
        std::cerr << "bot_already_beaten==NULL at syncBotAlreadyBeaten()" << std::endl;
        return;
    }
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_character_bot_already_beaten.asyncWrite({
                binarytoHexa(public_and_private_informations.bot_already_beaten,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1),
                std::to_string(character_id)
                });
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
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
    const uint32_t &map_file_database_id=static_cast<MapServer *>(map)->reverse_db_id;
    const uint32_t &rescue_map_file_database_id=static_cast<MapServer *>(rescue.map)->reverse_db_id;
    const uint32_t &unvalidated_rescue_map_file_database_id=static_cast<MapServer *>(unvalidated_rescue.map)->reverse_db_id;
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
                std::to_string(character_id)
                });
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
