#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonDatapackServerSpec.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/GeneralVariable.h"

#include <iostream>

using namespace CatchChallenger;

bool CommonFightEngine::doTheOtherMonsterTurn()
{
    generateOtherAttack();
    if(currentMonsterIsKO())
        return true;
    else
    {
        if(otherMonsterIsKO())
        {
            checkKOOtherMonstersForGain();
            return true;
        }
    }
    return false;
}

void CommonFightEngine::doTheTurn(const uint16_t &skill,const uint8_t &skillLevel,const bool currentMonsterStatIsFirstToAttack)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            errorFightEngine("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+" the skill use doTheTurn() at 0)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " the skill use doTheTurn() at 0)");
            return;
        }
    }
    #endif
    bool turnIsEnd=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " before the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " before the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        doTheCurrentMonsterAttack(skill,skillLevel);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(currentMonsterIsKO())
            turnIsEnd=true;
        else
        {
            if(otherMonsterIsKO())
            {
                checkKOOtherMonstersForGain();
                turnIsEnd=true;
            }
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
    //do the other monster attack
    if(!turnIsEnd)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(doTheOtherMonsterTurn())
            turnIsEnd=true;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
    //do the current monster attack
    if(!turnIsEnd && !currentMonsterStatIsFirstToAttack)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                errorFightEngine("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        doTheCurrentMonsterAttack(skill,skillLevel);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            PlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Unable to locate the current monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                                 " of current monster "+std::to_string(currentMonster->monster)+
                                 " at level "+std::to_string(currentMonster->level)+
                                 " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                                 " of other monster "+std::to_string(otherMonster->monster)+
                                 " at level "+std::to_string(otherMonster->level)+
                                 " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                                 " after the skill use doTheTurn() at 1)");
                return;
            }
        }
        #endif
        if(currentMonsterIsKO())
            turnIsEnd=true;
        else
        {
            if(otherMonsterIsKO())
            {
                checkKOOtherMonstersForGain();
                turnIsEnd=true;
            }
        }
    }
    (void)turnIsEnd;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return;
        }
    }
    #endif
}

