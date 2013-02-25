#include "FightEngine.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../base/Api_client_real.h"

#include <QDebug>

using namespace CatchChallenger;

FightEngine FightEngine::fightEngine;

FightEngine::FightEngine()
{
    resetAll();
}

FightEngine::~FightEngine()
{
}

//return is have random seed to do random step
bool FightEngine::canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    if(!wildMonsters.empty())
    {
        qDebug() << QString("map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y);
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(map,x,y) && !map.grassMonster.empty())
        return m_randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(CatchChallenger::MoveOnTheMap::isWater(map,x,y) && !map.waterMonster.empty())
        return m_randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(!map.caveMonster.empty())
        return m_randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    qDebug() << QString("map: %1 (%2,%3), no fight in this zone").arg(map.map_file).arg(x).arg(y);
    return true;
}

bool FightEngine::haveRandomFight(const Map &map,const quint8 &x,const quint8 &y)
{
    bool ok;
    if(!wildMonsters.empty())
    {
        qDebug() << QString("error: map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y);
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(map,x,y) && !map.grassMonster.empty())
    {
        if(stepFight_Grass.empty())
        {
            if(m_randomSeeds.size()==0)
            {
                qDebug() << QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y);
                return false;
            }
            else
            {
                stepFight_Grass << getOneSeed(16);
                qDebug() << QString("next grass monster into: %1").arg(stepFight_Grass.last());
            }
        }
        else
            stepFight_Grass.first()--;
        if(stepFight_Grass.first()<=0)
        {
            stepFight_Grass.removeFirst();

            PlayerMonster monster=getRandomMonster(map.grassMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }
    if(CatchChallenger::MoveOnTheMap::isWater(map,x,y) && !map.waterMonster.empty())
    {
        if(stepFight_Water.empty())
        {
            if(m_randomSeeds.size()==0)
            {
                qDebug() << QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y);
                return false;
            }
            else
            {
                stepFight_Water << getOneSeed(16);
                qDebug() << QString("next water monster into: %1").arg(stepFight_Water.last());
            }
        }
        else
            stepFight_Water.first()--;
        if(stepFight_Water.first()<=0)
        {
            stepFight_Water.removeFirst();

            PlayerMonster monster=getRandomMonster(map.waterMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }
    if(!map.caveMonster.empty())
    {
        if(stepFight_Cave.empty())
        {
            if(m_randomSeeds.size()==0)
            {
                qDebug() << QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y);
                return false;
            }
            else
            {
                stepFight_Cave << getOneSeed(16);
                qDebug() << QString("next cave monster into: %1").arg(stepFight_Cave.last());
            }
        }
        else
            stepFight_Cave.first()--;
        if(stepFight_Cave.first()<=0)
        {
            stepFight_Cave.removeFirst();

            PlayerMonster monster=getRandomMonster(map.caveMonster,&ok);
            if(ok)
                wildMonsters << monster;
            return ok;
        }
        else
            return false;
    }

    /// no fight in this zone
    return false;
}

quint8 FightEngine::getOneSeed(const quint8 &max)
{
    quint16 number=static_cast<quint8>(m_randomSeeds.at(0));
    if(max!=0)
    {
        number*=max;
        number/=255;
    }
    m_randomSeeds.remove(0,1);
    return number;
}

void FightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.success=false;
    const PlayerMonster &otherMonster=wildMonsters.first();
    if(otherMonster.skills.empty())
        return;
    int position;
    if(otherMonster.skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster.skills.size());
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster.skills.at(position);
    attackReturn.attack=otherMonsterSkill.skill;
    const Skill::SkillList &skillList=monsterSkills[otherMonsterSkill.skill].level.at(otherMonsterSkill.level-1);
    int index=0;
    while(index<skillList.buff.size())
    {
        const Skill::Buff &buff=skillList.buff.at(index);
        bool success;
        if(buff.success==100)
            success=true;
        else
            success=(getOneSeed(100)<buff.success);
        if(success)
        {
            applyOtherBuffEffect(buff.effect);
            attackReturn.buffEffectMonster << buff.effect;
            attackReturn.success=true;
        }
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        const Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
        {
            attackReturn.lifeEffectMonster << applyOtherLifeEffect(life.effect);
            attackReturn.success=true;
        }
        index++;
    }
    attackReturnList << attackReturn;
}

Skill::LifeEffectReturn FightEngine::applyOtherLifeEffect(const Skill::LifeEffect &effect)
{
    qint32 quantity;
    Monster::Stat stat=getStat(monsters[wildMonsters.first().monster],wildMonsters.first().level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(monsters[playerMonsterList[selectedMonster].monster],playerMonsterList[selectedMonster].level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*wildMonsters.first().level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(playerMonsterList[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>playerMonsterList[selectedMonster].hp)
            {
                playerMonsterList[selectedMonster].hp=0;
                playerMonsterList[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(stat.hp-playerMonsterList[selectedMonster].hp))
                playerMonsterList[selectedMonster].hp=stat.hp;
            else
                playerMonsterList[selectedMonster].hp+=quantity;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*wildMonsters.first().level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(wildMonsters.first().hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>wildMonsters.first().hp)
            {
                wildMonsters.first().hp=0;
                addXPSP();
            }
            else if(quantity>0 && quantity>(stat.hp-wildMonsters.first().hp))
                wildMonsters.first().hp=stat.hp;
            else
                wildMonsters.first().hp+=quantity;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void FightEngine::applyOtherBuffEffect(const Skill::BuffEffect &effect)
{
    PlayerMonster::PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            playerMonsterList[selectedMonster].buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            wildMonsters.first().buffs << tempBuff;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
}

bool FightEngine::wildMonsterIsKO()
{
    if(wildMonsters.empty())
        return true;
    if(wildMonsters.first().hp==0)
    {
        wildMonsters.first().buffs.clear();
        return true;
    }
    return false;
}

bool FightEngine::currentMonsterIsKO()
{
    if(playerMonsterList[selectedMonster].hp==0)
    {
        playerMonsterList[selectedMonster].buffs.clear();
        return true;
    }
    return false;
}

bool FightEngine::dropKOCurrentMonster()
{
    if(playerMonsterList[selectedMonster].hp==0)
    {
        playerMonsterList[selectedMonster].buffs.clear();
        updateCanDoFight();
        return true;
    }
    return false;
}

bool FightEngine::dropKOWildMonster()
{
    if(!wildMonsterIsKO())
        return false;
    wildMonsters.removeFirst();
    return true;
}

PlayerMonster FightEngine::getRandomMonster(const QList<MapMonster> &monsterList,bool *ok)
{
    PlayerMonster playerMonster;
    playerMonster.captured_with=0;
    playerMonster.egg_step=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    quint8 randomMonsterInt=getOneSeed(100);
    bool monsterFound=false;
    int index=0;
    while(index<monsterList.size())
    {
        int luck=monsterList.at(index).luck;
        if(randomMonsterInt<luck)
        {
            //it's this monster
            playerMonster.monster=monsterList.at(index).id;
            //select the level
            if(monsterList.at(index).maxLevel==monsterList.at(index).minLevel)
                playerMonster.level=monsterList.at(index).minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.at(index).maxLevel-monsterList.at(index).minLevel+1)+monsterList.at(index).minLevel;
            monsterFound=true;
            break;
        }
        else
            randomMonsterInt-=luck;
        index++;
    }
    if(!monsterFound)
    {
        qDebug() << QString("error: no wild monster selected");
        *ok=false;
        playerMonster.monster=0;
        playerMonster.level=0;
        playerMonster.gender=PlayerMonster::Unknown;
        return playerMonster;
    }
    Monster monsterDef=monsters[playerMonster.monster];
    if(monsterDef.ratio_gender>0 && monsterDef.ratio_gender<100)
    {
        qint8 temp_ratio=getOneSeed(101);
        if(temp_ratio<monsterDef.ratio_gender)
            playerMonster.gender=PlayerMonster::Male;
        else
            playerMonster.gender=PlayerMonster::Female;
    }
    else
    {
        switch(monsterDef.ratio_gender)
        {
            case 0:
                playerMonster.gender=PlayerMonster::Male;
            break;
            case 100:
                playerMonster.gender=PlayerMonster::Female;
            break;
            default:
                playerMonster.gender=PlayerMonster::Unknown;
            break;
        }
    }
    Monster::Stat monsterStat=getStat(monsterDef,playerMonster.level);
    playerMonster.hp=monsterStat.hp;
    index=monsterDef.learn.size()-1;
    while(index>=0 && playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
    {
        if(monsterDef.learn.at(index).learnAtLevel<=playerMonster.level)
        {
            PlayerMonster::PlayerSkill temp;
            temp.level=monsterDef.learn.at(index).learnSkillLevel;
            temp.skill=monsterDef.learn.at(index).learnSkill;
            playerMonster.skills << temp;
        }
        index--;
    }
    *ok=true;
    return playerMonster;
}

bool FightEngine::canDoFight()
{
    return m_canDoFight;
}

void FightEngine::healAllMonsters()
{
    int index=0;
    while(index<playerMonsterList.size())
    {
        if(playerMonsterList.at(index).egg_step==0)
            playerMonsterList[index].hp=
                    monsters[playerMonsterList.at(index).monster].stat.hp*
                    playerMonsterList.at(index).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        index++;
    }
}

bool FightEngine::isInFight()
{
    if(!wildMonsters.empty())
        return true;
    else
        return false;
}

bool FightEngine::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &monster=playerMonsterList.at(index);
        if(monster.id==monsterId)
        {
            int sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                if(monster.skills.at(sub_index2).skill==skill)
                    break;
                sub_index2++;
            }
            int sub_index=0;
            while(sub_index<CatchChallenger::FightEngine::fightEngine.monsters[monster.monster].learn.size())
            {
                const Monster::AttackToLearn &learn=CatchChallenger::FightEngine::fightEngine.monsters[monster.monster].learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills[sub_index2].level+1)==learn.learnSkillLevel)
                    {
                        quint32 sp=CatchChallenger::FightEngine::fightEngine.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).sp;
                        if(sp>monster.sp)
                            return false;
                        playerMonsterList[index].sp-=sp;
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            playerMonsterList[index].skills << temp;
                        }
                        else
                            playerMonsterList[index].skills[sub_index2].level++;
                        return true;
                    }
                }
                sub_index++;
            }
            return false;
        }
        index++;
    }
    return false;
}

