#include "../base/GlobalServerData.hpp"
#include "../base/Client.hpp"
#include "../base/Client.hpp"
#include "../base/PreparedDBQuery.hpp"

using namespace CatchChallenger;

void Client::wildDrop(const uint16_t &monster)
{
    std::vector<MonsterDrops> drops=GlobalServerData::serverPrivateVariables.monsterDrops.at(monster);
    if(questsDrop.find(monster)!=questsDrop.cend())
        drops.insert(drops.end(),questsDrop.at(monster).begin(),questsDrop.at(monster).end());
    unsigned int index=0;
    bool success;
    uint32_t quantity;
    while(index<drops.size())
    {
        const uint32_t &tempLuck=drops.at(index).luck;
        const uint32_t &quantity_min=drops.at(index).quantity_min;
        const uint32_t &quantity_max=drops.at(index).quantity_max;
        if(tempLuck==100)
            success=true;
        else
        {
            if(rand()%100<(int32_t)tempLuck)
                success=true;
            else
                success=false;
        }
        if(success)
        {
            if(quantity_max==1)
                quantity=1;
            else if(quantity_min==quantity_max)
                quantity=quantity_min;
            else
                quantity=rand()%(quantity_max-quantity_min+1)+quantity_min;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            normalOutput("Win "+std::to_string(quantity)+" item: "+std::to_string(drops.at(index).item));
            #endif
            addObjectAndSend(drops.at(index).item,quantity);
        }
        index++;
    }
}

