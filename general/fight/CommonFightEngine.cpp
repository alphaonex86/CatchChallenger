#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"
#include "../base/CommonSettings.h"
#include "../base/GeneralVariable.h"

#include <QtMath>
#include <QDebug>

using namespace CatchChallenger;

CommonFightEngine::CommonFightEngine()
{
    resetAll();
}

bool CommonFightEngine::canEscape()
{
    if(!isInFight())//check if is in fight
    {
        errorFightEngine(QStringLiteral("error: tryEscape() when is not in fight"));
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        messageFightEngine("You can't escape because it's not a wild monster");
        return false;
    }
    return true;
}

void CommonFightEngine::resetAll()
{
    stepFight=0;
    ableToFight=false;
    wildMonsters.clear();
    botFightMonsters.clear();
    selectedMonster=0;
    doTurnIfChangeOfMonster=true;
}

bool CommonFightEngine::otherMonsterIsKO()
{
    PublicPlayerMonster * publicPlayerMonster=getOtherMonster();
    if(publicPlayerMonster==NULL)
        return true;
    return genericMonsterIsKO(publicPlayerMonster);
}

bool CommonFightEngine::genericMonsterIsKO(const PublicPlayerMonster * publicPlayerMonster) const
{
    if(publicPlayerMonster->hp==0)
        return true;
    return false;
}

bool CommonFightEngine::currentMonsterIsKO()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
        return true;
    return genericMonsterIsKO(playerMonster);
}

bool CommonFightEngine::dropKOCurrentMonster()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    bool currentPlayerReturn=false;
    if(playerMonster==NULL)
        currentPlayerReturn=true;
    else if(playerMonster->hp==0)
    {
        playerMonster->buffs.clear();
        doTurnIfChangeOfMonster=false;
        ableToFight=false;
        currentPlayerReturn=true;
    }
    return currentPlayerReturn;
}

bool CommonFightEngine::dropKOOtherMonster()
{
    bool otherMonsterReturn=false;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            wildMonsters.removeFirst();
            otherMonsterReturn=true;
        }
    }
    if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            botFightMonsters.removeFirst();
            otherMonsterReturn=true;
        }
    }
    return otherMonsterReturn;
}

void CommonFightEngine::healAllMonsters()
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(public_and_private_informations.playerMonster.at(index).monster),public_and_private_informations.playerMonster.at(index).level);
            public_and_private_informations.playerMonster[index].hp=stat.hp;
            public_and_private_informations.playerMonster[index].buffs.clear();
            int sub_index=0;
            while(sub_index<public_and_private_informations.playerMonster.value(index).skills.size())
            {
                public_and_private_informations.playerMonster[index].skills[sub_index].endurance=
                        CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(
                        public_and_private_informations.playerMonster[index].skills.at(sub_index).skill
                        )
                        .level.at(public_and_private_informations.playerMonster.value(index).skills.at(sub_index).level-1).endurance;
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

bool CommonFightEngine::learnSkill(const quint32 &monsterId, const quint32 &skill)
{
    int index=0;
    int sub_index2,sub_index;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &monster=public_and_private_informations.playerMonster.at(index);
        //have located the monster
        if(monster.id==monsterId)
        {
            //search if have already the previous skill
            sub_index2=0;
            const int &monster_skill_size=monster.skills.size();
            while(sub_index2<monster_skill_size)
            {
                if(monster.skills.at(sub_index2).skill==skill)
                    break;
                sub_index2++;
            }
            sub_index=0;
            const int &learn_size=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.size();
            while(sub_index<learn_size)
            {
                const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if(
                            //if skill not found
                            (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                            ||
                            //if skill already found and need level up
                            (sub_index2<monster.skills.size() && (monster.skills.value(sub_index2).level+1)==learn.learnSkillLevel)
                            )
                    {
                        if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(learn.learnSkill))
                        {
                            errorFightEngine(QStringLiteral("Skill to learn not found into learnSkill()"));
                            return false;
                        }
                        if(learn.learnSkillLevel>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.size())
                        {
                            errorFightEngine(QStringLiteral("Skill level to learn not found learnSkill() %2>%1").arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.size()).arg(learn.learnSkillLevel));
                            return false;
                        }
                        if(CommonSettings::commonSettings.useSP)
                        {
                            const quint32 &sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel-1).sp_to_learn;
                            if(sp>monster.sp)
                                return false;
                            public_and_private_informations.playerMonster[index].sp-=sp;
                        }
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                            addSkill(&public_and_private_informations.playerMonster[index],temp);
                        }
                        else
                            setSkillLevel(&public_and_private_informations.playerMonster[index],sub_index2,public_and_private_informations.playerMonster.at(index).skills.at(sub_index2).level+1);
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

bool CommonFightEngine::learnSkillByItem(PlayerMonster *playerMonster, const quint32 &itemId)
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(playerMonster->monster))
    {
        errorFightEngine(QStringLiteral("Monster id not found into learnSkillByItem()"));
        return false;
    }
    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster).learnByItem.contains(itemId))
    {
        errorFightEngine(QStringLiteral("Item id not found into learnSkillByItem()"));
        return false;
    }
    const Monster::AttackToLearnByItem &attackToLearnByItem=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster).learnByItem.value(itemId);
    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(attackToLearnByItem.learnSkill))
    {
        errorFightEngine(QStringLiteral("Skill to learn not found into learnSkill()"));
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(attackToLearnByItem.learnSkill).level.size()>attackToLearnByItem.learnSkillLevel)
    {
        errorFightEngine(QStringLiteral("Skill level to learn not found learnSkill()"));
        return false;
    }
    //search if have already the previous skill
    int index=0;
    const int &monster_skill_size=playerMonster->skills.size();
    while(index<monster_skill_size)
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
    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(attackToLearnByItem.learnSkill).level.at(attackToLearnByItem.learnSkillLevel-1).endurance;
    addSkill(playerMonster,temp);
    return true;
}

bool CommonFightEngine::addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill)
{
    currentMonster->skills << skill;
    return true;
}

bool CommonFightEngine::setSkillLevel(PlayerMonster * currentMonster,const int &index,const quint8 &level)
{
    if(index<0 || index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills[index].level=level;
        return true;
    }
}

bool CommonFightEngine::removeSkill(PlayerMonster * currentMonster,const int &index)
{
    if(index<0 || index>=currentMonster->skills.size())
        return false;
    else
    {
        currentMonster->skills.removeAt(index);
        return true;
    }
}

bool CommonFightEngine::isInFight() const
{
    return !wildMonsters.empty() || !botFightMonsters.isEmpty();
}

bool CommonFightEngine::isInFightWithWild() const
{
    return !wildMonsters.empty();
}

bool CommonFightEngine::getAbleToFight() const
{
    return ableToFight;
}

bool CommonFightEngine::haveMonsters() const
{
    return !public_and_private_informations.playerMonster.isEmpty();
}