void FightEngine::setPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    this->playerMonsterList=playerMonster;
    updateCanDoFight();
}

void FightEngine::addPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    playerMonsterList << playerMonster;
    updateCanDoFight();
}

void FightEngine::updateCanDoFight()
{
    m_canDoFight=false;
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonsterEntry=playerMonsterList.at(index);
        if(playerMonsterEntry.hp>0 && playerMonsterEntry.egg_step==0)
        {
            selectedMonster=index;
            m_canDoFight=true;
            return;
        }
        index++;
    }
}

QList<PlayerMonster> FightEngine::getPlayerMonster()
{
    return playerMonsterList;
}

bool FightEngine::removeMonster(const quint32 &monsterId)
{
    int index=0;
    while(index<playerMonsterList.size())
    {
        if(playerMonsterList.at(index).id==monsterId)
        {
            playerMonsterList.removeAt(index);
            updateCanDoFight();
            return true;
        }
        index++;
    }
    return false;
}

bool FightEngine::remainMonstersToFight(const quint32 &monsterId)
{
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonsterEntry=playerMonsterList.at(index);
        if(playerMonsterEntry.id==monsterId)
        {
            //the current monster can't fight, echange it will do nothing
            if(playerMonsterEntry.hp<=0 || playerMonsterEntry.egg_step>0)
                return true;
        }
        else
        {
            //other monster can fight, can continue to fight
            if(playerMonsterEntry.hp>0 && playerMonsterEntry.egg_step==0)
                return true;
        }
        index++;
    }
    return false;
}

