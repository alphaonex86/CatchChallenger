#include "CommonFightEngineBase.hpp"
#include "../base/CommonDatapack.hpp"
#include "../base/GeneralVariable.hpp"

using namespace CatchChallenger;

std::vector<PlayerMonster::PlayerSkill> CommonFightEngineBase::generateWildSkill(const Monster &monster, const uint8_t &level)
{
    std::vector<PlayerMonster::PlayerSkill> skills;
    if(monster.learn.empty())
        return skills;

    uint16_t index=static_cast<uint16_t>(monster.learn.size()-1);
    std::vector<uint16_t> learnedSkill;
    while(skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
    {
        const Monster::AttackToLearn &attackToLearn=monster.learn.at(index);
        //attackToLearn.learnAtLevel -> need be sorted at load
        if(attackToLearn.learnAtLevel<=level)
        {
            //have already learned the best level because start to learn the highther skill, else if it's new skill
            if(!vectorcontainsAtLeastOne(learnedSkill,attackToLearn.learnSkill))
            {
                PlayerMonster::PlayerSkill temp;
                temp.level=attackToLearn.learnSkillLevel;
                temp.skill=attackToLearn.learnSkill;
                temp.endurance=CommonDatapack::commonDatapack.get_monsterSkills().at(temp.skill).level.at(temp.level-1).endurance;
                learnedSkill.push_back(attackToLearn.learnSkill);
                skills.push_back(temp);
            }
        }
        /*else
            break;-->start with wrong value, then never break*/
        if(index==0)
            break;
        index--;
    }

    return skills;
}

/// \warning you need check before the input data
Monster::Stat CommonFightEngineBase::getStat(const Monster &monster, const uint8_t &level)
{
    //get the normal stats
    Monster::Stat stat=monster.stat;
    stat.hp=stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    stat.attack=stat.attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.defense=stat.defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.special_attack=stat.special_attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.special_defense=stat.special_defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.speed=stat.speed*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    #endif

    //add a base
    stat.hp+=3;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    stat.speed+=2;
    stat.defense+=3;
    stat.attack+=2;
    stat.special_defense+=3;
    stat.special_attack+=2;
    #endif

    //drop the 0 value
    if(stat.hp==0)
        stat.hp=1;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    if(stat.speed==0)
        stat.speed=1;
    if(stat.defense==0)
        stat.defense=1;
    if(stat.attack==0)
        stat.attack=1;
    if(stat.special_defense==0)
        stat.special_defense=1;
    if(stat.special_attack==0)
        stat.special_attack=1;
    #endif

    return stat;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
bool CommonFightEngineBase::buffIsValid(const Skill::BuffEffect &buffEffect)
{
    if(CommonDatapack::commonDatapack.get_monsterBuffs().find(buffEffect.buff)==CommonDatapack::commonDatapack.get_monsterBuffs().cend())
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level>CommonDatapack::commonDatapack.get_monsterBuffs().at(buffEffect.buff).level.size())
        return false;
    switch(buffEffect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
        case ApplyOn_Nobody:
        break;
        default:
        return false;
    }
    return true;
}
#endif
