#include "../base/Client.h"
#include "../base/PreparedDBQuery.h"
#include "../base/GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/cpp11addition.h"
#include "../base/DatabaseFunction.h"

using namespace CatchChallenger;

void Client::loadMonsters()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_monsters_by_player_id.empty())
    {
        errorOutput("loadMonsters() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_monsters_by_player_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadMonsters_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadReputation();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadMonsters_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadMonsters_return();
}

void Client::loadMonsters_return()
{
    callbackRegistred.pop();
    bool ok;
    Monster monster;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        monster.give_sp=0;
        PlayerMonster playerMonster;
        playerMonster.id=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
            normalOutput("monsterId: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
        if(ok)
        {
            playerMonster.monster=DatabaseFunction::stringtouint16(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.monsters.find(playerMonster.monster)==CommonDatapack::commonDatapack.monsters.cend())
                {
                    ok=false;
                    normalOutput("monster: "+std::to_string(playerMonster.monster)+" is not into monster list");
                }
                else
                    monster=CommonDatapack::commonDatapack.monsters.at(playerMonster.monster);
            }
            else
                normalOutput("monster: "+GlobalServerData::serverPrivateVariables.db_common->value(2)+" is not a number");
        }
        if(ok)
        {
            playerMonster.level=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    normalOutput("level: "+std::to_string(playerMonster.level)+" greater than "+std::to_string(CATCHCHALLENGER_MONSTER_LEVEL_MAX)+", truncated");
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                normalOutput("level: "+GlobalServerData::serverPrivateVariables.db_common->value(3)+" is not a number");
        }
        if(ok)
        {
            playerMonster.remaining_xp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
            if(ok)
            {
                if(playerMonster.level>monster.level_to_xp.size())
                {
                    normalOutput("monster level: "+std::to_string(playerMonster.level)+" greater than loaded level "+std::to_string(monster.level_to_xp.size()));
                    ok=false;
                }
                else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
                {
                    normalOutput("monster xp: "+std::to_string(playerMonster.remaining_xp)+" greater than "+std::to_string(monster.level_to_xp.at(playerMonster.level-1))+", truncated");
                    playerMonster.remaining_xp=0;
                }
            }
            else
                normalOutput("monster xp: "+GlobalServerData::serverPrivateVariables.db_common->value(4)+" is not a number");
        }
        if(ok)
        {
            if(CommonSettingsServer::commonSettingsServer.useSP)
            {
                playerMonster.sp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
                if(!ok)
                    normalOutput("monster sp: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" is not a number");
            }
            else
                playerMonster.sp=0;
        }
        int sp_offset;
        if(CommonSettingsServer::commonSettingsServer.useSP)
            sp_offset=0;
        else
            sp_offset=-1;
        if(ok)
        {
            playerMonster.catched_with=DatabaseFunction::stringtouint16(GlobalServerData::serverPrivateVariables.db_common->value(6+sp_offset),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.items.item.find(playerMonster.catched_with)==CommonDatapack::commonDatapack.items.item.cend())
                    normalOutput("captured_with: "+std::to_string(playerMonster.catched_with)+" is not is not into items list");
            }
            else
                normalOutput("captured_with: "+GlobalServerData::serverPrivateVariables.db_common->value(6+sp_offset)+" is not a number");
        }
        if(ok)
        {
            const uint32_t &genderInt=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset),&ok);
            if(ok)
            {
                if(genderInt>=1 && genderInt<=3)
                    playerMonster.gender=static_cast<Gender>(genderInt);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    normalOutput("unknown monster gender, out of range: "+GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                normalOutput("unknown monster gender: "+GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(8+sp_offset),&ok);
            if(!ok)
                normalOutput("monster egg_step: "+GlobalServerData::serverPrivateVariables.db_common->value(8+sp_offset)+" is not a number");
        }
        if(ok)
        {
            playerMonster.character_origin=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(9+sp_offset),&ok);
            if(!ok)
                normalOutput("monster character_origin: "+GlobalServerData::serverPrivateVariables.db_common->value(9+sp_offset)+" is not a number");
        }
        //stats
        if(ok)
        {
            playerMonster.hp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    normalOutput("monster hp: "+std::to_string(playerMonster.hp)+
                                 " greater than max hp "+std::to_string(stat.hp)+
                                 " for the level "+std::to_string(playerMonster.level)+
                                 " of the monster "+std::to_string(playerMonster.monster)+
                                 ", truncated");
                    playerMonster.hp=stat.hp;
                }
            }
            else
                normalOutput("monster hp: "+GlobalServerData::serverPrivateVariables.db_common->value(1)+" is not a number");
        }
        //finish it
        if(ok)
            public_and_private_informations.playerMonster.push_back(playerMonster);
    }
    loadMonstersWarehouse();
}