Skill::AttackReturn CommonFightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.attack=0;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.attackReturnCase=Skill::AttackReturnCase_NormalAttack;
    attackReturn.success=false;
    PlayerMonster *otherMonster;
    if(!wildMonsters.empty())
        otherMonster=&wildMonsters.front();
    else if(!botFightMonsters.empty())
        otherMonster=&botFightMonsters.front();
    else
    {
        errorFightEngine("no other monster found");
        return attackReturn;
    }
    if(otherMonster->skills.empty())
    {
        if(CommonDatapack::commonDatapack.monsterSkills.find(0)!=CommonDatapack::commonDatapack.monsterSkills.cend())
        {
            messageFightEngine("Generated bot/wild default attack");
            attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),0,1);
            attackReturn.doByTheCurrentMonster=false;
            return attackReturn;
        }
        else
        {
            messageFightEngine("No other monster attack todo");
            return attackReturn;
        }
    }
    unsigned int position;
    if(otherMonster->skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster->skills.size()-1);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(position>=otherMonster->skills.size())
    {
        messageFightEngine("Position out of range: "+std::to_string(position)+" on "+std::to_string(otherMonster->skills.size())+" total skill(s)");
        position=position%otherMonster->skills.size();
    }
    #endif
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster->skills.at(position);
    messageFightEngine("Generated bot/wild attack: "+std::to_string(otherMonsterSkill.skill)+
                       " (position: "+std::to_string(position)+
                       ") at level "+std::to_string(otherMonsterSkill.level)+
                       " on "+std::to_string(otherMonster->skills.size())+
                       " total skill(s)");
    attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),otherMonsterSkill.skill,otherMonsterSkill.level);
    attackReturn.doByTheCurrentMonster=false;
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const uint16_t &skill, const uint8_t &skillLevel)
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=true;
    attackReturn.attackReturnCase=Skill::AttackReturnCase_NormalAttack;
    attackReturn.attack=skill;
    attackReturn.success=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(currentMonster==NULL)
        {
            errorFightEngine("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonster->hp==0)
        {
            errorFightEngine("Can't attack with KO monster");
            return attackReturn;
        }
        if(otherMonster==NULL)
        {
            errorFightEngine("Unable to locate the other monster");
            return attackReturn;
        }
        if(otherMonster->hp==0)
        {
            errorFightEngine("Can't attack with KO monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    const Skill &skillDef=CommonDatapack::commonDatapack.monsterSkills.at(skill);
    const Skill::SkillList &skillList=skillDef.level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    messageFightEngine("You use skill "+std::to_string(skill)+" at level "+std::to_string(skillLevel));
    #endif
    unsigned int index;
    //do the skill
    if(!genericMonsterIsKO(currentMonster))
    {
        index=0;
        while(index<skillList.life.size())
        {
            const Skill::Life &life=skillList.life.at(index);
            if(life.effect.quantity!=0)
            {
                bool success;
                if(life.success==100)
                    success=true;
                else
                    success=(getOneSeed(100)<life.success);
                if(success)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    //don't use pointer  here because the value of currentMonster->hp will change
                    const uint32_t currentMonsterHp=currentMonster->hp;
                    const uint32_t otherMonsterHp=otherMonster->hp;
                    #endif
                    attackReturn.success=true;//the attack have work because at less have a buff
                    Skill::LifeEffectReturn lifeEffectReturn;
                    lifeEffectReturn=applyLifeEffect(skillDef.type,life.effect,currentMonster,otherMonster);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(lifeEffectReturn.on==ApplyOn_AllAlly || lifeEffectReturn.on==ApplyOn_Themself)
                    {
                        if(currentMonster->hp!=(currentMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine("life effect: Returned damage don't match with the real effect on current monster: "+
                                             std::to_string(currentMonster->hp)+"!=("+std::to_string(currentMonsterHp)+
                                             "+"+std::to_string(lifeEffectReturn.quantity)+")");
                            return attackReturn;
                        }
                    }
                    if(lifeEffectReturn.on==ApplyOn_AllEnemy || lifeEffectReturn.on==ApplyOn_AloneEnemy)
                    {
                        if(otherMonster->hp!=(otherMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine("life effect: Returned damage don't match with the real effect on other monster: "+
                                             std::to_string(otherMonster->hp)+"!=("+std::to_string(otherMonsterHp)+
                                             "+"+std::to_string(lifeEffectReturn.quantity)+")");
                            return attackReturn;
                        }
                    }
                    #endif
                    attackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    //do the buff
    if(!genericMonsterIsKO(currentMonster))
    {
        index=0;
        while(index<skillList.buff.size())
        {
            const Skill::Buff &buff=skillList.buff.at(index);
            #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
            if(!buffIsValid(buff.effect))
            {
                errorFightEngine("Buff is not valid");
                return attackReturn;
            }
            #endif
            bool success;
            if(buff.success==100)
                success=true;
            else
            {
                success=(getOneSeed(100)<buff.success);
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    messageFightEngine("Add successfull buff: "+std::to_string(buff.effect.buff)+" at level: "+std::to_string(buff.effect.level)+" on "+std::to_string(buff.effect.on));
                #endif
            }
            if(success)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    messageFightEngine("Add buff: "+std::to_string(buff.effect.buff)+" at level: "+std::to_string(buff.effect.level)+" on "+std::to_string(buff.effect.on));
                #endif
                if(addBuffEffectFull(buff.effect,currentMonster,otherMonster)>=-2)//0 to X, update buff, -1 added, -2 updated same buff at same level
                {
                    attackReturn.success=true;//the attack have work because at less have a buff
                    attackReturn.addBuffEffectMonster.push_back(buff.effect);
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    //apply the effect of current buff
    if(!genericMonsterIsKO(currentMonster))
    {
        auto removeOldBuffVar=removeOldBuff(currentMonster);
        attackReturn.removeBuffEffectMonster.insert(attackReturn.removeBuffEffectMonster.cend(),removeOldBuffVar.cbegin(),removeOldBuffVar.cend());
        const std::vector<Skill::LifeEffectReturn> &lifeEffectMonster=applyBuffLifeEffect(currentMonster);
        attackReturn.lifeEffectMonster.insert(attackReturn.lifeEffectMonster.cend(),lifeEffectMonster.cbegin(),lifeEffectMonster.cend());
    }
    if(genericMonsterIsKO(currentMonster) && !genericMonsterIsKO(otherMonster))
        doTurnIfChangeOfMonster=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(currentMonster->hp)+
                             " of current monster "+std::to_string(currentMonster->monster)+
                             " at level "+std::to_string(currentMonster->level)+
                             " is greater than the max "+std::to_string(currentMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine("The hp "+std::to_string(otherMonster->hp)+
                             " of other monster "+std::to_string(otherMonster->monster)+
                             " at level "+std::to_string(otherMonster->level)+
                             " is greater than the max "+std::to_string(otherMonsterStat.hp)+
                             " after the skill use doTheTurn() at 1)");
            return attackReturn;
        }
    }
    #endif
    return attackReturn;
}

Skill::LifeEffectReturn CommonFightEngine::applyLifeEffect(const uint8_t &type,const Skill::LifeEffect &effect,PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster)
{
    Skill::LifeEffectReturn effect_to_return;
    int32_t quantity;
    const Monster &commonMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(currentMonster->monster);
    const Monster &commonOtherMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster);
    Monster::Stat stat=getStat(commonMonster,currentMonster->level);
    Monster::Stat otherStat=getStat(commonOtherMonster,otherMonster->level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            otherStat=stat;
            otherMonster=currentMonster;
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
        break;
    }
    if(effect.quantity==0)
        quantity=0;
    else if(effect.type==QuantityType_Quantity)
    {
        if(effect.quantity>0)
            quantity=effect.quantity;
        else
        {
            const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(type);
            float OtherMulti=1.0;
            if(vectorcontainsAtLeastOne(commonMonster.type,type))
            {
                std::cout << "1.45x because the attack is same type as the current monster: " << typeDefinition.name << std::endl;
                OtherMulti*=1.45;
            }
            effect_to_return.effective=1.0;
            const std::vector<uint8_t> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster).type;
            if(type!=255 && !typeList.empty())
            {
                unsigned int index=0;
                while(index<typeList.size())
                {
                    if(typeDefinition.multiplicator.find(typeList.at(index))!=typeDefinition.multiplicator.cend())
                    {
                        const int8_t &multiplicator=typeDefinition.multiplicator.at(typeList.at(index));
                        if(multiplicator>0)
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            std::cout << "type: " << typeList.at(index) << "very effective againts" << typeDefinition.name << std::endl;
                            #endif
                            effect_to_return.effective*=multiplicator;
                        }
                        else
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            std::cout << "type: " << typeList.at(index) << "not effective againts" << typeDefinition.name << std::endl;
                            #endif
                            effect_to_return.effective/=-multiplicator;
                        }
                    }
                    index++;
                }
            }
            float criticalHit=1.0;
            effect_to_return.critical=(getOneSeed(255)<20);
            if(effect_to_return.critical)
            {
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                std::cout << "critical hit, then: 1.5x" << std::endl;
                #endif
                criticalHit=1.5;
            }
            uint32_t attack;
            uint32_t defense;
            if(type==0)
                attack=stat.attack;
            else
                attack=stat.special_attack;
            if(type==0)
                defense=otherStat.defense;
            else
                defense=otherStat.special_defense;
            if(defense<1)
                defense=1;
            const uint8_t &seed=getOneSeed(17);
            quantity = effect_to_return.effective*(((float)currentMonster->level*(float)1.99+10.5)/(float)255*((float)attack/(float)defense)*(float)effect.quantity)*criticalHit*OtherMulti*(100-seed)/100;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            {
                int32_t effect_quantity=effect.quantity;
                if(effect_quantity<0)
                    effect_quantity-=2;
            }
            std::cout << "quantity(" << quantity << ") = effect_to_return.effective(" << effect_to_return.effective << ")*(((float)currentMonster->level(" << currentMonster->level << ")*(float)1.99+10.5)/(float)255*((float)attack(" << attack << ")/(float)defense(" << defense << "))*(float)effect.quantity(" << effect.quantity << "))*criticalHit(" << criticalHit << ")*OtherMulti(" << OtherMulti << ")*(100-getOneSeed(17)(" << seed << "))/100; effect_quantity: " << effect_quantity << std::endl;
            #endif
            /*if(effect.quantity<0)
                quantity=-((-effect.quantity*stat.attack)/(otherStat.defense*4));
            else if(effect.quantity>0)//ignore the def for heal
                quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            const std::vector<uint8_t> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster).type;
            if(type!=255 && !typeList.empty())
            {
                unsigned int index=0;
                while(index<typeList.size())
                {
                    const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(typeList.at(index));
                    if(typeDefinition.multiplicator.contains(type))
                    {
                        const int8_t &multiplicator=typeDefinition.multiplicator.at(type);
                        if(multiplicator>=0)
                            quantity*=multiplicator;
                        else
                            quantity/=-multiplicator;
                    }
                    index++;
                }
            }*/
        }
    }
    else
        quantity=((int64_t)currentMonster->hp*(int64_t)effect.quantity)/(int64_t)100;
    if(effect.quantity<0)
    {
        if(quantity==0)
            quantity=-1;
    }
    else if(effect.quantity>0)
    {
        if(quantity==0)
            quantity=1;
    }
    //check
    if(effect.quantity<0 && quantity>0)
    {
        quantity=-1;
        errorFightEngine("Wrong calculated value");
    }
    if(effect.quantity>0 && quantity<0)
    {
        quantity=1;
        errorFightEngine("Wrong calculated value");
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp was:" << otherMonster->hp << std::endl;
    #endif
    //kill
    if(quantity<0 && (-quantity)>=(int32_t)otherMonster->hp)
    {
        quantity=-(int32_t)otherMonster->hp;
        otherMonster->hp=0;
    }
    //full heal
    else if(quantity>0 && quantity>=(int32_t)(stat.hp-otherMonster->hp))
    {
        quantity=(int32_t)(stat.hp-otherMonster->hp);
        otherMonster->hp=stat.hp;
    }
    //other life change
    else
        otherMonster->hp+=quantity;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    std::cout << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp is now:" << otherMonster->hp << std::endl;
    #endif
    effect_to_return.critical=false;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void CommonFightEngine::confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster)
{
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(playerMonster->monster);
    const Monster::Stat oldStat=getStat(monsterInformations,playerMonster->level);
    playerMonster->monster=monster;
    const Monster::Stat stat=getStat(CommonDatapack::commonDatapack.monsters.at(playerMonster->monster),playerMonster->level);
    if(oldStat.hp>stat.hp)
    {
        if(playerMonster->hp>stat.hp)
            playerMonster->hp=stat.hp;
    }
    else if(oldStat.hp<stat.hp)
    {
        if(playerMonster->hp>0)
            playerMonster->hp+=(stat.hp-oldStat.hp);
    }
}
