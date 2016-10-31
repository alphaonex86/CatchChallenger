#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonDatapackServerSpec.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/GeneralVariable.h"

#include <iostream>

using namespace CatchChallenger;

bool CommonFightEngine::learnSkill(PlayerMonster *monsterPlayer, const uint16_t &skill)
{
    //search if have already the previous skill
    uint16_t sub_index2=0;
    while(sub_index2<monsterPlayer->skills.size())
    {
        if(monsterPlayer->skills.at(sub_index2).skill==skill)
            break;
        sub_index2++;
    }
    uint16_t sub_index=0;
    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsterPlayer->monster).learn.size())
    {
        const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsterPlayer->monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monsterPlayer->level && learn.learnSkill==skill)
        {
            if(
                    //if skill not found
                    (sub_index2==monsterPlayer->skills.size() && learn.learnSkillLevel==1)
                    ||
                    //if skill already found and need level up
                    (sub_index2<monsterPlayer->skills.size() && (monsterPlayer->skills.at(sub_index2).level+1)==learn.learnSkillLevel)
                    )
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(learn.learnSkill)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                {
                    errorFightEngine("Skill to learn not found into learnSkill()");
                    return false;
                }
                if(learn.learnSkillLevel>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.size())
                {
                    errorFightEngine("Skill level to learn not found learnSkill() "+std::to_string(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.size())+">"+std::to_string(learn.learnSkillLevel));
                    return false;
                }
                if(CommonSettingsServer::commonSettingsServer.useSP)
                {
                    const uint32_t &sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).sp_to_learn;
                    if(sp>monsterPlayer->sp)
                        return false;
                    monsterPlayer->sp-=sp;
                }
                if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=skill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                    addSkill(monsterPlayer,temp);
                }
                else
                    setSkillLevel(monsterPlayer,sub_index2,monsterPlayer->skills.at(sub_index2).level+1);
                return true;
            }
        }
        sub_index++;
    }
    return false;
}

bool CommonFightEngine::learnSkillByItem(PlayerMonster *playerMonster, const uint32_t &itemId)
{
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(playerMonster->monster)==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
    {
        errorFightEngine("Monster id not found into learnSkillByItem()");
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.find(itemId)==CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.cend())
    {
        errorFightEngine("Item id not found into learnSkillByItem()");
        return false;
    }
    const Monster::AttackToLearnByItem &attackToLearnByItem=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(playerMonster->monster).learnByItem.at(itemId);
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(attackToLearnByItem.learnSkill)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
    {
        errorFightEngine("Skill to learn not found into learnSkill()");
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(attackToLearnByItem.learnSkill).level.size()>attackToLearnByItem.learnSkillLevel)
    {
        errorFightEngine("Skill level to learn not found learnSkill()");
        return false;
    }
    //search if have already the previous skill
    unsigned int index=0;
    while(index<playerMonster->skills.size())
    {
        if(playerMonster->skills.at(index).skill==attackToLearnByItem.learnSkill)
        {
            if(playerMonster->skills.at(index).level>=attackToLearnByItem.learnSkillLevel)
                return false;
            else
            {
                setSkillLevel(playerMonster,index,attackToLearnByItem.learnSkillLevel);
                return true;
            }
        }
        index++;
    }
    PlayerMonster::PlayerSkill temp;
    temp.skill=attackToLearnByItem.learnSkill;
    temp.level=attackToLearnByItem.learnSkillLevel;
    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(attackToLearnByItem.learnSkill).level.at(attackToLearnByItem.learnSkillLevel-1).endurance;
    addSkill(playerMonster,temp);
    return true;
}

bool CommonFightEngine::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    currentMonster->skills.push_back(skill);
    return true;
}

bool CommonFightEngine::setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level)
{
    if(index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills[index].level=level;
        return true;
    }
}

bool CommonFightEngine::removeSkill(PlayerMonster * currentMonster,const unsigned int &index)
{
    if(index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills.erase(currentMonster->skills.begin()+index);
        return true;
    }
}