void Client::loadMonstersWarehouse()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id.empty())
    {
        errorOutput("loadMonsters() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_monsters_warehouse_by_player_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadMonstersWarehouse_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadReputation();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadMonstersWarehouse_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadMonstersWarehouse_return();
}

void Client::loadMonstersWarehouse_return()
{
    callbackRegistred.pop();
    bool ok;
    Monster monster;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        monster.give_sp=88889;
        PlayerMonster playerMonster;
        playerMonster.id=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
            normalOutput("monsterId: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
        if(ok)
        {
            playerMonster.monster=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.monsters.find(playerMonster.monster)==CommonDatapack::commonDatapack.monsters.cend())
                {
                    ok=false;
                    normalOutput("monster: "+std::to_string(playerMonster.monster)+" is not into monster list");
                }
                else
                    monster=CommonDatapack::commonDatapack.monsters.at(playerMonster.monster);
            }
            else
                normalOutput("monster: "+GlobalServerData::serverPrivateVariables.db_common->value(2)+" is not a number");
        }
        if(ok)
        {
            playerMonster.level=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    normalOutput("level: "+std::to_string(playerMonster.level)+" greater than "+std::to_string(CATCHCHALLENGER_MONSTER_LEVEL_MAX)+", truncated");
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                normalOutput("level: "+GlobalServerData::serverPrivateVariables.db_common->value(3)+" is not a number");
        }
        if(ok)
        {
            playerMonster.remaining_xp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
            if(ok)
            {
                if(playerMonster.level>monster.level_to_xp.size())
                {
                    normalOutput("monster level: "+std::to_string(playerMonster.level)+" greater than loaded level "+std::to_string(monster.level_to_xp.size()));
                    ok=false;
                }
                else if(playerMonster.remaining_xp>monster.level_to_xp.at(playerMonster.level-1))
                {
                    normalOutput("monster xp: "+std::to_string(playerMonster.remaining_xp)+" greater than "+std::to_string(monster.level_to_xp.at(playerMonster.level-1))+", truncated");
                    playerMonster.remaining_xp=0;
                }
            }
            else
            {
                playerMonster.remaining_xp=0;
                normalOutput("monster xp: "+GlobalServerData::serverPrivateVariables.db_common->value(4)+" is not a number");
            }
        }
        if(ok)
        {
            if(CommonSettingsServer::commonSettingsServer.useSP)
            {
                playerMonster.sp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
                if(!ok)
                {
                    playerMonster.sp=0;
                    normalOutput("monster sp: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" is not a number");
                }
            }
            else
                playerMonster.sp=0;
        }
        int sp_offset;
        if(CommonSettingsServer::commonSettingsServer.useSP)
            sp_offset=0;
        else
            sp_offset=-1;
        if(ok)
        {
            playerMonster.catched_with=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(6+sp_offset),&ok);
            if(ok)
            {
                if(CommonDatapack::commonDatapack.items.item.find(playerMonster.catched_with)==CommonDatapack::commonDatapack.items.item.cend())
                    normalOutput("captured_with: "+std::to_string(playerMonster.catched_with)+" is not is not into items list");
            }
            else
                normalOutput("captured_with: "+GlobalServerData::serverPrivateVariables.db_common->value(6+sp_offset)+" is not a number");
        }
        if(ok)
        {
            const uint32_t &genderInt=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset),&ok);
            if(ok)
            {
                if(genderInt>=1 && genderInt<=3)
                    playerMonster.gender=static_cast<Gender>(genderInt);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    normalOutput("unknown monster gender, out of range: "+GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                normalOutput("unknown monster gender: "+GlobalServerData::serverPrivateVariables.db_common->value(7+sp_offset));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(8+sp_offset),&ok);
            if(!ok)
                normalOutput("monster egg_step: "+GlobalServerData::serverPrivateVariables.db_common->value(8+sp_offset)+" is not a number");
        }
        if(ok)
        {
            playerMonster.character_origin=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(9+sp_offset),&ok);
            if(!ok)
                normalOutput("monster character_origin: "+GlobalServerData::serverPrivateVariables.db_common->value(9+sp_offset)+" is not a number");
        }
        //stats
        if(ok)
        {
            playerMonster.hp=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                const Monster::Stat &stat=CommonFightEngine::getStat(monster,playerMonster.level);
                if(playerMonster.hp>stat.hp)
                {
                    normalOutput("monster hp: "+std::to_string(playerMonster.hp)+
                                 " greater than max hp "+std::to_string(stat.hp)+
                                 " for the level "+std::to_string(playerMonster.level)+
                                 " of the monster "+std::to_string(playerMonster.monster)+
                                 ", truncated");
                    playerMonster.hp=stat.hp;
                }
            }
            else
                normalOutput("monster hp: "+GlobalServerData::serverPrivateVariables.db_common->value(1)+" is not a number");
        }
        //finish it
        if(ok)
            public_and_private_informations.warehouse_playerMonster.push_back(playerMonster);
    }
    if(!public_and_private_informations.playerMonster.empty())
        loadPlayerMonsterBuffs(0);
    else if(!public_and_private_informations.warehouse_playerMonster.empty())
        loadPlayerMonsterBuffs(0);
    else
        loadReputation();
}