PlayerMonster FightEngine::getFightMonster()
{
    return playerMonsterList.at(selectedMonster);
}

PlayerMonster FightEngine::getOtherMonster()
{
    return wildMonsters.first();
}

bool FightEngine::haveOtherMonster()
{
    return !wildMonsters.empty();
}

void FightEngine::resetAll()
{
    monsters.clear();
    monsterSkills.clear();
    monsterBuffs.clear();
    m_randomSeeds.clear();
    playerMonsterList.clear();
    m_canDoFight=false;

    monsterExtra.clear();
    monsterBuffsExtra.clear();
    monsterSkillsExtra.clear();
    attackReturnList.clear();

    wildMonsters.clear();
}

void FightEngine::appendRandomSeeds(const QByteArray &data)
{
    m_randomSeeds+=data;
}

/** \warning you need check before the input data */
Monster::Stat FightEngine::getStat(const Monster &monster, const quint8 &level)
{
    Monster::Stat stat=monster.stat;
    stat.attack=stat.attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.attack==0)
        stat.attack=1;
    stat.defense=stat.defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.defense==0)
        stat.defense=1;
    stat.hp=stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.hp==0)
        stat.hp=1;
    stat.special_attack=stat.special_attack*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.special_attack==0)
        stat.special_attack=1;
    stat.special_defense=stat.special_defense*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.special_defense==0)
        stat.special_defense=1;
    stat.speed=stat.speed*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    if(stat.speed==0)
        stat.speed=1;
    return stat;
}

const QByteArray FightEngine::randomSeeds()
{
    return m_randomSeeds;
}

bool FightEngine::tryEscape()
{
    if(internalTryEscape())
    {
        wildMonsters.clear();
        return true;
    }
    else
        return false;
}

bool FightEngine::internalTryEscape()
{
    CatchChallenger::Api_client_real::client->tryEscape();
    quint8 value=getOneSeed(101);
    if(wildMonsters.first().level<playerMonsterList.at(selectedMonster).level && value<75)
        return true;
    if(wildMonsters.first().level==playerMonsterList.at(selectedMonster).level && value<50)
        return true;
    if(wildMonsters.first().level>playerMonsterList.at(selectedMonster).level && value<25)
        return true;
    return false;
}

