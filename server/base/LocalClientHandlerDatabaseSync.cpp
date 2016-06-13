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
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_item.compose(
                        std::string(),
                        std::to_string(character_id)
                        )
                    );
    }
    else
    {
        uint32_t lastItemId=0;
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
                item=65536-lastItemId+i->first;
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
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_item.compose(
                        binarytoHexa(item_raw,sizeof(item_raw)),
                        std::to_string(character_id)
                        )
                    );
    }
}

/*void Client::updateMonsterInDatabase()
{
    if(public_and_private_informations.playerMonster.empty())
    {
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_monster.compose(
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
                    PreparedDBQueryCommon::db_query_update_character_monster.compose(
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
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_item_warehouse.compose(
                        std::string(),
                        std::to_string(character_id)
                        )
                    );
    }
    else
    {
        uint32_t lastItemId=0;
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
                item=65536-lastItemId+i->first;
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
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_item_warehouse.compose(
                        binarytoHexa(item_raw,sizeof(item_raw)),
                        std::to_string(character_id)
                        )
                    );
    }
}

/*void Client::updateMonsterInWarehouseDatabase()
{
    if(public_and_private_informations.warehouse_playerMonster.empty())
    {
        dbQueryWriteCommon(
                    PreparedDBQueryCommon::db_query_update_character_monster_warehouse.compose(
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
                    PreparedDBQueryCommon::db_query_update_character_monster_warehouse.compose(
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
        uint32_t lastItemId=0;
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
                item=65536-lastItemId+i->first;
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
        item=binarytoHexa(item_raw,sizeof(item_raw));
    }
    dbQueryWriteCommon(
                PreparedDBQueryCommon::db_query_update_character_item_and_encyclopedia.compose(
                    item,
                    binarytoHexa(public_and_private_informations.encyclopedia_item,CommonDatapack::commonDatapack.items.item.size()/8+1),
                    std::to_string(character_id)
                    )
                );
}

void Client::updateMonsterInDatabaseEncyclopedia()
{
    dbQueryWriteCommon(
                PreparedDBQueryCommon::db_query_update_character_monster_encyclopedia.compose(
                    binarytoHexa(public_and_private_informations.encyclopedia_monster,CommonDatapack::commonDatapack.monsters.size()/8+1),
                    std::to_string(character_id)
                    )
                );
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
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_allow.compose(
                binarytoHexa(allowflat,sizeof(allowflat)),
                std::to_string(character_id)
                );
    dbQueryWriteCommon(queryText);
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
        #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
        //not ordened
        uint8_t type;
        if(lastReputationId<=i->first)
        {
            type=i->first-lastReputationId;
            lastReputationId=i->first;
        }
        else
        {
            type=256-lastReputationId+i->first;
            lastReputationId=i->first;
        }
        #else
        //ordened
        const uint8_t &type=i->first-lastReputationId;
        lastReputationId=i->first;
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
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_reputations.compose(
                binarytoHexa(buffer,posOutput),
                std::to_string(character_id)
                );
    dbQueryWriteCommon(queryText);

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
    const std::string &queryText=PreparedDBQueryServer::db_query_update_character_bot_already_beaten.compose(
                binarytoHexa(public_and_private_informations.bot_already_beaten,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1),
                std::to_string(character_id)
                );
    dbQueryWriteServer(queryText);
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
    const std::string &updateMapPositionQuery=PreparedDBQueryServer::db_query_update_character_forserver_map.compose(
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
                );
    dbQueryWriteServer(updateMapPositionQuery);
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
