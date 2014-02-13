#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"

#include <QtMath>

using namespace CatchChallenger;

CommonFightEngine::CommonFightEngine()
{
    resetAll();
    player_informations=NULL;
}

bool CommonFightEngine::canEscape() const
{
    if(!isInFight())//check if is in fight
    {
        emit error(QStringLiteral("error: tryEscape() when is not in fight"));
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        emit message("You can't escape because it's not a wild monster");
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
    randomSeeds.clear();
    selectedMonster=0;
    doTurnIfChangeOfMonster=true;
}

bool CommonFightEngine::otherMonsterIsKO() const
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

bool CommonFightEngine::currentMonsterIsKO() const
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
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).egg_step==0)
        {
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(player_informations->playerMonster.at(index).monster),player_informations->playerMonster.at(index).level);
            player_informations->playerMonster[index].hp=stat.hp;
            player_informations->playerMonster[index].buffs.clear();
            int sub_index=0;
            while(sub_index<player_informations->playerMonster.value(index).skills.size())
            {
                player_informations->playerMonster[index].skills[sub_index].endurance=
                        CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(
                        player_informations->playerMonster[index].skills.at(sub_index).skill
                        )
                        .level.at(player_informations->playerMonster.value(index).skills.at(sub_index).level-1).endurance;
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
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &monster=player_informations->playerMonster.at(index);
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
                            emit error(QStringLiteral("Skill to learn not found into learnSkill()"));
                            return false;
                        }
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.size()>learn.learnSkillLevel)
                        {
                            emit error(QStringLiteral("Skill level to learn not found learnSkill()"));
                            return false;
                        }
                        quint32 sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel-1).sp_to_learn;
                        if(sp>monster.sp)
                            return false;
                        player_informations->playerMonster[index].sp-=sp;
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(learn.learnSkill).level.at(learn.learnSkillLevel-1).endurance;
                            addSkill(&player_informations->playerMonster[index],temp);
                        }
                        else
                            setSkillLevel(&player_informations->playerMonster[index],sub_index2,player_informations->playerMonster.at(index).skills.at(sub_index2).level+1);
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
        emit error(QStringLiteral("Monster id not found into learnSkillByItem()"));
        return false;
    }
    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster).learnByItem.contains(itemId))
    {
        emit error(QStringLiteral("Item id not found into learnSkillByItem()"));
        return false;
    }
    const Monster::AttackToLearnByItem &attackToLearnByItem=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster).learnByItem.value(itemId);
    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(attackToLearnByItem.learnSkill))
    {
        emit error(QStringLiteral("Skill to learn not found into learnSkill()"));
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(attackToLearnByItem.learnSkill).level.size()>attackToLearnByItem.learnSkillLevel)
    {
        emit error(QStringLiteral("Skill level to learn not found learnSkill()"));
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

void CommonFightEngine::newRandomNumber(const QByteArray &randomData)
{
    randomSeeds+=randomData;
}

void CommonFightEngine::setVariable(Player_private_and_public_informations *player_informations)
{
    this->player_informations=player_informations;
}

bool CommonFightEngine::getAbleToFight() const
{
    return ableToFight;
}

bool CommonFightEngine::haveMonsters() const
{
    return !player_informations->playerMonster.isEmpty();
}

//return is have random seed to do random step
bool CommonFightEngine::canDoRandomFight(const CommonMap &map,const quint8 &x,const quint8 &y) const
{
    if(isInFight())
    {
        emit message(QStringLiteral("map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y));
        return false;
    }
    if(map.parsed_layer.monstersCollisionMap==NULL)
        return true;

    quint8 zoneCode=map.parsed_layer.monstersCollisionMap[x+y*map.width];
    const MonstersCollisionValue &monstersCollisionValue=map.parsed_layer.monstersCollisionList.at(zoneCode);
    if(monstersCollisionValue.walkOn.isEmpty())
        return true;
    else
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    emit message(QStringLiteral("map: %1 (%2,%3), no fight in this zone").arg(map.map_file).arg(x).arg(y));
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
    bool monsterFound=false;
    int index=0;
    while(index<monsterList.size())
    {
        int luck=monsterList.at(index).luck;
        if(randomMonsterInt<=luck)// with < it crash because randomMonsterInt can be 0
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
        if(monsterList.isEmpty())
        {
            emit error(QStringLiteral("error: no wild monster selected, with: randomMonsterInt: %1").arg(randomMonsterInt));
            *ok=false;
            playerMonster.monster=0;
            playerMonster.level=0;
            playerMonster.gender=Gender_Unknown;
            return playerMonster;
        }
        else
        {
            emit message(QStringLiteral("error: no wild monster selected, with: randomMonsterInt: %1").arg(randomMonsterInt));
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
    index=monsterDef.learn.size()-1;
    while(index>=0 && playerMonster.skills.size()<CATCHCHALLENGER_MONSTER_WILD_SKILL_NUMBER)
    {
        if(monsterDef.learn.at(index).learnAtLevel<=playerMonster.level)
        {
            PlayerMonster::PlayerSkill temp;
            temp.level=monsterDef.learn.at(index).learnSkillLevel;
            temp.skill=monsterDef.learn.at(index).learnSkill;
            temp.endurance=CommonDatapack::commonDatapack.monsterSkills.value(temp.skill).level.at(temp.level-1).endurance;
            playerMonster.skills << temp;
        }
        index--;
    }
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
        emit error(QStringLiteral("Can't auto select the next monster when is in battle"));
        return;
    }
    ableToFight=false;
    if(player_informations==NULL)
    {
        emit error(QStringLiteral("player_informations is NULL"));
        return;
    }
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
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
    if(player_informations==NULL)
        return false;
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
        if(!monsterIsKO(playerMonsterEntry))
            return true;
        index++;
    }
    return false;
}

bool CommonFightEngine::haveAnotherEnnemyMonsterToFight() const
{
    if(!wildMonsters.isEmpty())
        return false;
    if(!botFightMonsters.isEmpty())
        return botFightMonsters.size()>1;
    emit error("Unable to locate the other monster");
    return false;
}

PlayerMonster * CommonFightEngine::getCurrentMonster() const
{
    if(player_informations==NULL)
    {
        emit error(QStringLiteral("player_informations is NULL"));
        return NULL;
    }
    int playerMonsterSize=player_informations->playerMonster.size();
    if(selectedMonster>=0 && selectedMonster<playerMonsterSize)
        return &player_informations->playerMonster[selectedMonster];
    else
    {
        emit error(QStringLiteral("selectedMonster is out of range, max: %1").arg(playerMonsterSize));
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

PublicPlayerMonster *CommonFightEngine::getOtherMonster() const
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
    if(player_informations==NULL)
    {
        emit error("player_informations is NULL");
        return false;
    }
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
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
            emit error("Not apply match, can't apply the buff");
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
            float OtherMulti=1.0;
            /*if(type==commentMonster.type)
                OtherMulti*=1.45;*/
            OtherMulti*=1.24;
            effect_to_return.effective=1.0;
            const QList<quint8> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster).type;
            if(type!=255 && !typeList.isEmpty())
            {
                int index=0;
                while(index<typeList.size())
                {
                    const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(type);
                    if(typeDefinition.multiplicator.contains(typeList.at(index)))
                    {
                        const qint8 &multiplicator=typeDefinition.multiplicator.value(typeList.at(index));
                        if(multiplicator>0)
                            effect_to_return.effective*=multiplicator;
                        else
                            effect_to_return.effective/=-multiplicator;
                    }
                    index++;
                }
            }
            float criticalHit=1.0;
            effect_to_return.critical=(getOneSeed(255)<20);
            if(effect_to_return.critical)
                criticalHit=1.5;
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
            quantity = effect_to_return.effective*(((float)currentMonster->level*(float)1.99+10.5)/(float)255*((float)attack/(float)defense)*(float)effect.quantity)*criticalHit*OtherMulti*(100-getOneSeed(17))/100;
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
        emit error("Wrong calculated value");
    }
    if(effect.quantity>0 && quantity<0)
    {
        quantity=1;
        emit error("Wrong calculated value");
    }
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
        emit error(QStringLiteral("apply a unknown buff"));
        return -4;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.size())
    {
        emit error(QStringLiteral("apply buff level out of range"));
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
            emit error("Not apply match, can't apply the buff");
            return -4;
        break;
    }
    return -4;
}

void CommonFightEngine::removeBuffEffectFull(const Skill::BuffEffect &effect)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(effect.buff))
    {
        emit error(QStringLiteral("apply a unknown buff"));
        return;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs.value(effect.buff).level.size())
    {
        emit error(QStringLiteral("apply buff level out of range"));
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
                emit error("Other monster not found for buff remove");
                return;
            }
            while(index<otherMonster->buffs.size())
            {
                if(otherMonster->buffs.at(index).buff==effect.buff)
                {
                    if(otherMonster->buffs.at(index).level!=effect.level)
                        emit message(QStringLiteral("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(otherMonster->buffs.at(index).level).arg(effect.level));
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
                emit error("Other monster not found for buff remove");
                return;
            }
            while(index<currentMonster->buffs.size())
            {
                if(currentMonster->buffs.at(index).buff==effect.buff)
                {
                    if(currentMonster->buffs.at(index).level!=effect.level)
                        emit message(QStringLiteral("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(currentMonster->buffs.at(index).level).arg(effect.level));
                    currentMonster->buffs.removeAt(index);
                    return;
                }
                index++;
            }
        }
        break;
        default:
            emit error("Not apply match, can't apply the buff");
            return;
        break;
    }
    emit error("Buff not found to remove");
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
        emit error(QStringLiteral("have not this monster: %1").arg(object));
        return false;
    }
    PlayerMonster * playerMonster=monsterById(monster);
    if(genericMonsterIsKO(playerMonster))
    {
        emit error(QStringLiteral("can't be applyied on KO monster: %1").arg(object));
        return false;
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(object))
    {
        if(isInFight())
        {
            emit error(QStringLiteral("this item %1 can't be used in fight").arg(object));
            return false;
        }
        if(!CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(object).contains(playerMonster->monster))
        {
            emit error(QStringLiteral("this item %1 can't be applied on this monster %2").arg(object).arg(playerMonster->monster));
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
                        emit message(QStringLiteral("Item %1 have unknown effect").arg(object));
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
                emit error(QStringLiteral("this item %1 can't be used in fight").arg(object));
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
                            emit error(QStringLiteral("this item %1 can't be used on monster at level max").arg(object));
                            return false;
                        }
                        addLevel(playerMonster);
                    break;
                    default:
                        emit message(QStringLiteral("Item %1 have unknown effect").arg(object));
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
            emit error(QStringLiteral("this item %1 can't be used in fight").arg(object));
            return false;
        }
        if(!CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.value(object).contains(playerMonster->monster))
        {
            emit error(QStringLiteral("this item %1 can't be applied on this monster %2").arg(object).arg(playerMonster->monster));
            return false;
        }
        return learnSkillByItem(playerMonster,object);
    }
    else
    {
        emit error(QStringLiteral("This object can't be applied on monster: %1").arg(object));
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
    if(player_informations==NULL)
    {
        emit error(QStringLiteral("player_informations is NULL"));
        return false;
    }
    if(getCurrentMonster()->id==monsterId)
    {
        emit error(QStringLiteral("try change monster but is already on the current monster"));
        return false;
    }
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &playerMonsterEntry=player_informations->playerMonster.at(index);
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
    emit error(QStringLiteral("unable to locate the new monster to change"));
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

quint8 CommonFightEngine::getOneSeed(const quint8 &max)
{
    quint8 number=static_cast<quint8>(randomSeeds.at(0));
    randomSeeds.remove(0,1);
    return number%(max+1);
}

bool CommonFightEngine::internalTryEscape()
{
    doTurnIfChangeOfMonster=true;
    quint8 value=getOneSeed(101);
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        emit error("No current monster to try escape");
        return false;
    }
    if(wildMonsters.isEmpty())
    {
        emit error("Not againts wild monster");
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
        emit error("Not againts wild monster");
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
                    emit error(QStringLiteral("Buff level for wild monter not found: %1 at level %2").arg(playerBuff.buff).arg(playerBuff.level));
                    bonusStat+=1.0;
                }
            }
            else
            {
                emit error(QStringLiteral("Buff for wild monter not found: %1").arg(playerBuff.buff));
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

    emit message(QStringLiteral("Formule: (%1*(%2-%3)*%4*%5)/(%6), realRate: %7")
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
        emit message(QStringLiteral("capture is successful"));
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
        newMonster.id=catchAWild(player_informations->playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER,newMonster);
        return newMonster.id;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QStringLiteral("capture is failed"));
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return 0;
    }
}