//return is have random seed to do random step
bool CommonFightEngine::canDoRandomFight(const CommonMap &map,const quint8 &x,const quint8 &y) const
{
    if(isInFight())
    {
        messageFightEngine(QStringLiteral("map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y));
        return false;
    }
    if(map.parsed_layer.monstersCollisionMap==NULL)
        return true;

    quint8 zoneCode=map.parsed_layer.monstersCollisionMap[x+y*map.width];
    const MonstersCollisionValue &monstersCollisionValue=map.parsed_layer.monstersCollisionList.at(zoneCode);
    if(monstersCollisionValue.walkOn.isEmpty())
        return true;
    else
        return randomSeedsSize()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    messageFightEngine(QStringLiteral("map: %1 (%2,%3), no fight in this zone").arg(map.map_file).arg(x).arg(y));
    return true;
}

PlayerMonster CommonFightEngine::getRandomMonster(const QList<MapMonster> &monsterList,bool *ok)
{
    PlayerMonster playerMonster;
    playerMonster.catched_with=0;
    playerMonster.egg_step=0;
    playerMonster.remaining_xp=0;
    playerMonster.sp=0;
    quint8 randomMonsterInt=getOneSeed(100);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    quint8 randomMonsterIntOriginal=randomMonsterInt;
    QString monsterListString;
    #endif
    bool monsterFound=false;
    int index=0;
    while(index<monsterList.size())
    {
        const MapMonster &mapMonster=monsterList.at(index);
        const int &luck=mapMonster.luck;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        monsterListString=QStringLiteral("luck:%1;id:%2;").arg(mapMonster.luck).arg(mapMonster.id);
        #endif
        if(randomMonsterInt<=luck)// with < it crash because randomMonsterInt can be 0
        {
            //it's this monster
            playerMonster.monster=mapMonster.id;
            //select the level
            if(mapMonster.maxLevel==mapMonster.minLevel)
                playerMonster.level=mapMonster.minLevel;
            else
                playerMonster.level=getOneSeed(mapMonster.maxLevel-mapMonster.minLevel+1)+mapMonster.minLevel;
            monsterFound=true;
            break;
        }
        else
            randomMonsterInt-=luck;
        index++;
    }
    if(!monsterFound)
    {
        if(monsterList.isEmpty())
        {
            #ifndef CATCHCHALLENGER_EXTRA_CHECK
            errorFightEngine(QStringLiteral("error: no wild monster selected, with: randomMonsterInt: %1").arg(randomMonsterInt));
            #else
            errorFightEngine(QStringLiteral("error: no wild monster selected, with: randomMonsterIntOriginal: %1, monsterListString: %2").arg(randomMonsterIntOriginal).arg(monsterListString));
            #endif

            *ok=false;
            playerMonster.monster=0;
            playerMonster.level=0;
            playerMonster.gender=Gender_Unknown;
            return playerMonster;
        }
        else
        {
            #ifndef CATCHCHALLENGER_EXTRA_CHECK
            messageFightEngine(QStringLiteral("warning: no wild monster selected, with: randomMonsterInt: %1").arg(randomMonsterInt));
            #else
            messageFightEngine(QStringLiteral("warning: no wild monster selected, with: randomMonsterIntOriginal: %1, monsterListString: %2").arg(randomMonsterIntOriginal).arg(monsterListString));
            #endif
            playerMonster.monster=monsterList.first().id;
            //select the level
            if(monsterList.first().maxLevel==monsterList.first().minLevel)
                playerMonster.level=monsterList.first().minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.first().maxLevel-monsterList.first().minLevel+1)+monsterList.first().minLevel;
        }
    }
    Monster monsterDef=CommonDatapack::commonDatapack.monsters.value(playerMonster.monster);
    if(monsterDef.ratio_gender>0 && monsterDef.ratio_gender<100)
    {
        qint8 temp_ratio=getOneSeed(101);
        if(temp_ratio<monsterDef.ratio_gender)
            playerMonster.gender=Gender_Male;
        else
            playerMonster.gender=Gender_Female;
    }
    else
    {
        switch(monsterDef.ratio_gender)
        {
            case 0:
                playerMonster.gender=Gender_Male;
            break;
            case 100:
                playerMonster.gender=Gender_Female;
            break;
            default:
                playerMonster.gender=Gender_Unknown;
            break;
        }
    }
    Monster::Stat monsterStat=getStat(monsterDef,playerMonster.level);
    playerMonster.hp=monsterStat.hp;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    messageFightEngine(QStringLiteral("do skill while: playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER: %1<%2").arg(playerMonster.skills.size()).arg(CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER));
    #endif
    {
        index=monsterDef.learn.size()-1;
        QList<quint16> learnedSkill;
        while(index>=0 && playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
        {
            const Monster::AttackToLearn &attackToLearn=monsterDef.learn.at(index);
            //attackToLearn.learnAtLevel -> need be sorted at load
            if(attackToLearn.learnAtLevel<=playerMonster.level)
            {
                //have already learned the best level because start to learn the highther skill, else if it's new skill
                if(!learnedSkill.contains(attackToLearn.learnSkill))
                {
                    PlayerMonster::PlayerSkill temp;
                    temp.level=attackToLearn.learnSkillLevel;
                    temp.skill=attackToLearn.learnSkill;
                    temp.endurance=CommonDatapack::commonDatapack.monsterSkills.value(temp.skill).level.at(temp.level-1).endurance;
                    learnedSkill << attackToLearn.learnSkill;
                    playerMonster.skills << temp;
                }
            }
            /*else
                break;-->start with wrong value, then never break*/
            index--;
        }
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        if(playerMonster.skills.isEmpty())
        {
            if(monsterDef.learn.isEmpty())
                messageFightEngine(QStringLiteral("no skill to learn for random monster"));
            else
            {
                messageFightEngine(QStringLiteral("no skill for random monster, but skill to learn:"));
                int index=0;
                while(index<monsterDef.learn.size())
                {
                    messageFightEngine(
                                QStringLiteral("%1 level %2 for monster at level %3")
                                       .arg(monsterDef.learn.at(index).learnSkill)
                                       .arg(monsterDef.learn.at(index).learnSkillLevel)
                                       .arg(monsterDef.learn.at(index).learnAtLevel)
                                       );
                    index++;
                }
            }
        }
        else
        {
            int index=0;
            while(index<playerMonster.skills.size())
            {
                messageFightEngine(QStringLiteral("skill for random monster: %1 level %2").arg(playerMonster.skills.at(index).skill).arg(playerMonster.skills.at(index).level));
                index++;
            }
        }
        messageFightEngine(QStringLiteral("random monster id: %1").arg(playerMonster.monster));
    }
    #endif
    *ok=true;
    return playerMonster;
}

/// \warning you need check before the input data
Monster::Stat CommonFightEngine::getStat(const Monster &monster, const quint8 &level)
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

void CommonFightEngine::updateCanDoFight()
{
    if(isInBattle())
    {
        errorFightEngine(QStringLiteral("Can't auto select the next monster when is in battle"));
        return;
    }
    ableToFight=false;
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
        {
            selectedMonster=index;
            ableToFight=true;
            return;
        }
        index++;
    }
}

bool CommonFightEngine::haveAnotherMonsterOnThePlayerToFight() const
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
            return true;
        index++;
    }
    return false;
}

