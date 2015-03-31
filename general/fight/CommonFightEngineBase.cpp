#include "CommonFightEngineBase.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonSettings.h"
#include "../base/GeneralVariable.h"

#include <QtMath>
#include <QDebug>

using namespace CatchChallenger;

QList<PlayerMonster::PlayerSkill> CommonFightEngineBase::generateWildSkill(const Monster &monster, const quint8 &level)
{
    QList<PlayerMonster::PlayerSkill> skills;

    int index=monster.learn.size()-1;
    QList<quint16> learnedSkill;
    while(index>=0 && skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
    {
        const Monster::AttackToLearn &attackToLearn=monster.learn.at(index);
        //attackToLearn.learnAtLevel -> need be sorted at load
        if(attackToLearn.learnAtLevel<=level)
        {
            //have already learned the best level because start to learn the highther skill, else if it's new skill
            if(!learnedSkill.contains(attackToLearn.learnSkill))
            {
                PlayerMonster::PlayerSkill temp;
                temp.level=attackToLearn.learnSkillLevel;
                temp.skill=attackToLearn.learnSkill;
                temp.endurance=CommonDatapack::commonDatapack.monsterSkills.value(temp.skill).level.at(temp.level-1).endurance;
                learnedSkill << attackToLearn.learnSkill;
                skills << temp;
            }
        }
        /*else
            break;-->start with wrong value, then never break*/
        index--;
    }

    return skills;
}

/// \warning you need check before the input data
Monster::Stat CommonFightEngineBase::getStat(const Monster &monster, const quint8 &level)
{
    //get the normal stats
    Monster::Stat stat=monster.stat;
    stat.attack=stat.attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.defense=stat.defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.hp=stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.special_attack=stat.special_attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.special_defense=stat.special_defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    stat.speed=stat.speed*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;

    //add a base
    stat.speed+=2;
    stat.defense+=3;
    stat.attack+=2;
    stat.hp+=3;
    stat.special_defense+=3;
    stat.special_attack+=2;

    //drop the 0 value
    if(stat.speed==0)
        stat.speed=1;
    if(stat.defense==0)
        stat.defense=1;
    if(stat.attack==0)
        stat.attack=1;
    if(stat.hp==0)
        stat.hp=1;
    if(stat.special_defense==0)
        stat.special_defense=1;
    if(stat.special_attack==0)
        stat.special_attack=1;

    return stat;
}

bool CommonFightEngineBase::buffIsValid(const Skill::BuffEffect &buffEffect)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buffEffect.buff))
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level>CommonDatapack::commonDatapack.monsterBuffs.value(buffEffect.buff).level.size())
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