void CommonFightEngine::fightFinished()
{
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        int sub_index=0;
        while(sub_index<player_informations->playerMonster.at(index).buffs.size())
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.contains(player_informations->playerMonster.at(index).buffs.at(sub_index).buff))
            {
                const PlayerBuff &playerBuff=player_informations->playerMonster.at(index).buffs.at(sub_index);
                if(CommonDatapack::commonDatapack.monsterBuffs.value(playerBuff.buff).level.at(playerBuff.level-1).duration!=Buff::Duration_Always)
                    player_informations->playerMonster[index].buffs.removeAt(sub_index);
                else
                    sub_index++;
            }
            else
                player_informations->playerMonster[index].buffs.removeAt(sub_index);
        }
        index++;
    }
    wildMonsters.clear();
    botFightMonsters.clear();
    doTurnIfChangeOfMonster=true;
}

void CommonFightEngine::addPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    player_informations->playerMonster << playerMonster;
    updateCanDoFight();
}

void CommonFightEngine::addPlayerMonster(const PlayerMonster &playerMonster)
{
    player_informations->playerMonster << playerMonster;
    updateCanDoFight();
}

void CommonFightEngine::insertPlayerMonster(const quint8 &place,const PlayerMonster &playerMonster)
{
    player_informations->playerMonster.insert(place,playerMonster);
    updateCanDoFight();
}