bool CommonFightEngine::haveAnotherEnnemyMonsterToFight()
{
    if(!wildMonsters.isEmpty())
        return false;
    if(!botFightMonsters.isEmpty())
        return botFightMonsters.size()>1;
    errorFightEngine("Unable to locate the other monster");
    return false;
}

PlayerMonster * CommonFightEngine::getCurrentMonster()
{
    const int &playerMonsterSize=public_and_private_informations.playerMonster.size();
    if(selectedMonster>=0 && selectedMonster<playerMonsterSize)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!CommonDatapack::commonDatapack.monsters.contains(public_and_private_informations.playerMonster.at(selectedMonster).monster))
        {
            errorFightEngine(QStringLiteral("Current monster don't exists: %1").arg(public_and_private_informations.playerMonster.at(selectedMonster).monster));
            return NULL;
        }
        #endif
        return &public_and_private_informations.playerMonster[selectedMonster];
    }
    else
    {
        errorFightEngine(QStringLiteral("selectedMonster is out of range, max: %1").arg(playerMonsterSize));
        return NULL;
    }
}

quint8 CommonFightEngine::getCurrentSelectedMonsterNumber() const
{
    return selectedMonster;
}

quint8 CommonFightEngine::getOtherSelectedMonsterNumber() const
{
    return 0;
}

PublicPlayerMonster *CommonFightEngine::getOtherMonster()
{
    if(!wildMonsters.isEmpty())
        return (PublicPlayerMonster *)&wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        return (PublicPlayerMonster *)&botFightMonsters.first();
    else
        return NULL;
}

bool CommonFightEngine::remainMonstersToFight(const quint32 &monsterId) const
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(playerMonsterEntry.id==monsterId)
        {
            //the current monster can't fight, echange it will do nothing
            if(monsterIsKO(playerMonsterEntry))
                return true;
        }
        else
        {
            //other monster can fight, can continue to fight
            if(!monsterIsKO(playerMonsterEntry))
                return true;
        }
        index++;
    }
    return false;
}

bool CommonFightEngine::monsterIsKO(const PlayerMonster &playerMonter)
{
    return playerMonter.hp<=0 || playerMonter.egg_step>0;
}

Skill::LifeEffectReturn CommonFightEngine::applyLifeEffect(const quint8 &type,const Skill::LifeEffect &effect,PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster)
{
    Skill::LifeEffectReturn effect_to_return;
    qint32 quantity;
    const Monster &commonMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster);
    const Monster &commonOtherMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster);
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
            if(commonMonster.type.contains(type))
            {
                qDebug() << "1.45x because the attack is same type as the current monster" << typeDefinition.name;
                OtherMulti*=1.45;
            }
            effect_to_return.effective=1.0;
            const QList<quint8> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster).type;
            if(type!=255 && !typeList.isEmpty())
            {
                int index=0;
                while(index<typeList.size())
                {
                    if(typeDefinition.multiplicator.contains(typeList.at(index)))
                    {
                        const qint8 &multiplicator=typeDefinition.multiplicator.value(typeList.at(index));
                        if(multiplicator>0)
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            qDebug() << "type: " << typeList.at(index) << "very effective againts" << typeDefinition.name;
                            #endif
                            effect_to_return.effective*=multiplicator;
                        }
                        else
                        {
                            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                            qDebug() << "type: " << typeList.at(index) << "not effective againts" << typeDefinition.name;
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
                qDebug() << "critical hit, then: 1.5x";
                #endif
                criticalHit=1.5;
            }
            quint32 attack;
            quint32 defense;
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
            qint32 effect_quantity=effect.quantity;
            if(effect_quantity<0)
                effect_quantity-=2;
            const quint8 &seed=getOneSeed(17);
            quantity = effect_to_return.effective*(((float)currentMonster->level*(float)1.99+10.5)/(float)255*((float)attack/(float)defense)*(float)effect.quantity)*criticalHit*OtherMulti*(100-seed)/100;
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            qDebug() << "quantity(" << quantity << ") = effect_to_return.effective(" << effect_to_return.effective << ")*(((float)currentMonster->level(" << currentMonster->level << ")*(float)1.99+10.5)/(float)255*((float)attack(" << attack << ")/(float)defense(" << defense << "))*(float)effect.quantity(" << effect.quantity << "))*criticalHit(" << criticalHit << ")*OtherMulti(" << OtherMulti << ")*(100-getOneSeed(17)(" << seed << "))/100;";
            #endif
            /*if(effect.quantity<0)
                quantity=-((-effect.quantity*stat.attack)/(otherStat.defense*4));
            else if(effect.quantity>0)//ignore the def for heal
                quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            const QList<quint8> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster).type;
            if(type!=255 && !typeList.isEmpty())
            {
                int index=0;
                while(index<typeList.size())
                {
                    const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(typeList.at(index));
                    if(typeDefinition.multiplicator.contains(type))
                    {
                        const qint8 &multiplicator=typeDefinition.multiplicator.value(type);
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
        quantity=((qint64)currentMonster->hp*(qint64)effect.quantity)/(qint64)100;
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
    qDebug() << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp was:" << otherMonster->hp;
    #endif
    //kill
    if(quantity<0 && (-quantity)>=(qint32)otherMonster->hp)
    {
        quantity=-(qint32)otherMonster->hp;
        otherMonster->hp=0;
    }
    //full heal
    else if(quantity>0 && quantity>=(qint32)(stat.hp-otherMonster->hp))
    {
        quantity=(qint32)(stat.hp-otherMonster->hp);
        otherMonster->hp=stat.hp;
    }
    //other life change
    else
        otherMonster->hp+=quantity;
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    qDebug() << "Life: Apply on" << otherMonster->monster << "quantity" << quantity << "hp is now:" << otherMonster->hp;
    #endif
    effect_to_return.critical=false;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

int CommonFightEngine::addOtherBuffEffect(const Skill::BuffEffect &effect)
{
    return addBuffEffectFull(effect,getOtherMonster(),getCurrentMonster());
}

int CommonFightEngine::addCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    return addBuffEffectFull(effect,getCurrentMonster(),getOtherMonster());
}

/* return code:
 * >=0 if the index of the buff updated
 * -1 buff added
 * -2 same buff already found
 * -3 failed by hp
 * -4 failed by other
 * */
int CommonFightEngine::addBuffEffectFull(const Skill::BuffEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(effect.buff))
    {
        errorFightEngine(QStringLiteral("apply a unknown buff"));
        return -4;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.size())
    {
        errorFightEngine(QStringLiteral("apply buff level out of range"));
        return -4;
    }
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    tempBuff.remainingNumberOfTurn=CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.at(effect.level-1).durationNumberOfTurn;
    int index=0;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(otherMonster->hp==0)
                return -3;
            while(index<otherMonster->buffs.size())
            {
                if(otherMonster->buffs.at(index).buff==effect.buff)
                {
                    if(otherMonster->buffs.at(index).level==effect.level)
                        return -2;
                    otherMonster->buffs[index].level=effect.level;
                        return index;
                }
                index++;
            }
            otherMonster->buffs << tempBuff;
            return -1;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(currentMonster->hp==0)
                return -3;
            while(index<currentMonster->buffs.size())
            {
                if(currentMonster->buffs.at(index).buff==effect.buff)
                {
                    if(currentMonster->buffs.at(index).level==effect.level)
                        return -2;
                    currentMonster->buffs[index].level=effect.level;
                        return index;
                }
                index++;
            }
            currentMonster->buffs << tempBuff;
            return -1;
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
            return -4;
        break;
    }
    return -4;
}

