#include "../VariableServer.h"
#include "../base/GlobalServerData.h"
#include "../base/MapServer.h"
#include "../base/Client.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "../base/Client.h"
#include "../base/PreparedDBQuery.h"

using namespace CatchChallenger;

void Client::syncMonsterBuff(const PlayerMonster &monster)
{
    char raw_buff[(1+1+1)*monster.buffs.size()];
    uint8_t lastBuffId=0;
    uint32_t sub_index=0;
    while(sub_index<monster.buffs.size())
    {
        const PlayerBuff &buff=monster.buffs.at(sub_index);

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
            buffInt=256-lastBuffId+buff.buff;
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
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_buff.compose(
                binarytoHexa(raw_buff,sizeof(raw_buff)),
                std::to_string(monster.id)
                );
    #if defined(CATCHCHALLENGER_EXTRA_CHECK) && defined(CATCHCHALLENGER_DB_POSTGRESQL)
    if(!monster.buffs.empty() && queryText.find("buffs='\\x'")!=std::string::npos)
    {
        std::cerr << "buffs='\\x' when have buff to save" << std::endl;
        abort();
    }
    #endif
    dbQueryWriteCommon(queryText);
}

void Client::syncMonsterSkillAndEndurance(const PlayerMonster &monster)
{
    if(monster.skills.empty())
    {
        normalOutput("Internal error: try sync skill when don't have skill");
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return;
    }
    char skills_endurance[monster.skills.size()*(1)];
    char skills[monster.skills.size()*(2+1)];
    unsigned int sub_index=0;
    uint16_t lastSkillId=0;
    const unsigned int &sub_size=monster.skills.size();
    while(sub_index<sub_size)
    {
        const PlayerMonster::PlayerSkill &playerSkill=monster.skills.at(sub_index);
        skills_endurance[sub_index]=playerSkill.endurance;

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
            skillInt=65536-lastSkillId+playerSkill.skill;
            lastSkillId=playerSkill.skill;
        }
        #else
        //ordened
        const uint16_t &skillInt=playerSkill.skill-lastSkillId;
        lastSkillId=playerSkill.skill;
        #endif

        *reinterpret_cast<uint16_t *>(skills+sub_index*(2+1))=htole16(skillInt);
        skills[2+sub_index*(2+1)]=playerSkill.level;

        sub_index++;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_monster_update_skill_and_endurance.compose(
                binarytoHexa(skills,sizeof(skills)),
                binarytoHexa(skills_endurance,sizeof(skills_endurance)),
                std::to_string(monster.id)
                );
    #if defined(CATCHCHALLENGER_EXTRA_CHECK) && defined(CATCHCHALLENGER_DB_POSTGRESQL)
    if(queryText.find("skills='\\x'")!=std::string::npos)
    {
        std::cerr << "skills='\\x' when have skills to save" << std::endl;
        abort();
    }
    #endif
    dbQueryWriteCommon(queryText);
}

void Client::syncMonsterEndurance(const PlayerMonster &monster)
{
    if(monster.skills.empty())
    {
        normalOutput("Internal error: try sync skill when don't have skill");
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return;
    }
    char skills_endurance[monster.skills.size()*(1)];
    unsigned int sub_index=0;
    const unsigned int &sub_size=monster.skills.size();
    while(sub_index<sub_size)
    {
        const PlayerMonster::PlayerSkill &playerSkill=monster.skills.at(sub_index);
        skills_endurance[sub_index]=playerSkill.endurance;

        sub_index++;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_monster_update_endurance.compose(
                binarytoHexa(skills_endurance,sizeof(skills_endurance)),
                std::to_string(monster.id)
                );
    #if defined(CATCHCHALLENGER_EXTRA_CHECK) && defined(CATCHCHALLENGER_DB_POSTGRESQL)
    if(queryText.find("skills_endurance='\\x'")!=std::string::npos)
    {
        std::cerr << "skills_endurance='\\x' when have skills_endurance to save" << std::endl;
        abort();
    }
    #endif
    dbQueryWriteCommon(queryText);
}

void Client::saveAllMonsterPosition()
{
    const std::vector<PlayerMonster> &playerMonsterList=getPlayerMonster();
    unsigned int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(playerMonster.id,index+1);
        index++;
    }
}

void Client::syncForEndOfTurn()
{
    if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
        saveStat();
}

void Client::saveStat()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        PublicPlayerMonster * otherMonster=getOtherMonster();
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster!=NULL)
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorOutput("saveStat() The hp "+std::to_string(currentMonster->hp)+
                            " of current monster "+std::to_string(currentMonster->monster)+
                            " is greater than the max "+std::to_string(currentMonsterStat.hp)
                            );
                return;
            }
        if(otherMonster!=NULL)
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorOutput("saveStat() The hp "+std::to_string(otherMonster->hp)+
                            " of other monster "+std::to_string(otherMonster->monster)+
                            " is greater than the max "+std::to_string(otherMonsterStat.hp)
                            );
                return;
            }
    }
    #endif
    const PlayerMonster * const monster=getCurrentMonster();
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_monster_hp_only.compose(
                std::to_string(monster->hp),
                std::to_string(monster->id)
                );
    dbQueryWriteCommon(queryText);
}