uint32_t Client::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    int position=999999;
    bool ok;
    #ifndef CATCHCHALLENGER_DB_FILE
    const uint32_t monster_id=getMonsterId(&ok);
    #else
    const uint32_t monster_id=9999;
    #endif
    if(!ok)
    {
        errorFightEngine("No more monster id: getMonsterId(&ok) failed");
        return 0;
    }

    char raw_skill_endurance[newMonster.skills.size()*(1)];
    char raw_skill[newMonster.skills.size()*(2+1)];
    {
        unsigned int sub_index=0;
        uint16_t lastSkillId=0;
        while(sub_index<newMonster.skills.size())
        {
            const PlayerMonster::PlayerSkill &playerSkill=newMonster.skills.at(sub_index);
            raw_skill_endurance[sub_index]=playerSkill.endurance;

            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint16_t skillInt;
            if(lastSkillId<=playerSkill.skill)
            {
                skillInt=playerSkill.skill-lastSkillId;
                lastSkillId=playerSkill.skill;
            }
            else
            {
                skillInt=static_cast<uint16_t>(65536-static_cast<uint32_t>(lastSkillId)+static_cast<uint32_t>(playerSkill.skill));
                lastSkillId=playerSkill.skill;
            }
            #else
            //ordened
            const uint16_t &skillInt=playerSkill.skill-lastSkillId;
            lastSkillId=playerSkill.skill;
            #endif

            *reinterpret_cast<uint16_t *>(raw_skill+sub_index*(2+1))=htole16(skillInt);
            raw_skill[2+sub_index*(2+1)]=playerSkill.level;

            sub_index++;
        }
    }

    char raw_buff[(1+1+1)*newMonster.buffs.size()];
    {
        uint8_t lastBuffId=0;
        uint32_t sub_index=0;
        while(sub_index<newMonster.buffs.size())
        {
            const PlayerBuff &buff=newMonster.buffs.at(sub_index);

            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            //not ordened
            uint8_t buffInt;
            if(lastBuffId<=buff.buff)
            {
                buffInt=buff.buff-lastBuffId;
                lastBuffId=buff.buff;
            }
            else
            {
                buffInt=static_cast<uint8_t>(256-static_cast<uint16_t>(lastBuffId)+static_cast<uint16_t>(buff.buff));
                lastBuffId=buff.buff;
            }
            #else
            //ordened
            const uint8_t &buffInt=buff.buff-lastBuffId;
            lastBuffId=buff.buff;
            #endif

            raw_buff[sub_index*3+0]=buffInt;
            raw_buff[sub_index*3+1]=buff.level;
            raw_buff[sub_index*3+2]=buff.remainingNumberOfTurn;
            sub_index++;
        }
    }

    if(toStorage)
    {
        public_and_private_informations.warehouse_playerMonster.push_back(newMonster);
        public_and_private_informations.warehouse_playerMonster.back().id=monster_id;
        position=static_cast<int>(public_and_private_informations.warehouse_playerMonster.size());
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_full.asyncWrite({
                    std::to_string(monster_id),
                    std::to_string(character_id),
                    "2",
                    std::to_string(newMonster.hp),
                    std::to_string(newMonster.monster),
                    std::to_string(newMonster.level),
                    std::to_string(newMonster.catched_with),
                    std::to_string((uint8_t)newMonster.gender),
                    std::to_string(character_id),
                    std::to_string(position),
                    binarytoHexa(raw_buff,static_cast<uint32_t>(sizeof(raw_buff))),
                    binarytoHexa(raw_skill,static_cast<uint32_t>(sizeof(raw_skill))),
                    binarytoHexa(raw_skill_endurance,static_cast<uint32_t>(sizeof(raw_skill_endurance)))
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        #if defined(CATCHCHALLENGER_EXTRA_CHECK)
        if(sizeof(raw_skill)==0)
        {
            std::cerr << "skills='\\x' when have skills to save" << std::endl;
            abort();
        }
        #endif
    }
    else
    {
        public_and_private_informations.playerMonster.push_back(newMonster);
        public_and_private_informations.playerMonster.back().id=monster_id;
        position=static_cast<int>(public_and_private_informations.playerMonster.size());
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_full.asyncWrite({
                    std::to_string(monster_id),
                    std::to_string(character_id),
                    "1",
                    std::to_string(newMonster.hp),
                    std::to_string(newMonster.monster),
                    std::to_string(newMonster.level),
                    std::to_string(newMonster.catched_with),
                    std::to_string((uint8_t)newMonster.gender),
                    std::to_string(character_id),
                    std::to_string(position),
                    binarytoHexa(raw_buff,static_cast<uint32_t>(sizeof(raw_buff))),
                    binarytoHexa(raw_skill,static_cast<uint32_t>(sizeof(raw_skill))),
                    binarytoHexa(raw_skill_endurance,static_cast<uint32_t>(sizeof(raw_skill_endurance)))
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        #if defined(CATCHCHALLENGER_EXTRA_CHECK)
        if(sizeof(raw_skill)==0)
        {
            std::cerr << "skills='\\x' when have skills to save" << std::endl;
            abort();
        }
        #endif
    }

    /*unsigned int index=0;
    while(index<newMonster.skills.size())
    {
        std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_skill;
        stringreplaceOne(queryText,"%1",std::to_string(monster_id));
        stringreplaceOne(queryText,"%2",std::to_string(newMonster.skills.at(index).skill));
        stringreplaceOne(queryText,"%3",std::to_string(newMonster.skills.at(index).level));
        stringreplaceOne(queryText,"%4",std::to_string(newMonster.skills.at(index).endurance));
        dbQueryWriteCommon(queryText);
        index++;
    }
    index=0;
    while(index<newMonster.buffs.size())
    {
        if(CommonDatapack::commonDatapack.monsterBuffs.at(newMonster.buffs.at(index).buff).level.at(newMonster.buffs.at(index).level).duration==Buff::Duration_Always)
        {
            std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_buff;
            stringreplaceOne(queryText,"%1",std::to_string(monster_id));
            stringreplaceOne(queryText,"%2",std::to_string(newMonster.buffs.at(index).buff));
            stringreplaceOne(queryText,"%3",std::to_string(newMonster.buffs.at(index).level));
            dbQueryWriteCommon(queryText);
        }
        index++;
    }*/
    wildMonsters.erase(wildMonsters.begin());
    return monster_id;
}
