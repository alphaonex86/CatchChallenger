#include "Client.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "PreparedDBQuery.hpp"
#include "GlobalServerData.hpp"
#include "MapServer.hpp"

using namespace CatchChallenger;

void Client::addWarehouseObject(const uint16_t &item,const uint32_t &quantity,const bool databaseSync)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        public_and_private_informations.warehouse_items[item]+=quantity;
        if(databaseSync)
            updateObjectInWarehouseDatabase();
    }
    else
    {
        public_and_private_informations.warehouse_items[item]=quantity;
        if(databaseSync)
            updateObjectInWarehouseDatabase();
    }
}

uint32_t Client::removeWarehouseObject(const uint16_t &item,const uint32_t &quantity,const bool databaseSync)
{
    if(public_and_private_informations.warehouse_items.find(item)!=public_and_private_informations.warehouse_items.cend())
    {
        if(public_and_private_informations.warehouse_items.at(item)>quantity)
        {
            public_and_private_informations.warehouse_items[item]-=quantity;
            if(databaseSync)
                updateObjectInWarehouseDatabase();
            return quantity;
        }
        else
        {
            const uint32_t removed_quantity=public_and_private_informations.warehouse_items.at(item);
            public_and_private_informations.warehouse_items.erase(item);
            if(databaseSync)
                updateObjectInWarehouseDatabase();
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::addWarehouseCash(const uint64_t &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.warehouse_cash+=cash;
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_warehouse_cash.asyncWrite({
                std::to_string(public_and_private_informations.warehouse_cash),
                std::to_string(character_id)
                });
}

void Client::removeWarehouseCash(const uint64_t &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.warehouse_cash-=cash;
    public_and_private_informations.warehouse_cash+=cash;
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_warehouse_cash.asyncWrite({
                std::to_string(public_and_private_informations.warehouse_cash),
                std::to_string(character_id)
                });
}

void Client::wareHouseStore(const uint64_t &withdrawCash, const uint64_t &depositeCash,
                            const std::vector<std::pair<uint16_t,uint32_t> > &withdrawItems, const std::vector<std::pair<uint16_t,uint32_t> > &depositeItems,
                            const std::vector<uint8_t> &withdrawMonsters, const std::vector<uint8_t> &depositeMonsters)
{
    if(!wareHouseStoreCheck(withdrawCash,depositeCash,withdrawItems,depositeItems,withdrawMonsters,depositeMonsters))
        return;
    {
        unsigned int index=0;
        while(index<withdrawItems.size())
        {
            const std::pair<uint16_t, uint32_t> &item=withdrawItems.at(index);
            removeWarehouseObject(item.first,item.second);
            addObject(item.first,item.second);
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<withdrawItems.size())
        {
            const std::pair<uint16_t, uint32_t> &item=withdrawItems.at(index);
            removeObject(item.first,-item.second);
            addWarehouseObject(item.first,-item.second);
            index++;
        }
    }
    //validate the change here
    if(withdrawCash>0)
    {
        removeWarehouseCash(withdrawCash);
        addCash(withdrawCash);
    }
    else if(depositeCash>0)
    {
        removeCash(depositeCash);
        addWarehouseCash(depositeCash);
    }
    std::vector<PlayerMonster> playerMonster=public_and_private_informations.playerMonster;
    std::vector<PlayerMonster> warehouse_playerMonster=public_and_private_informations.warehouse_playerMonster;
    uint8_t lowerMonsterPos=playerMonster.size();
    uint8_t lowerWarehouseMonsterPos=warehouse_playerMonster.size();
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            const uint8_t &monsterPos=withdrawMonsters.at(index);
            if(lowerWarehouseMonsterPos>monsterPos)
                lowerWarehouseMonsterPos=monsterPos;
            const PlayerMonster &monster=public_and_private_informations.warehouse_playerMonster.at(monsterPos);
            playerMonster.push_back(monster);
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            const uint8_t &monsterPos=depositeMonsters.at(index);
            if(lowerMonsterPos>monsterPos)
                lowerMonsterPos=monsterPos;
            const PlayerMonster &monster=public_and_private_informations.playerMonster.at(monsterPos);
            warehouse_playerMonster.push_back(monster);
            index++;
        }
    }
    public_and_private_informations.playerMonster=playerMonster;
    public_and_private_informations.warehouse_playerMonster=warehouse_playerMonster;
    if(depositeMonsters.size()>0 || withdrawMonsters.size()>0)
    {
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
            saveMonsterStat(public_and_private_informations.playerMonster.back());
        {
            unsigned int index=lowerMonsterPos;
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
                GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position_and_place.asyncWrite({std::to_string(playerMonster.size()),"1",std::to_string(monster.id)});
                index++;
            }
        }
        {
            unsigned int index=lowerWarehouseMonsterPos;
            while(index<public_and_private_informations.warehouse_playerMonster.size())
            {
                const PlayerMonster &monster=public_and_private_informations.warehouse_playerMonster.at(index);
                GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position_and_place.asyncWrite({std::to_string(warehouse_playerMonster.size()),"2",std::to_string(monster.id)});
                index++;
            }
        }
    }
    if(!withdrawItems.empty() || !depositeItems.empty())
    {
        updateObjectInDatabase();//do above in bad way, improve this
        updateObjectInWarehouseDatabase();//do above in bad way, improve this
    }
}