void CommonFightEngine::removeBuffEffectFull(const Skill::BuffEffect &effect)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(effect.buff))
    {
        errorFightEngine(QStringLiteral("apply a unknown buff"));
        return;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.size())
    {
        errorFightEngine(QStringLiteral("apply buff level out of range"));
        return;
    }
    int index=0;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {
            PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                errorFightEngine("Other monster not found for buff remove");
                return;
            }
            while(index<otherMonster->buffs.size())
            {
                if(otherMonster->buffs.at(index).buff==effect.buff)
                {
                    if(otherMonster->buffs.at(index).level!=effect.level)
                        messageFightEngine(QStringLiteral("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(otherMonster->buffs.at(index).level).arg(effect.level));
                    otherMonster->buffs.removeAt(index);
                    return;
                }
                index++;
            }
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
        {
            PublicPlayerMonster * currentMonster=getCurrentMonster();
            if(currentMonster==NULL)
            {
                errorFightEngine("Other monster not found for buff remove");
                return;
            }
            while(index<currentMonster->buffs.size())
            {
                if(currentMonster->buffs.at(index).buff==effect.buff)
                {
                    if(currentMonster->buffs.at(index).level!=effect.level)
                        messageFightEngine(QStringLiteral("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(currentMonster->buffs.at(index).level).arg(effect.level));
                    currentMonster->buffs.removeAt(index);
                    return;
                }
                index++;
            }
        }
        break;
        default:
            errorFightEngine("Not apply match, can't apply the buff");
            return;
        break;
    }
    errorFightEngine("Buff not found to remove");
}

void CommonFightEngine::confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster)
{
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(playerMonster->monster);
    const Monster::Stat oldStat=getStat(monsterInformations,playerMonster->level);
    playerMonster->monster=monster;
    const Monster::Stat stat=getStat(CommonDatapack::commonDatapack.monsters.value(playerMonster->monster),playerMonster->level);
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

/// \note Object quantity verified in LocalClientHandler::useObjectOnMonster()
bool CommonFightEngine::useObjectOnMonster(const quint32 &object,const quint32 &monster)
{
    if(!haveThisMonster(monster))
    {
        errorFightEngine(QStringLiteral("have not this monster: %1").arg(object));
        return false;
    }
    PlayerMonster * playerMonster=monsterById(monster);
    if(genericMonsterIsKO(playerMonster))
    {
        errorFightEngine(QStringLiteral("can't be applyied on KO monster: %1").arg(object));
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(object))
    {
        if(isInFight())
        {
            errorFightEngine(QStringLiteral("this item %1 can't be used in fight").arg(object));
            return false;
        }
        if(!CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(object).contains(playerMonster->monster))
        {
            errorFightEngine(QStringLiteral("this item %1 can't be applied on this monster %2").arg(object).arg(playerMonster->monster));
            return false;
        }
        confirmEvolutionTo(playerMonster,CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(object).value(playerMonster->monster));
        return true;
    }
    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.contains(object) || CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.contains(object))
    {
        if(CommonDatapack::commonDatapack.items.monsterItemEffect.contains(object))
        {
            const Monster::Stat &playerMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster),playerMonster->level);
            const QList<MonsterItemEffect> monsterItemEffect = CommonDatapack::commonDatapack.items.monsterItemEffect.values(object);
            int index=0;
            while(index<monsterItemEffect.size())
            {
                const MonsterItemEffect &effect=monsterItemEffect.at(index);
                switch(effect.type)
                {
                    case MonsterItemEffectType_AddHp:
                        if(effect.value>0 && (playerMonsterStat.hp-playerMonster->hp)>(quint32)effect.value)
                            hpChange(playerMonster,playerMonster->hp+effect.value);
                        else
                            hpChange(playerMonster,playerMonsterStat.hp);
                    break;
                    case MonsterItemEffectType_RemoveBuff:
                        if(effect.value>0)
                            removeBuffOnMonster(playerMonster,effect.value);
                        else
                            removeAllBuffOnMonster(playerMonster);
                    break;
                    default:
                        messageFightEngine(QStringLiteral("Item %1 have unknown effect").arg(object));
                    break;
                }
                index++;
            }
            if(isInFight())
                doTheOtherMonsterTurn();
        }
        else if(CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.contains(object))
        {
            if(isInFight())
            {
                errorFightEngine(QStringLiteral("this item %1 can't be used in fight").arg(object));
                return false;
            }
            const QList<MonsterItemEffectOutOfFight> monsterItemEffectOutOfFight = CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.values(object);
            int index=0;
            while(index<monsterItemEffectOutOfFight.size())
            {
                const MonsterItemEffectOutOfFight &effect=monsterItemEffectOutOfFight.at(index);
                switch(effect.type)
                {
                    case MonsterItemEffectTypeOutOfFight_AddLevel:
                        if(playerMonster->level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                        {
                            errorFightEngine(QStringLiteral("this item %1 can't be used on monster at level max").arg(object));
                            return false;
                        }
                        addLevel(playerMonster);
                    break;
                    default:
                        messageFightEngine(QStringLiteral("Item %1 have unknown effect").arg(object));
                    break;
                }
                index++;
            }
        }
    }
    else if(CommonDatapack::commonDatapack.items.itemToLearn.contains(object))
    {
        if(isInFight())
        {
            errorFightEngine(QStringLiteral("this item %1 can't be used in fight").arg(object));
            return false;
        }
        if(!CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.value(object).contains(playerMonster->monster))
        {
            errorFightEngine(QStringLiteral("this item %1 can't be applied on this monster %2").arg(object).arg(playerMonster->monster));
            return false;
        }
        return learnSkillByItem(playerMonster,object);
    }
    else
    {
        errorFightEngine(QStringLiteral("This object can't be applied on monster: %1").arg(object));
        return false;
    }

    return true;
}

void CommonFightEngine::hpChange(PlayerMonster * currentMonster,const quint32 &newHpValue)
{
    currentMonster->hp=newHpValue;
}