QList<PlayerMonster> CommonFightEngine::getPlayerMonster() const
{
    return player_informations->playerMonster;
}

bool CommonFightEngine::moveUpMonster(const quint8 &number)
{
    if(isInFight())
        return false;
    if(number==0)
        return false;
    if(number>=(player_informations->playerMonster.size()))
        return false;
    player_informations->playerMonster.swap(number,number-1);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::moveDownMonster(const quint8 &number)
{
    if(isInFight())
        return false;
    if(number>=(player_informations->playerMonster.size()-1))
        return false;
    player_informations->playerMonster.swap(number,number+1);
    updateCanDoFight();
    return true;
}

bool CommonFightEngine::removeMonster(const quint32 &monsterId)
{
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).id==monsterId)
        {
            player_informations->playerMonster.removeAt(index);
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
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).id==monsterId)
            return true;
        index++;
    }
    return false;
}

PlayerMonster * CommonFightEngine::monsterById(const quint32 &monsterId) const
{
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).id==monsterId)
            return &player_informations->playerMonster[index];
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
    emit message(QStringLiteral("checkKOOtherMonstersForGain()"));
    bool winTheTurn=false;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QStringLiteral("The wild monster (%1) is KO").arg(wildMonsters.first().monster));
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
            emit message(QStringLiteral("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
        }
    }
    else if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QStringLiteral("The wild monster (%1) is KO").arg(botFightMonsters.first().monster));
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &botmonster=CommonDatapack::commonDatapack.monsters.value(botFightMonsters.first().monster);
            //multiplicator do at datapack loading
            int sp=botmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=botmonster.give_xp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QStringLiteral("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
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
        emit error("unknown other monster type");
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
        emit message(QStringLiteral("You pass to the level %1").arg(level));
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
        emit error(QStringLiteral("Monster is already at the level max into addLevel()"));
        return false;
    }
    int monsterIndex=0;
    const int &monsterListSize=player_informations->playerMonster.size();
    while(monsterIndex<monsterListSize)
    {
        if(&player_informations->playerMonster.at(monsterIndex)==monster)
            break;
        monsterIndex++;
    }
    if(monsterIndex==monsterListSize)
    {
        emit error(QStringLiteral("Monster index not for to add level"));
        return false;
    }
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster->monster);
    const quint8 &level=player_informations->playerMonster.at(monsterIndex).level;
    const quint32 &old_max_hp=getStat(monsterInformations,level).hp;
    const quint32 &new_max_hp=getStat(monsterInformations,level+1).hp;
    levelUp(level,monsterIndex);
    if(new_max_hp>old_max_hp)
        monster->hp+=new_max_hp-old_max_hp;
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QStringLiteral("You pass to the level %1 on your monster %2").arg(level).arg(monster->id));
    #endif

    if((player_informations->playerMonster.value(monsterIndex).level+numberOfLevel)>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        player_informations->playerMonster[monsterIndex].level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    else
        player_informations->playerMonster[monsterIndex].level+=numberOfLevel;
    player_informations->playerMonster[monsterIndex].remaining_xp=0;
    return true;
}