bool Client::wareHouseStoreCheck(const uint64_t &withdrawCash, const uint64_t &depositeCash, const std::vector<std::pair<uint16_t, uint32_t> > &withdrawItems, const std::vector<std::pair<uint16_t, uint32_t> > &depositeItems, const std::vector<uint8_t> &withdrawMonsters, const std::vector<uint8_t> &depositeMonsters)
{
    //check all
    if((withdrawCash>0 && public_and_private_informations.warehouse_cash<withdrawCash))
    {
        errorOutput("cash transfer is wrong");
        return false;
    }
    if((depositeCash>0 && public_and_private_informations.cash<withdrawCash))
    {
        errorOutput("cash transfer is wrong");
        return false;
    }
    {
        unsigned int index=0;
        while(index<withdrawItems.size())
        {
            const std::pair<uint16_t, int32_t> &item=withdrawItems.at(index);
            if(public_and_private_informations.warehouse_items.find(item.first)==public_and_private_informations.warehouse_items.cend())
            {
                errorOutput("item transfer is wrong due to missing");
                return false;
            }
            if((int64_t)public_and_private_informations.warehouse_items.at(item.first)<item.second)
            {
                errorOutput("item transfer is wrong due to wrong quantity");
                return false;
            }
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeItems.size())
        {
            const std::pair<uint16_t, int32_t> &item=depositeItems.at(index);
            if(public_and_private_informations.items.find(item.first)==public_and_private_informations.items.cend())
            {
                errorOutput("item transfer is wrong due to missing");
                return false;
            }
            if((int64_t)public_and_private_informations.items.at(item.first)<item.second)
            {
                errorOutput("item transfer is wrong due to wrong quantity");
                return false;
            }
            index++;
        }
    }
    std::unordered_set<uint8_t> alreadyMovedToWarehouse,alreadyMovedFromWarehouse;
    int count_change=0;
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            const uint8_t &monsterPos=withdrawMonsters.at(index);
            if(monsterPos>=public_and_private_informations.warehouse_playerMonster.size())
            {
                errorOutput("withdrawMonsters pos out of bound");
                return false;
            }
            if(alreadyMovedFromWarehouse.find(monsterPos)!=alreadyMovedFromWarehouse.cend())
            {
                errorOutput("withdrawMonsters pos already moved");
                return false;
            }
            alreadyMovedFromWarehouse.insert(monsterPos);
            count_change++;
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            const uint8_t &monsterPos=depositeMonsters.at(index);
            if(monsterPos>=public_and_private_informations.playerMonster.size())
            {
                errorOutput("withdrawMonsters pos out of bound");
                return false;
            }
            if(alreadyMovedToWarehouse.find(monsterPos)!=alreadyMovedToWarehouse.cend())
            {
                errorOutput("withdrawMonsters pos already moved");
                return false;
            }
            alreadyMovedToWarehouse.insert(monsterPos);
            count_change++;
            index++;
        }
    }
    if((public_and_private_informations.playerMonster.size()+count_change)>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        errorOutput("have more monster to withdraw than the allowed");
        return false;
    }
    return true;
}