bool CommonFightEngine::removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId)
{
    int index=0;
    while(index<currentMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=currentMonster->buffs.at(index);
        if(playerBuff.buff==buffId)
        {
            currentMonster->buffs.removeAt(index);
            return true;
        }
    }
    return false;
}

bool CommonFightEngine::removeAllBuffOnMonster(PlayerMonster * currentMonster)
{
    if(!currentMonster->buffs.isEmpty())
    {
        currentMonster->buffs.clear();
        return true;
    }
    return false;
}

bool CommonFightEngine::changeOfMonsterInFight(const quint32 &monsterId)
{
    if(!isInFight())
        return false;
    if(getCurrentMonster()->id==monsterId)
    {
        errorFightEngine(QStringLiteral("try change monster but is already on the current monster"));
        return false;
    }
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=public_and_private_informations.playerMonster.at(index);
        if(playerMonsterEntry.id==monsterId)
        {
            if(!monsterIsKO(playerMonsterEntry))
            {
                selectedMonster=index;
                ableToFight=true;
                if(doTurnIfChangeOfMonster)
                    doTheOtherMonsterTurn();
                else
                    doTurnIfChangeOfMonster=true;
                return true;
            }
            else
                return false;
        }
        index++;
    }
    errorFightEngine(QStringLiteral("unable to locate the new monster to change"));
    return false;
}

ApplyOn CommonFightEngine::invertApplyOn(const ApplyOn &applyOn)
{
    switch(applyOn)
    {
        case ApplyOn_AloneEnemy:
            return ApplyOn_Themself;
        case ApplyOn_AllEnemy:
            return ApplyOn_AllAlly;
        case ApplyOn_Themself:
            return ApplyOn_AloneEnemy;
        case ApplyOn_AllAlly:
            return ApplyOn_AllEnemy;
        default:
            return ApplyOn_Themself;
        break;
    }
}

bool CommonFightEngine::internalTryEscape()
{
    doTurnIfChangeOfMonster=true;
    quint8 value=getOneSeed(101);
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        errorFightEngine("No current monster to try escape");
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        errorFightEngine("Not againts wild monster");
        return false;
    }
    if(wildMonsters.first().level<playerMonster->level && value<75)
        return true;
    if(wildMonsters.first().level==playerMonster->level && value<50)
        return true;
    if(wildMonsters.first().level>playerMonster->level && value<25)
        return true;
    return false;
}

bool CommonFightEngine::internalTryCapture(const Trap &trap)
{
    if(wildMonsters.isEmpty())
    {
        errorFightEngine("Not againts wild monster");
        return false;
    }
    const Monster::Stat &wildMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(wildMonsters.first().monster),wildMonsters.first().level);

    float bonusStat=1.0;
    if(wildMonsters.first().buffs.size())
    {
        bonusStat=0;
        int index=0;
        while(index<wildMonsters.first().buffs.size())
        {
            const PlayerBuff &playerBuff=wildMonsters.first().buffs.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.contains(playerBuff.buff))
            {
                const Buff &buff=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.value(playerBuff.buff);
                if(playerBuff.level>0 && playerBuff.level<=buff.level.size())
                    bonusStat+=buff.level.at(playerBuff.level-1).capture_bonus;
                else
                {
                    errorFightEngine(QStringLiteral("Buff level for wild monter not found: %1 at level %2").arg(playerBuff.buff).arg(playerBuff.level));
                    bonusStat+=1.0;
                }
            }
            else
            {
                errorFightEngine(QStringLiteral("Buff for wild monter not found: %1").arg(playerBuff.buff));
                bonusStat+=1.0;
            }
            index++;
        }
        bonusStat/=wildMonsters.first().buffs.size();
    }
    const quint32 maxTempRate=12;
    const quint32 minTempRate=5;
    const quint32 tryCapture=4;
    const quint32 catch_rate=(quint32)CatchChallenger::CommonDatapack::commonDatapack.monsters.value(wildMonsters.first().monster).catch_rate;
    quint32 tempRate=(catch_rate*(wildMonsterStat.hp*maxTempRate-wildMonsters.first().hp*minTempRate)*bonusStat*trap.bonus_rate)/(wildMonsterStat.hp*maxTempRate);
    if(tempRate>255)
        return true;
    quint32 realRate=65535*qPow((float)tempRate/(float)255,0.25);

    messageFightEngine(QStringLiteral("Formule: (%1*(%2-%3)*%4*%5)/(%6), realRate: %7")
                 .arg(catch_rate)
                 .arg(wildMonsterStat.hp*maxTempRate)
                 .arg(minTempRate*wildMonsters.first().hp)
                 .arg(bonusStat)
                 .arg(trap.bonus_rate)
                 .arg(wildMonsterStat.hp*maxTempRate)
                 .arg(realRate)
                 );

    quint32 index=0;
    while(index<tryCapture)
    {
        quint16 number=rand()%65536;
        if(number>=realRate)
            return false;
        index++;
    }
    return true;
}

quint32 CommonFightEngine::tryCapture(const quint32 &item)
{
    doTurnIfChangeOfMonster=true;
    if(internalTryCapture(CommonDatapack::commonDatapack.items.trap.value(item)))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine(QStringLiteral("capture is successful"));
        #endif
        PlayerMonster newMonster;
        newMonster.buffs=wildMonsters.first().buffs;
        newMonster.catched_with=item;
        newMonster.egg_step=0;
        newMonster.gender=wildMonsters.first().gender;
        newMonster.hp=wildMonsters.first().hp;
        newMonster.id=0;//unknown at this time
        newMonster.level=wildMonsters.first().level;
        newMonster.monster=wildMonsters.first().monster;
        newMonster.remaining_xp=0;
        newMonster.skills=wildMonsters.first().skills;
        newMonster.sp=0;
        newMonster.id=catchAWild(public_and_private_informations.playerMonster.size()>=CommonSettings::commonSettings.maxPlayerMonsters,newMonster);
        return newMonster.id;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine(QStringLiteral("capture is failed"));
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return 0;
    }
}

void CommonFightEngine::fightFinished()
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        int sub_index=0;
        while(sub_index<public_and_private_informations.playerMonster.at(index).buffs.size())
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.contains(public_and_private_informations.playerMonster.at(index).buffs.at(sub_index).buff))
            {
                const PlayerBuff &playerBuff=public_and_private_informations.playerMonster.at(index).buffs.at(sub_index);
                if(CommonDatapack::commonDatapack.monsterBuffs.value(playerBuff.buff).level.at(playerBuff.level-1).duration!=Buff::Duration_Always)
                    public_and_private_informations.playerMonster[index].buffs.removeAt(sub_index);
                else
                    sub_index++;
            }
            else
                public_and_private_informations.playerMonster[index].buffs.removeAt(sub_index);
        }
        index++;
    }
    wildMonsters.clear();
    botFightMonsters.clear();
    doTurnIfChangeOfMonster=true;
}