void CommonFightEngine::levelUp(const quint8 &level,const quint8 &monsterIndex)
{
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
        emit message(QStringLiteral("escape is successful"));
        #endif
        wildMonsters.clear();
        return true;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QStringLiteral("escape is failed"));
        #endif
        generateOtherAttack();//Skill::AttackReturn attackReturn=
        return false;
    }
}

bool CommonFightEngine::canDoFightAction()
{
    if(randomSeeds.size()>5)
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
            emit error("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 0)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 0)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            emit error("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 4)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 4)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            emit error("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 5)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 5)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 3)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use doTheTurn() at 3)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Unable to locate the current monster");
                return;
            }
            if(currentMonsterIsKO())
            {
                emit error("Can't attack with KO monster");
                return;
            }
            const PublicPlayerMonster * otherMonster=getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("Unable to locate the other monster");
                return;
            }
            Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
            Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
            if(currentMonster->hp>currentMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 3)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
                return;
            }
            if(otherMonster->hp>otherMonsterStat.hp)
            {
                emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 after the skill use doTheTurn() at 3)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            emit error("Unable to locate the current monster");
            return;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 6)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use doTheTurn() at 6)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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

QList<Skill::LifeEffectReturn> CommonFightEngine::buffLifeEffect(PublicPlayerMonster *playerMonster)
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
                    }
                }
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
        emit error("Try use skill when not in fight");
        return false;
    }
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        emit error("Unable to locate the current monster");
        return false;
    }
    if(currentMonsterIsKO())
    {
        emit error("Can't attack with KO monster");
        return false;
    }
    const PublicPlayerMonster * otherMonster=getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("Unable to locate the other monster");
        return false;
    }
    quint8 skillLevel=getSkillLevel(skill);
    if(skillLevel==0)
    {
        if(!haveMoreEndurance() && skill==0 && CommonDatapack::commonDatapack.monsterSkills.contains(skill))
            skillLevel=1;
        else
        {
            emit error(QStringLiteral("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(currentMonster->monster).arg(currentMonster->level).arg(skill));
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
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 before the skill use").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 before the skill use").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return false;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return false;
        }
    }
    #endif
    return true;
}