void FightEngine::addXPSP()
{
    const Monster &wildmonster=monsters[wildMonsters.first().monster];
    const Monster &currentmonster=monsters[playerMonsterList[selectedMonster].monster];
    playerMonsterList[selectedMonster].sp+=wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    quint32 give_xp=wildmonster.give_xp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    quint32 xp=playerMonsterList[selectedMonster].remaining_xp;
    quint32 level=playerMonsterList[selectedMonster].level;
    while(currentmonster.level_to_xp.at(level-1)<(xp+give_xp))
    {
        quint32 old_max_hp=currentmonster.stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        quint32 new_max_hp=currentmonster.stat.hp*(level+1)/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        give_xp-=currentmonster.level_to_xp.at(level-1)-xp;
        xp=0;
        level++;
        playerMonsterList[selectedMonster].hp+=new_max_hp-old_max_hp;
        playerMonsterList[selectedMonster].level=level;
    }
    xp+=give_xp;
    playerMonsterList[selectedMonster].remaining_xp=xp;
}

bool FightEngine::canDoFightAction()
{
    if(m_randomSeeds.size()>5)
        return true;
    else
        return false;
}

void FightEngine::useSkill(const quint32 &skill)
{
    CatchChallenger::Api_client_real::client->useSkill(skill);
    Monster::Stat currentMonsterStat=getStat(monsters[playerMonsterList.at(selectedMonster).monster],playerMonsterList.at(selectedMonster).level);
    Monster::Stat otherMonsterStat=getStat(monsters[wildMonsters.first().monster],wildMonsters.first().level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    //do the current monster attack
    if(currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill);
        if(!m_canDoFight || wildMonsterIsKO())
            return;
    }
    //do the other monster attack
    generateOtherAttack();
    if(!m_canDoFight || wildMonsterIsKO())
        return;
    //do the current monster attack
    if(!currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill);
        if(!m_canDoFight || wildMonsterIsKO())
            return;
    }
}

void FightEngine::doTheCurrentMonsterAttack(const quint32 &skill)
{
    int index=0;
    while(index<playerMonsterList.at(selectedMonster).skills.size())
    {
        if(playerMonsterList.at(selectedMonster).skills.at(index).skill==skill)
            break;
        index++;
    }
    if(index==playerMonsterList.at(selectedMonster).skills.size())
    {
        qDebug() << QString("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(playerMonsterList.at(selectedMonster).monster).arg(playerMonsterList.at(selectedMonster).level).arg(skill);
        return;
    }
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=true;
    attackReturn.success=false;
    const Skill::SkillList &skillList=monsterSkills[playerMonsterList.at(selectedMonster).skills.at(index).skill].level.at(playerMonsterList.at(selectedMonster).skills.at(index).level-1);
    index=0;
    while(index<skillList.buff.size())
    {
        const Skill::Buff &buff=skillList.buff.at(index);
        bool success;
        if(buff.success==100)
            success=true;
        else
            success=(getOneSeed(100)<buff.success);
        if(success)
        {
            applyCurrentBuffEffect(buff.effect);
            attackReturn.buffEffectMonster << buff.effect;
            attackReturn.success=true;
        }
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        const Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
        {
            attackReturn.lifeEffectMonster << applyCurrentLifeEffect(life.effect);
            attackReturn.success=true;
        }
        index++;
    }
    attackReturnList << attackReturn;
}

Skill::LifeEffectReturn FightEngine::applyCurrentLifeEffect(const Skill::LifeEffect &effect)
{
    qint32 quantity;
    Monster::Stat stat=getStat(monsters[playerMonsterList.at(selectedMonster).monster],playerMonsterList.at(selectedMonster).level);
    Monster::Stat otherStat;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                otherStat=getStat(monsters[wildMonsters.first().monster],wildMonsters.first().level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*playerMonsterList.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*playerMonsterList.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(wildMonsters.first().hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>wildMonsters.first().hp)
            {
                wildMonsters.first().hp=0;
                addXPSP();
            }
            else if(quantity>0 && quantity>(stat.hp-wildMonsters.first().hp))
                wildMonsters.first().hp=stat.hp;
            else
                wildMonsters.first().hp+=quantity;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*playerMonsterList.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*playerMonsterList.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(playerMonsterList[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>playerMonsterList[selectedMonster].hp)
            {
                playerMonsterList[selectedMonster].hp=0;
                playerMonsterList[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(stat.hp-playerMonsterList[selectedMonster].hp))
                playerMonsterList[selectedMonster].hp=stat.hp;
            else
                playerMonsterList[selectedMonster].hp+=quantity;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void FightEngine::applyCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    PlayerMonster::PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            wildMonsters.first().buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            playerMonsterList[selectedMonster].buffs << tempBuff;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
}