void Client::loadPlayerMonsterBuffs(const uint32_t &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_monstersBuff_by_id.empty())
    {
        errorOutput("loadPlayerMonsterBuffs() Query is empty, bug");
        loadPlayerMonsterSkills(0);
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_monstersBuff_by_id;
    if(index<(uint32_t)public_and_private_informations.playerMonster.size())
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).id));
    else if(index<(uint32_t)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index-public_and_private_informations.playerMonster.size()).id));
    else
    {
        loadPlayerMonsterSkills(0);
        return;
    }
    if(!queryText.empty())
    {
        SelectIndexParam *selectIndexParam=new SelectIndexParam;
        selectIndexParam->index=index;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(index>(uint32_t)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
            abort();
        #endif

        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadPlayerMonsterBuffs_static);
        if(callback==NULL)
        {
            std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            delete selectIndexParam;
            loadPlayerMonsterSkills(0);
            return;
        }
        else
        {
            callbackRegistred.push(callback);
            paramToPassToCallBack.push(selectIndexParam);
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            paramToPassToCallBackType.push("SelectIndexParam");
            #endif
        }
    }
}

void Client::loadPlayerMonsterBuffs_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerMonsterBuffs_object();
}

void Client::loadPlayerMonsterBuffs_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectIndexParam *selectIndexParam=static_cast<SelectIndexParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectIndexParam==NULL)
        abort();
    #endif
    loadPlayerMonsterBuffs_return(selectIndexParam->index);
    delete selectIndexParam;
}

void Client::loadPlayerMonsterBuffs_return(const uint32_t &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="SelectIndexParam")
    {
        std::cerr << "is not SelectIndexParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>(uint32_t)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        abort();
    #endif
    callbackRegistred.pop();
    PlayerMonster *playerMonster;
    if(index<(uint32_t)public_and_private_informations.playerMonster.size())
        playerMonster=&public_and_private_informations.playerMonster[index];
    else
        playerMonster=&public_and_private_informations.warehouse_playerMonster[index-public_and_private_informations.playerMonster.size()];
    const uint32_t &monsterId=playerMonster->id;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerBuff buff;
        buff.buff=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.find(buff.buff)==CommonDatapack::commonDatapack.monsterBuffs.cend())
            {
                ok=false;
                normalOutput("buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(monsterId)+" is not found into buff list");
            }
        }
        else
            normalOutput("buff id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
        if(ok)
        {
            buff.level=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                if(buff.level>CommonDatapack::commonDatapack.monsterBuffs.at(buff.buff).level.size())
                {
                    ok=false;
                    normalOutput("buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(monsterId)+" have not the level: "+std::to_string(buff.level));
                }
            }
            else
                normalOutput("buff level: "+GlobalServerData::serverPrivateVariables.db_common->value(2)+" is not a number");
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.at(buff.buff).level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                normalOutput("buff "+std::to_string(buff.buff)+" for monsterId: "+std::to_string(monsterId)+" can't be loaded from the db if is not permanent");
            }
        }
        if(ok)
            playerMonster->buffs.push_back(buff);
    }
    loadPlayerMonsterBuffs(index+1);
}