quint8 CommonFightEngine::getSkillLevel(const quint32 &skill) const
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        emit error("Unable to locate the current monster");
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
        emit error("Unable to locate the current monster");
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

bool CommonFightEngine::haveMoreEndurance() const
{
    PlayerMonster * currentMonster=getCurrentMonster();
    if(currentMonster==NULL)
    {
        emit error("Unable to locate the current monster");
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
bool CommonFightEngine::generateWildFightIfCollision(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const QHash<quint32,quint32> &items)
{
    bool ok;
    if(isInFight())
    {
        emit error(QStringLiteral("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
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
        emit error(QStringLiteral("error: map: %1 (%2,%3), zone code out of range").arg(map->map_file).arg(x).arg(y));
        /// no fight in this zone
        return false;
    }
    const MonstersCollisionValue &monstersCollisionValue=map->parsed_layer.monstersCollisionList.at(zoneCode);
    QMapIterator<quint32/*item*/, MonstersCollisionValueMonster> i(monstersCollisionValue.walkOn);
    while (i.hasNext()) {
        i.next();
        if(i.key()==0 || items.contains(i.key()))
        {
            if(i.value().monsters.isEmpty())
            {
                /// no fight in this zone
                return false;
            }
            else
            {
                if(!ableToFight)
                {
                    emit error(QStringLiteral("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return false;
                }
                if(stepFight==0)
                {
                    if(randomSeeds.size()==0)
                    {
                        emit error(QStringLiteral("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                        return false;
                    }
                    else
                        stepFight=getOneSeed(16);
                }
                else
                    stepFight--;
                if(stepFight==0)
                {
                    const PlayerMonster &monster=getRandomMonster(i.value().monsters,&ok);
                    if(ok)
                    {
                        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                        emit message(QStringLiteral("Start grass fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                        #endif
                        startTheFight();
                        wildMonsters << monster;
                    }
                    else
                        emit error(QStringLiteral("error: no more random seed here to have the get"));
                    return ok;
                }
                else
                    return false;
            }
        }
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
        emit error("no other monster found");
        return attackReturn;
    }
    if(otherMonster->skills.empty())
    {
        if(CommonDatapack::commonDatapack.monsterSkills.contains(0))
        {
            emit message("Generated bot/wild default attack");
            attackReturn=genericMonsterAttack(otherMonster,getCurrentMonster(),0,1);
            attackReturn.doByTheCurrentMonster=false;
            return attackReturn;
        }
        else
        {
            emit message("No other monster attack todo");
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
        emit message(QStringLiteral("Position out of range: %1 on %2 total skill(s)")
                     .arg(position)
                     .arg(otherMonster->skills.size())
                     );
        position=position%otherMonster->skills.size();
    }
    #endif
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster->skills.at(position);
    emit message(QStringLiteral("Generated bot/wild attack: %1 (position: %2) at level %3 on %4 total skill(s)")
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
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            emit error("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return attackReturn;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 1)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 1)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return attackReturn;
        }
    }
    #endif
    const Skill &skillDef=CommonDatapack::commonDatapack.monsterSkills.value(skill);
    const Skill::SkillList &skillList=skillDef.level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QStringLiteral("You use skill %1 at level %2").arg(skill).arg(skillLevel));
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
                            emit error(QStringLiteral("life effect: Returned damage don't match with the real effect on current monster: %1!=(%2+%3)").arg(currentMonster->hp).arg(currentMonsterHp).arg(lifeEffectReturn.quantity));
                            return attackReturn;
                        }
                    }
                    if(lifeEffectReturn.on==ApplyOn_AllEnemy || lifeEffectReturn.on==ApplyOn_AloneEnemy)
                    {
                        if(otherMonster->hp!=(otherMonsterHp+lifeEffectReturn.quantity))
                        {
                            emit error(QStringLiteral("life effect: Returned damage don't match with the real effect on other monster: %1!=(%2+%3)").arg(otherMonster->hp).arg(otherMonsterHp).arg(lifeEffectReturn.quantity));
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
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            emit error("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return attackReturn;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 2)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
                emit error("Buff is not valid");
                return tempReturnBuff;
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
                    emit message(QStringLiteral("Add successfull buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
                #endif
            }
            if(success)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    emit message(QStringLiteral("Add buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
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
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            emit error("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return attackReturn;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 3)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 3)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
            return attackReturn;
        }
    }
    #endif
    //apply the effect of current buff
    if(!genericMonsterIsKO(currentMonster))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //don't use pointer  here because the value of currentMonster->hp will change
        quint32 currentMonsterHp=currentMonster->hp;
        quint32 otherMonsterHp=otherMonster->hp;
        #endif
        attackReturn.removeBuffEffectMonster << removeOldBuff(currentMonster);
        QList<Skill::LifeEffectReturn> lifeEffectMonster=buffLifeEffect(currentMonster);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        int index=0;
        while(index<lifeEffectMonster.size())
        {
            if(lifeEffectMonster.at(index).on==ApplyOn_AllAlly || lifeEffectMonster.at(index).on==ApplyOn_Themself)
                currentMonsterHp+=lifeEffectMonster.at(index).quantity;
            if(lifeEffectMonster.at(index).on==ApplyOn_AllEnemy || lifeEffectMonster.at(index).on==ApplyOn_AloneEnemy)
                otherMonsterHp+=lifeEffectMonster.at(index).quantity;
            index++;
        }
        if(currentMonster->hp!=currentMonsterHp)
        {
            emit error(QStringLiteral("buff effect: Returned damage don't match with the real effect on current monster: %1!=(%2+%3)").arg(currentMonster->hp).arg(currentMonsterHp).arg(lifeEffectMonster.at(index).quantity));
            return attackReturn;
        }
        if(otherMonster->hp!=otherMonsterHp)
        {
            emit error(QStringLiteral("buff effect: Returned damage don't match with the real effect on other monster: %1!=(%2+%3)").arg(otherMonster->hp).arg(otherMonsterHp).arg(lifeEffectMonster.at(index).quantity));
            return attackReturn;
        }
        #endif
        attackReturn.lifeEffectMonster << lifeEffectMonster;
    }
    if(genericMonsterIsKO(currentMonster) && !genericMonsterIsKO(otherMonster))
        doTurnIfChangeOfMonster=false;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        PlayerMonster * currentMonster=getCurrentMonster();
        if(currentMonster==NULL)
        {
            emit error("Unable to locate the current monster");
            return attackReturn;
        }
        if(currentMonsterIsKO())
        {
            emit error("Can't attack with KO monster");
            return attackReturn;
        }
        const PublicPlayerMonster * otherMonster=getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("Unable to locate the other monster");
            return attackReturn;
        }
        Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonster->level);
        Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        if(currentMonster->hp>currentMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of current monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 4)").arg(currentMonster->hp).arg(currentMonster->monster).arg(currentMonster->level).arg(currentMonsterStat.hp));
            return attackReturn;
        }
        if(otherMonster->hp>otherMonsterStat.hp)
        {
            emit error(QStringLiteral("The hp %1 of other monster %2 at level %3 is greater than the max %4 the skill use genericMonsterAttack() at 4)").arg(otherMonster->hp).arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonsterStat.hp));
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