void CommonFightEngine::addPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    public_and_private_informations.playerMonster << playerMonster;
    updateCanDoFight();
}

void CommonFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    public_and_private_informations.playerMonster << playerMonster;
    updateCanDoFight();
}

void CommonFightEngine::insertPlayerMonster(const quint8 &place,const PlayerMonster &playerMonster)
{
    public_and_private_informations.playerMonster.insert(place,playerMonster);
    updateCanDoFight();
}

QList<PlayerMonster> CommonFightEngine::getPlayerMonster() const
{
    return public_and_private_informations.playerMonster;
}

bool CommonFightEngine::moveUpMonster(const quint8 &number)
{
    if(isInFight())
        return false;
    if(number==0)
        return false;
    if(number>=(public_and_private_informations.playerMonster.size()))
        return false;
    public_and_private_informations.playerMonster.swap(number,number-1);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::moveDownMonster(const quint8 &number)
{
    if(isInFight())
        return false;
    if(number>=(public_and_private_informations.playerMonster.size()-1))
        return false;
    public_and_private_informations.playerMonster.swap(number,number+1);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::removeMonster(const quint32 &monsterId)
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            public_and_private_informations.playerMonster.removeAt(index);
            updateCanDoFight();
            return true;
        }
        index++;
    }
    return false;
}

bool CommonFightEngine::haveThisMonster(const quint32 &monsterId) const
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
            return true;
        index++;
    }
    return false;
}

PlayerMonster * CommonFightEngine::monsterById(const quint32 &monsterId)
{
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
            return &public_and_private_informations.playerMonster[index];
        index++;
    }
    return NULL;
}

void CommonFightEngine::wildDrop(const quint32 &monster)
{
    Q_UNUSED(monster);
}

bool CommonFightEngine::checkKOOtherMonstersForGain()
{
    messageFightEngine(QStringLiteral("checkKOOtherMonstersForGain()"));
    bool winTheTurn=false;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine(QStringLiteral("The wild monster (%1) is KO").arg(wildMonsters.first().monster));
            #endif
            //drop the drop item here
            wildDrop(wildMonsters.first().monster);
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters.value(wildMonsters.first().monster);
            //multiplicator do at datapack loading
            int sp=wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=wildmonster.give_xp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine(QStringLiteral("You win %1 xp and %2 sp").arg(wildmonster.give_xp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX).arg(wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
        }
    }
    else if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine(QStringLiteral("The wild monster (%1) is KO").arg(botFightMonsters.first().monster));
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &botmonster=CommonDatapack::commonDatapack.monsters.value(botFightMonsters.first().monster);
            //multiplicator do at datapack loading
            int sp=botmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=botmonster.give_xp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            messageFightEngine(QStringLiteral("You win %1 xp and %2 sp").arg(botmonster.give_xp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX).arg(botmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
        }
    }
    else if(isInBattle())
    {
        if(otherMonsterIsKO())
            return true;
        return false;
    }
    else
    {
        errorFightEngine("unknown other monster type");
        return false;
    }
    return winTheTurn;
}

//return true if change level, multiplicator do at datapack loading
bool CommonFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=false;
    PlayerMonster * monster=getCurrentMonster();
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster->monster);
    quint32 remaining_xp=monster->remaining_xp;
    quint8 level=monster->level;
    if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        remaining_xp=0;
        xp=0;
    }
    while((remaining_xp+xp)>=monsterInformations.level_to_xp.at(level-1))
    {
        const quint32 &old_max_hp=getStat(monsterInformations,level).hp;
        const quint32 &new_max_hp=getStat(monsterInformations,level+1).hp;
        xp-=monsterInformations.level_to_xp.at(level-1)-remaining_xp;
        remaining_xp=0;
        level++;
        levelUp(level,getCurrentSelectedMonsterNumber());
        haveChangeOfLevel=true;
        if(new_max_hp>old_max_hp)
            monster->hp+=new_max_hp-old_max_hp;
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine(QStringLiteral("You pass to the level %1").arg(level));
        #endif
        if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            remaining_xp=0;
            xp=0;
        }
    }
    remaining_xp+=xp;
    monster->remaining_xp=remaining_xp;
    if(haveChangeOfLevel)
        monster->level=level;
    monster->sp+=sp;

    return haveChangeOfLevel;
}

bool CommonFightEngine::addLevel(PlayerMonster * monster, const quint8 &numberOfLevel)
{
    if(monster->level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        errorFightEngine(QStringLiteral("Monster is already at the level max into addLevel()"));
        return false;
    }
    int monsterIndex=0;
    const int &monsterListSize=public_and_private_informations.playerMonster.size();
    while(monsterIndex<monsterListSize)
    {
        if(&public_and_private_informations.playerMonster.at(monsterIndex)==monster)
            break;
        monsterIndex++;
    }
    if(monsterIndex==monsterListSize)
    {
        errorFightEngine(QStringLiteral("Monster index not for to add level"));
        return false;
    }
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster->monster);
    const quint8 &level=public_and_private_informations.playerMonster.at(monsterIndex).level;
    const quint32 &old_max_hp=getStat(monsterInformations,level).hp;
    const quint32 &new_max_hp=getStat(monsterInformations,level+1).hp;
    if(new_max_hp>old_max_hp)
        monster->hp+=new_max_hp-old_max_hp;
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    messageFightEngine(QStringLiteral("You pass to the level %1 on your monster %2").arg(level).arg(monster->id));
    #endif

    if((public_and_private_informations.playerMonster.value(monsterIndex).level+numberOfLevel)>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        public_and_private_informations.playerMonster[monsterIndex].level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    else
        public_and_private_informations.playerMonster[monsterIndex].level+=numberOfLevel;
    public_and_private_informations.playerMonster[monsterIndex].remaining_xp=0;
    levelUp(level,monsterIndex);
    return true;
}

QList<Monster::AttackToLearn> CommonFightEngine::autoLearnSkill(const quint8 &level,const quint8 &monsterIndex)
{
    QList<Monster::AttackToLearn> returnVar;
    const PlayerMonster &monster=public_and_private_informations.playerMonster.value(monsterIndex);
    int sub_index=0;
    int sub_index2=0;
    const int &learn_size=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.size();
    while(sub_index<learn_size)
    {
        const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel==level)
        {
            //search if have already the previous skill
            sub_index2=0;
            const int &monster_skill_size=monster.skills.size();
            while(sub_index2<monster_skill_size)
            {
                if(monster.skills.at(sub_index2).skill==learn.learnSkill)
                {
                    if(monster.skills.at(sub_index2).level<learn.learnSkillLevel)
                    {
                        setSkillLevel(&public_and_private_informations.playerMonster[monsterIndex],sub_index2,learn.learnSkillLevel);
                        returnVar << learn;
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
                    temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                    addSkill(&public_and_private_informations.playerMonster[monsterIndex],temp);
                    returnVar << learn;
                }
            }
        }
        sub_index++;
    }
    return returnVar;
}