void Client::loadPlayerMonsterSkills(const uint32_t &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_monstersSkill_by_id.empty())
    {
        errorOutput("loadMonsterSkills() Query is empty, bug");
        loadReputation();
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_monstersSkill_by_id;
    if(index<(uint32_t)public_and_private_informations.playerMonster.size())
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).id));
    else if(index<(uint32_t)(public_and_private_informations.playerMonster.size()+public_and_private_informations.warehouse_playerMonster.size()))
        stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index-public_and_private_informations.playerMonster.size()).id));
    else
    {
        loadReputation();
        return;
    }
    if(!queryText.empty())
    {
        SelectIndexParam *selectIndexParam=new SelectIndexParam;
        selectIndexParam->index=index;
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadPlayerMonsterSkills_static);
        if(callback==NULL)
        {
            std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            delete selectIndexParam;
            loadReputation();
            return;
        }
        else
        {
            callbackRegistred.push(callback);
            paramToPassToCallBack.push(selectIndexParam);
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            paramToPassToCallBackType.push("SelectIndexParam");
            #endif
        }
    }
}

void Client::loadPlayerMonsterSkills_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerMonsterSkills_object();
}

void Client::loadPlayerMonsterSkills_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectIndexParam *selectIndexParam=static_cast<SelectIndexParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectIndexParam==NULL)
        abort();
    #endif
    loadPlayerMonsterSkills_return(selectIndexParam->index);
    delete selectIndexParam;
}

void Client::loadPlayerMonsterSkills_return(const uint32_t &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="SelectIndexParam")
    {
        std::cerr << "is not SelectIndexParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();
    PlayerMonster *playerMonster;
    if(index<(uint32_t)public_and_private_informations.playerMonster.size())
        playerMonster=&public_and_private_informations.playerMonster[index];
    else
        playerMonster=&public_and_private_informations.warehouse_playerMonster[index-public_and_private_informations.playerMonster.size()];
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterSkills.find(skill.skill)==CommonDatapack::commonDatapack.monsterSkills.cend())
            {
                ok=false;
                normalOutput("skill "+std::to_string(skill.skill)+" for monsterId: "+std::to_string(playerMonster->id)+" is not found into skill list");
            }
        }
        else
            normalOutput("skill id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
        if(ok)
        {
            skill.level=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.size())
                {
                    ok=false;
                    normalOutput("skill "+std::to_string(skill.skill)+" for monsterId: "+std::to_string(playerMonster->id)+" have not the level: "+std::to_string(skill.level));
                }
            }
            else
                normalOutput("skill level: "+GlobalServerData::serverPrivateVariables.db_common->value(1)+" is not a number");
        }
        if(ok)
        {
            skill.endurance=DatabaseFunction::stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.at(skill.skill).level.at(skill.level-1).endurance;
                    normalOutput("skill "+std::to_string(skill.skill)+
                                 " for monsterId: "+std::to_string(playerMonster->id)+
                                 " have too hight endurance, lowered to: "+std::to_string(skill.endurance)
                                 );
                }
            }
            else
                normalOutput("skill endurance: "+GlobalServerData::serverPrivateVariables.db_common->value(2)+" is not a number");
        }
        if(ok)
            playerMonster->skills.push_back(skill);
    }
    loadPlayerMonsterSkills(index+1);
}

void Client::generateRandomNumber()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x53;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);//set the dynamic size

    if((randomIndex+randomSize)<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        //can send the next block
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,GlobalServerData::serverPrivateVariables.randomData.data()+randomIndex+randomSize,CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);
        posOutput+=CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //need return to the first block
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,GlobalServerData::serverPrivateVariables.randomData.data(),CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE);
        posOutput+=CATCHCHALLENGER_SERVER_RANDOM_LIST_SIZE;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    randomSize+=CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE;
}

uint32_t Client::randomSeedsSize() const
{
    return randomSize;
}

uint8_t Client::getOneSeed(const uint8_t &max)
{
    const uint8_t &number=GlobalServerData::serverPrivateVariables.randomData.at(randomIndex);
    randomIndex++;
    randomSize--;
    if(randomSize<CATCHCHALLENGER_SERVER_MIN_RANDOM_LIST_SIZE)
        generateRandomNumber();
    return number%(max+1);
}

void Client::loadBotAlreadyBeaten()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_bot_beaten.empty())
    {
        errorOutput("loadBotAlreadyBeaten() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_bot_beaten;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadBotAlreadyBeaten_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        loadItemOnMap();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadBotAlreadyBeaten_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadBotAlreadyBeaten_return();
}

void Client::loadBotAlreadyBeaten_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint32_t &id=DatabaseFunction::stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for quest, skip: "+std::to_string(id));
            continue;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(id)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
        {
            normalOutput("fights is not into the fights list, skip: "+std::to_string(id));
            continue;
        }
        public_and_private_informations.bot_already_beaten.insert(id);
    }
    loadItemOnMap();
}