std::vector<Monster::AttackToLearn> CommonFightEngine::autoLearnSkill(const uint8_t &level,const uint8_t &monsterIndex)
{
    std::vector<Monster::AttackToLearn> returnVar;
    const PlayerMonster &monster=public_and_private_informations.playerMonster.at(monsterIndex);
    unsigned int sub_index=0;
    unsigned int sub_index2=0;
    const unsigned int &learn_size=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size();
    while(sub_index<learn_size)
    {
        const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel==level)
        {
            //search if have already the previous skill
            sub_index2=0;
            const unsigned int &monster_skill_size=monster.skills.size();
            while(sub_index2<monster_skill_size)
            {
                if(monster.skills.at(sub_index2).skill==learn.learnSkill)
                {
                    if(monster.skills.at(sub_index2).level<learn.learnSkillLevel)
                    {
                        setSkillLevel(&public_and_private_informations.playerMonster[monsterIndex],sub_index2,learn.learnSkillLevel);
                        returnVar.push_back(learn);
                    }
                    break;
                }
                sub_index2++;
            }
            if(sub_index2==monster_skill_size)
            {
                if(learn.learnSkillLevel==1)
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.skill=learn.learnSkill;
                    temp.level=1;
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                    addSkill(&public_and_private_informations.playerMonster[monsterIndex],temp);
                    returnVar.push_back(learn);
                }
            }
        }
        sub_index++;
    }
    return returnVar;
}

bool CommonFightEngine::useSkill(const uint16_t &skill)
{
    doTurnIfChangeOfMonster=true;
    if(!isInFight())
    {
        errorFightEngine("Try use skill when not in fight");
        return false;
    }
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return false;
    }
    if(currentMonsterIsKO())
    {
        errorFightEngine("Can't attack with KO monster");
        return false;
    }
    const PublicPlayerMonster * otherMonster=getOtherMonster();
    if(otherMonster==NULL)
    {
        errorFightEngine("Unable to locate the other monster");
        return false;
    }
    uint8_t skillLevel=0;
    PlayerMonster::PlayerSkill * currrentMonsterSkill=getTheSkill(skill);
    if(currrentMonsterSkill==NULL)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.find(skill)!=CommonDatapack::commonDatapack.monsterSkills.cend())
            skillLevel=1;//default attack
        else
        {
            errorFightEngine("Unable to fight because the current monster ("+std::to_string(currentMonster->monster)+
                             ", level: "+std::to_string(currentMonster->level)+
                             ") have not the skill "+std::to_string(skill));
            return false;
        }
    }
    else
    {
        skillLevel=currrentMonsterSkill->level;
        if(currrentMonsterSkill->endurance>0)
            decreaseSkillEndurance(currrentMonsterSkill);
        else
        {
            errorFightEngine("Try use skill without endurance: "+std::to_string(skill));
            return false;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
    }
    #endif
    doTheTurn(skill,skillLevel,currentMonsterAttackFirst(currentMonster,otherMonster));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return false;
        }
    }
    #endif
    return true;
}

bool CommonFightEngine::haveTheSkill(const uint16_t &skill)//no const due to error message
{
    const PlayerMonster * const currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
            return true;
        index++;
    }
    return false;
}

PlayerMonster::PlayerSkill * CommonFightEngine::getTheSkill(const uint16_t &skill)//no const due to error message
{
    PlayerMonster * const currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
            return &currentMonster->skills[index];
        index++;
    }
    return NULL;//normal case, hope this if skill is not found
}

uint8_t CommonFightEngine::getSkillLevel(const uint16_t &skill)//no const due to error message
{
    const PlayerMonster * const currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    unsigned int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
            return currentMonster->skills.at(index).level;
        index++;
    }
    errorFightEngine("Unable to locate the current monster skill to get the level");
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    abort();//it's internal error
    #endif
    return 0;
}

uint8_t CommonFightEngine::decreaseSkillEndurance(PlayerMonster::PlayerSkill *skill)
{
    if(skill->endurance>0)
    {
        skill->endurance--;
        return skill->endurance;
    }
    else
    {
        errorFightEngine("Skill don't have correct endurance count");
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();//it's internal error
        #endif
        return 0;
    }
}