void CommonFightEngine::levelUp(const quint8 &level,const quint8 &monsterIndex)//call after done the level
{
    autoLearnSkill(level,monsterIndex);

    Q_UNUSED(level);
    Q_UNUSED(monsterIndex);
}

bool CommonFightEngine::tryEscape()
{
    if(!canEscape())//check if is in fight
        return false;
    if(internalTryEscape())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine(QStringLiteral("escape is successful"));
        #endif
        wildMonsters.clear();
        return true;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        messageFightEngine(QStringLiteral("escape is failed"));
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return false;
    }
}

bool CommonFightEngine::canDoFightAction()
{
    if(randomSeedsSize()>5)
        return true;
    else
        return false;
}

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

void CommonFightEngine::doTheTurn(const quint32 &skill,const quint8 &skillLevel,const bool currentMonsterStatIsFirstToAttack)
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
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 0)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 0)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 4)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 4)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 5)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 5)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 3)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 3)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 3)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 3)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 6)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 6)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return;
        }
    }
    #endif
}

bool CommonFightEngine::buffIsValid(const Skill::BuffEffect &buffEffect)
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

QList<Skill::BuffEffect> CommonFightEngine::removeOldBuff(PublicPlayerMonster * playerMonster)
{
    QList<Skill::BuffEffect> returnValue;
    int index=0;
    while(index<playerMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
        if(!CommonDatapack::commonDatapack.monsterBuffs.contains(playerBuff.buff))
            playerMonster->buffs.removeAt(index);
        else
        {
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs.value(playerBuff.buff);
            if(buff.level.at(playerBuff.level-1).duration==Buff::Duration_NumberOfTurn)
            {
                if(playerMonster->buffs.at(index).remainingNumberOfTurn>0)
                    playerMonster->buffs[index].remainingNumberOfTurn--;
                if(playerMonster->buffs.at(index).remainingNumberOfTurn<=0)
                {
                    Skill::BuffEffect buffEffect;
                    buffEffect.buff=playerBuff.buff;
                    buffEffect.on=ApplyOn_Themself;
                    buffEffect.level=playerBuff.level;
                    playerMonster->buffs.removeAt(index);
                    returnValue << buffEffect;
                    continue;
                }
            }
            index++;
        }
    }
    return returnValue;
}

QList<Skill::LifeEffectReturn> CommonFightEngine::applyBuffLifeEffect(PublicPlayerMonster *playerMonster)
{
    QList<Skill::LifeEffectReturn> returnValue;
    int index=0;
    while(index<playerMonster->buffs.size())
    {
        const PlayerBuff &playerBuff=playerMonster->buffs.at(index);
        if(!CommonDatapack::commonDatapack.monsterBuffs.contains(playerBuff.buff))
            playerMonster->buffs.removeAt(index);
        else
        {
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs.value(playerBuff.buff);
            const QList<Buff::Effect> &effects=buff.level.at(playerBuff.level-1).fight;
            int sub_index=0;
            while(sub_index<effects.size())
            {
                const Buff::Effect &effect=effects.at(sub_index);
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                qDebug() << "Buff: Apply on" << playerMonster->monster << "the buff" << playerBuff.buff << "and effect on" << effect.on << "quantity" << effect.quantity << "type" << effect.type << "was hp:" << playerMonster->hp;
                #endif
                if(effect.on==Buff::Effect::EffectOn_HP)
                {
                    qint32 quantity=0;
                    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster),playerMonster->level);
                    if(effect.type==QuantityType_Quantity)
                    {
                        quantity=effect.quantity;
                        if(quantity<0 && (qint32)playerMonster->hp<=(-quantity))
                        {
                            quantity=-playerMonster->hp;
                            playerMonster->hp=0;
                            playerMonster->buffs.clear();
                        }
                        if(quantity>0 && quantity>=(qint32)(currentMonsterStat.hp-playerMonster->hp))
                        {
                            quantity=currentMonsterStat.hp-playerMonster->hp;
                            playerMonster->hp=currentMonsterStat.hp;
                        }
                    }
                    if(effect.type==QuantityType_Percent)
                    {
                        quantity=((qint64)currentMonsterStat.hp*(qint64)effect.quantity)/(qint64)100;
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
                    }
                    if(quantity<0 || quantity>0)
                    {
                        Skill::LifeEffectReturn lifeEffectReturn;
                        lifeEffectReturn.on=ApplyOn_Themself;
                        lifeEffectReturn.quantity=quantity;
                        returnValue << lifeEffectReturn;
                        playerMonster->hp+=quantity;
                    }
                }
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                qDebug() << "Buff: Apply on" << playerMonster->monster << "the buff" << playerBuff.buff << "and effect on" << effect.on << "quantity" << effect.quantity << "type" << effect.type << "new hp:" << playerMonster->hp;
                #endif
                sub_index++;
            }
            index++;
        }
    }
    return returnValue;
}

bool CommonFightEngine::useSkill(const quint32 &skill)
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
    quint8 skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.contains(skill))
            skillLevel=1;
        else
        {
            errorFightEngine(QStringLiteral("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(currentMonster->monster).arg(currentMonster->level).arg(skill));
            return false;
        }
    }
    else
        decreaseSkillEndurance(skill);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return false;
        }
    }
    #endif
    doTheTurn(skill,skillLevel,currentMonsterAttackFirst(currentMonster,otherMonster));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return false;
        }
    }
    #endif
    return true;
}

quint8 CommonFightEngine::getSkillLevel(const quint32 &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
            return currentMonster->skills.at(index).level;
        index++;
    }
    return 0;
}

quint8 CommonFightEngine::decreaseSkillEndurance(const quint32 &skill)
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return 0;
    }
    int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).skill==skill && currentMonster->skills.at(index).endurance>0)
        {
            currentMonster->skills[index].endurance--;
            return currentMonster->skills.at(index).endurance;
        }
        index++;
    }
    return 0;
}

bool CommonFightEngine::haveMoreEndurance()
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        errorFightEngine("Unable to locate the current monster");
        return false;
    }
    int index=0;
    while(index<currentMonster->skills.size())
    {
        if(currentMonster->skills.at(index).endurance>0)
            return true;
        index++;
    }
    return false;
}

bool CommonFightEngine::currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const
{
    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
    Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
    bool currentMonsterStatIsFirstToAttack=false;
    if(currentMonsterStat.speed>=otherMonsterStat.speed)
        currentMonsterStatIsFirstToAttack=true;
    return currentMonsterStatIsFirstToAttack;
}

void CommonFightEngine::startTheFight()
{
    doTurnIfChangeOfMonster=true;
    updateCanDoFight();
}

