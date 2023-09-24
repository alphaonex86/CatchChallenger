#include "../base/GlobalServerData.hpp"
#include "../base/Client.hpp"
#include "../base/Client.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsServer.hpp"

using namespace CatchChallenger;

bool Client::learnSkillInternal(const uint8_t &monsterPosition,const uint16_t &skill)
{
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorOutput("The monster is not found: "+std::to_string(monsterPosition));
        return false;
    }
    unsigned int index=monsterPosition;
    const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
    unsigned int sub_index2=0;
    while(sub_index2<monster.skills.size())
    {
        if(monster.skills.at(sub_index2).skill==skill)
            break;
        sub_index2++;
    }
    unsigned int sub_index=0;
    while(sub_index<CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.size())
    {
        const Monster::AttackToLearn &learn=CommonDatapack::commonDatapack.get_monsters().at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
        {
            if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
            {
                if(CommonSettingsServer::commonSettingsServer.useSP)
                {
                    const Skill &skillStructure=CommonDatapack::commonDatapack.get_monsterSkills().at(learn.learnSkill);
                    const uint32_t &sp=skillStructure.level.at(learn.learnSkillLevel-1).sp_to_learn;
                    if(sp>monster.sp)
                    {
                        errorOutput("The attack require "+std::to_string(sp)+" sp to be learned, you have only "+std::to_string(monster.sp));
                        return false;
                    }
                    public_and_private_informations.playerMonster[index].sp-=sp;
                    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_sp_only.asyncWrite({
                                std::to_string(public_and_private_informations.playerMonster.at(index).sp),
                                std::to_string(public_and_private_informations.playerMonster.at(index).id)
                                });
                    #elif CATCHCHALLENGER_DB_BLACKHOLE
                    #elif CATCHCHALLENGER_DB_FILE
                    #else
                    #error Define what do here
                    #endif
                }
                if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=skill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(temp.skill).level.front().endurance;
                    public_and_private_informations.playerMonster[index].skills.push_back(temp);
                    /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_skill;
                    stringreplaceOne(queryText,"%1",std::to_string(monsterId));
                    stringreplaceOne(queryText,"%2",std::to_string(temp.skill));
                    stringreplaceOne(queryText,"%3","1");
                    stringreplaceOne(queryText,"%4",std::to_string(temp.endurance));
                    dbQueryWriteCommon(queryText);*/
                }
                else
                {
                    public_and_private_informations.playerMonster[index].skills[sub_index2].level++;
                    /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_skill_level;
                    stringreplaceOne(queryText,"%1",std::to_string(public_and_private_informations.playerMonster.at(index).skills.at(sub_index2).level));
                    stringreplaceOne(queryText,"%2",std::to_string(monsterId));
                    stringreplaceOne(queryText,"%3",std::to_string(skill));
                    dbQueryWriteCommon(queryText);*/
                }
                syncMonsterSkillAndEndurance(public_and_private_informations.playerMonster[index]);
                return true;
            }
        }
        sub_index++;
    }
    errorOutput("The skill "+std::to_string(skill)+" is not into learn skill list for the monster");
    return false;
}

bool Client::useSkill(const uint16_t &skill)
{
    normalOutput("use the skill: "+std::to_string(skill));
    if(!isInBattle())//wild or bot
    {
        if(!CommonFightEngine::useSkill(skill))
            return false;
        return finishTheTurn(!botFightMonsters.empty());
    }
    else
    {
        if(haveBattleAction())
        {
            errorOutput("Have already a battle action");
            return false;
        }
        mHaveCurrentSkill=true;
        mCurrentSkillId=skill;
        return checkIfCanDoTheTurn();
    }
}

bool Client::haveCurrentSkill() const
{
    return mHaveCurrentSkill;
}

uint16_t Client::getCurrentSkill() const
{
    return mCurrentSkillId;
}

uint8_t Client::decreaseSkillEndurance(PlayerMonster::PlayerSkill * skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorOutput("Unable to locate the current monster");
        return 0;
    }
    if(skill->endurance>0)
    {
        const uint8_t &newEndurance=CommonFightEngine::decreaseSkillEndurance(skill);
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtEachTurn)
        {
            /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_monster_skill;
            stringreplaceOne(queryText,"%1",std::to_string(newEndurance));
            stringreplaceOne(queryText,"%2",std::to_string(currentMonster->id));
            stringreplaceOne(queryText,"%3",std::to_string(skill));
            dbQueryWriteCommon(queryText);*/
            syncMonsterEndurance(*currentMonster);
        }
        else
        {
            if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle)
            {
                //deferedEnduranceSync[currentMonster][skill]=newEndurance;
                deferedEnduranceSync.insert(currentMonster);
            }
        }
        return newEndurance;
    }
    else
    {
        errorOutput("Try use skill without endurance: "+std::to_string(skill->skill));
        return 0;
    }
}

bool Client::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    if(!CommonFightEngine::addSkill(currentMonster,skill))
        return false;
    /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster_skill;
    stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%2",std::to_string(skill.skill));
    stringreplaceOne(queryText,"%3",std::to_string(skill.level));
    stringreplaceOne(queryText,"%4",std::to_string(skill.endurance));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}

bool Client::setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level)
{
    if(!CommonFightEngine::setSkillLevel(currentMonster,index,level))
        return false;
    /*std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_skill_level;
    stringreplaceOne(queryText,"%1",std::to_string(level));
    stringreplaceOne(queryText,"%2",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%3",std::to_string(currentMonster->skills.at(index).skill));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}

bool Client::removeSkill(PlayerMonster * currentMonster,const unsigned int &index)
{
    if(!CommonFightEngine::removeSkill(currentMonster,index))
        return false;
    /*const std::string &queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_delete_monster_specific_skill;
    stringreplaceOne(queryText,"%1",std::to_string(currentMonster->id));
    stringreplaceOne(queryText,"%2",std::to_string(currentMonster->skills.at(index).skill));
    dbQueryWriteCommon(queryText);*/
    syncMonsterSkillAndEndurance(*currentMonster);
    return true;
}