//return true if now have wild monter to fight
bool CommonFightEngine::generateWildFightIfCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const QHash<quint16,quint32> &items,const QList<quint8> &events)
{
    bool ok;
    if(isInFight())
    {
        errorFightEngine(QStringLiteral("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }

    if(map->parsed_layer.monstersCollisionMap==NULL)
    {
        /// no fight in this zone
        return false;
    }
    quint8 zoneCode=map->parsed_layer.monstersCollisionMap[x+y*map->width];
    if(zoneCode>=map->parsed_layer.monstersCollisionList.size())
    {
        errorFightEngine(QStringLiteral("error: map: %1 (%2,%3), zone code out of range").arg(map->map_file).arg(x).arg(y));
        /// no fight in this zone
        return false;
    }
    const MonstersCollisionValue &monstersCollisionValue=map->parsed_layer.monstersCollisionList.at(zoneCode);
    int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
        if(monstersCollision.item==0 || items.contains(monstersCollision.item))
        {
            if(monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.isEmpty())
            {
                /// no fight in this zone
                return false;
            }
            else
            {
                if(!ableToFight)
                {
                    errorFightEngine(QStringLiteral("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return false;
                }
                if(stepFight==0)
                {
                    if(randomSeedsSize()==0)
                    {
                        errorFightEngine(QStringLiteral("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                        return false;
                    }
                    else
                        stepFight=getOneSeed(16);
                }
                else
                    stepFight--;
                if(stepFight==0)
                {
                    const MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContent=monstersCollisionValue.walkOnMonsters.at(index);
                    QList<MapMonster> monsterList;
                    int index_condition=0;
                    while(index_condition<monstersCollisionContent.conditions.size())
                    {
                        const MonstersCollisionValue::MonstersCollisionValueOnCondition &monstersCollisionValueOnCondition=monstersCollisionContent.conditions.at(index_condition);
                        if(events.at(monstersCollisionValueOnCondition.event)==monstersCollisionValueOnCondition.event_value)
                        {
                            monsterList=monstersCollisionValueOnCondition.monsters;
                            break;
                        }
                        index_condition++;
                    }
                    if(index_condition==monstersCollisionContent.conditions.size())
                        monsterList=monstersCollisionContent.defaultMonsters;
                    if(monsterList.isEmpty())
                        return false;
                    else
                    {
                        const PlayerMonster &monster=getRandomMonster(monsterList,&ok);
                        if(ok)
                        {
                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                            messageFightEngine(QStringLiteral("Start grass fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                            #endif
                            startTheFight();
                            wildMonsters << monster;
                            return true;
                        }
                        else
                        {
                            errorFightEngine(QStringLiteral("error: no more random seed here to have the get on map %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return false;
                        }
                    }
                }
                else
                    return false;
            }
        }
        index++;
    }
    /// no fight in this zone
    return false;
}

Skill::AttackReturn CommonFightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.attack=0;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.success=false;
    PlayerMonster *otherMonster;
    if(!wildMonsters.isEmpty())
        otherMonster=&wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        otherMonster=&botFightMonsters.first();
    else
    {
        errorFightEngine("no other monster found");
        return attackReturn;
    }
    if(otherMonster->skills.empty())
    {
        if(CommonDatapack::commonDatapack.monsterSkills.contains(0))
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
    int position;
    if(otherMonster->skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster->skills.size()-1);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(position>=otherMonster->skills.size())
    {
        messageFightEngine(QStringLiteral("Position out of range: %1 on %2 total skill(s)")
                     .arg(position)
                     .arg(otherMonster->skills.size())
                     );
        position=position%otherMonster->skills.size();
    }
    #endif
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster->skills.at(position);
    messageFightEngine(QStringLiteral("Generated bot/wild attack: %1 (position: %2) at level %3 on %4 total skill(s)")
                 .arg(otherMonsterSkill.skill)
                 .arg(position)
                 .arg(otherMonsterSkill.level)
                 .arg(otherMonster->skills.size())
                 );
    attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),otherMonsterSkill.skill,otherMonsterSkill.level);
    attackReturn.doByTheCurrentMonster=false;
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const quint32 &skill, const quint8 &skillLevel)
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=true;
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
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return attackReturn;
        }
    }
    #endif
    const Skill &skillDef=CommonDatapack::commonDatapack.monsterSkills.value(skill);
    const Skill::SkillList &skillList=skillDef.level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    messageFightEngine(QStringLiteral("You use skill %1 at level %2").arg(skill).arg(skillLevel));
    #endif
    int index;
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
                    const quint32 currentMonsterHp=currentMonster->hp;
                    const quint32 otherMonsterHp=otherMonster->hp;
                    #endif
                    attackReturn.success=true;//the attack have work because at less have a buff
                    Skill::LifeEffectReturn lifeEffectReturn;
                    lifeEffectReturn=applyLifeEffect(skillDef.type,life.effect,currentMonster,otherMonster);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(lifeEffectReturn.on==ApplyOn_AllAlly || lifeEffectReturn.on==ApplyOn_Themself)
                    {
                        if(currentMonster->hp!=(currentMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine(QStringLiteral("life effect: Returned damage don't match with the real effect on current monster: %1!=(%2+%3)").arg(currentMonster->hp).arg(currentMonsterHp).arg(lifeEffectReturn.quantity));
                            return attackReturn;
                        }
                    }
                    if(lifeEffectReturn.on==ApplyOn_AllEnemy || lifeEffectReturn.on==ApplyOn_AloneEnemy)
                    {
                        if(otherMonster->hp!=(otherMonsterHp+lifeEffectReturn.quantity))
                        {
                            errorFightEngine(QStringLiteral("life effect: Returned damage don't match with the real effect on other monster: %1!=(%2+%3)").arg(otherMonster->hp).arg(otherMonsterHp).arg(lifeEffectReturn.quantity));
                            return attackReturn;
                        }
                    }
                    #endif
                    attackReturn.lifeEffectMonster << lifeEffectReturn;
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                    messageFightEngine(QStringLiteral("Add successfull buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
                #endif
            }
            if(success)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    messageFightEngine(QStringLiteral("Add buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
                #endif
                if(addBuffEffectFull(buff.effect,currentMonster,otherMonster)>=-2)//0 to X, update buff, -1 added, -2 updated same buff at same level
                {
                    attackReturn.success=true;//the attack have work because at less have a buff
                    attackReturn.addBuffEffectMonster << buff.effect;
                }
            }
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return attackReturn;
        }
    }
    #endif
    //apply the effect of current buff
    if(!genericMonsterIsKO(currentMonster))
    {
        attackReturn.removeBuffEffectMonster << removeOldBuff(currentMonster);
        const QList<Skill::LifeEffectReturn> &lifeEffectMonster=applyBuffLifeEffect(currentMonster);
        attackReturn.lifeEffectMonster << lifeEffectMonster;
    }
    if(genericMonsterIsKO(currentMonster) && !genericMonsterIsKO(otherMonster))
        doTurnIfChangeOfMonster=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //both can be KO here
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            errorFightEngine(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return attackReturn;
        }
    }
    #endif
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel)
{
    return genericMonsterAttack(getCurrentMonster(),getOtherMonster(),skill,skillLevel);
}
