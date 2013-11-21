#include "CommonFightEngine.h"
#include "../base/CommonDatapack.h"

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
        emit error(QString("error: tryEscape() when is not in fight"));
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
    stepFight_Grass=0;
    stepFight_Water=0;
    stepFight_Cave=0;
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
            const Monster::Stat &stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster.at(index).monster],player_informations->playerMonster.at(index).level);
            player_informations->playerMonster[index].hp=stat.hp;
            player_informations->playerMonster[index].buffs.clear();
            int sub_index=0;
            while(sub_index<player_informations->playerMonster[index].skills.size())
            {
                player_informations->playerMonster[index].skills[sub_index].endurance=
                        CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[
                        player_informations->playerMonster[index].skills.at(sub_index).skill
                        ]
                        .level.at(player_informations->playerMonster[index].skills.at(sub_index).level-1).endurance;
                sub_index++;
            }
        }
        index++;
    }
    if(!isInBattle())
        updateCanDoFight();
}

bool CommonFightEngine::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    int index=0;
    int sub_index2,sub_index;
    while(index<player_informations->playerMonster.size())
    {
        const PlayerMonster &monster=player_informations->playerMonster.at(index);
        if(monster.id==monsterId)
        {
            sub_index2=0;
            while(sub_index2<monster.skills.size())
            {
                if(monster.skills.at(sub_index2).skill==skill)
                    break;
                sub_index2++;
            }
            sub_index=0;
            while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster].learn.size())
            {
                const Monster::AttackToLearn &learn=CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster].learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level && learn.learnSkill==skill)
                {
                    if(
                            //if skill not found
                            (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                            ||
                            //if skill already found and need level up
                            (sub_index2<monster.skills.size() && (monster.skills[sub_index2].level+1)==learn.learnSkillLevel)
                            )
                    {
                        quint32 sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).sp_to_learn;
                        if(sp>monster.sp)
                            return false;
                        player_informations->playerMonster[index].sp-=sp;
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
                            temp.endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).endurance;
                            player_informations->playerMonster[index].skills << temp;
                        }
                        else
                            player_informations->playerMonster[index].skills[sub_index2].level++;
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
bool CommonFightEngine::canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y) const
{
    if(isInFight())
    {
        emit message(QString("map: %1 (%2,%3), is in fight").arg(map.map_file).arg(x).arg(y));
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(map,x,y) && !map.grassMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(CatchChallenger::MoveOnTheMap::isWater(map,x,y) && !map.waterMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;
    if(!map.caveMonster.empty())
        return randomSeeds.size()>=CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT;

    /// no fight in this zone
    emit message(QString("map: %1 (%2,%3), no fight in this zone").arg(map.map_file).arg(x).arg(y));
    return true;
}

PlayerMonster CommonFightEngine::getRandomMonster(const QList<MapMonster> &monsterList,bool *ok)
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
        emit error(QString("error: no wild monster selected, with: randomMonsterInt: %1").arg(randomMonsterInt));
        if(monsterList.isEmpty())
        {
            *ok=false;
            playerMonster.monster=0;
            playerMonster.level=0;
            playerMonster.gender=Gender_Unknown;
            return playerMonster;
        }
        else
        {
            playerMonster.monster=monsterList.first().id;
            //select the level
            if(monsterList.at(index).maxLevel==monsterList.first().minLevel)
                playerMonster.level=monsterList.first().minLevel;
            else
                playerMonster.level=getOneSeed(monsterList.first().maxLevel-monsterList.first().minLevel+1)+monsterList.first().minLevel;
        }
    }
    Monster monsterDef=CommonDatapack::commonDatapack.monsters[playerMonster.monster];
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
            temp.endurance=CommonDatapack::commonDatapack.monsterSkills[temp.skill].level.at(temp.level-1).endurance;
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
        emit error(QString("Can't auto select the next monster when is in battle"));
        return;
    }
    ableToFight=false;
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
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
        emit error(QString("player_informations is NULL"));
        return NULL;
    }
    int playerMonsterSize=player_informations->playerMonster.size();
    if(selectedMonster>=0 && selectedMonster<playerMonsterSize)
        return &player_informations->playerMonster[selectedMonster];
    else
    {
        emit error(QString("selectedMonster is out of range, max: %1").arg(playerMonsterSize));
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
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[currentMonster->monster],currentMonster->level);
    Monster::Stat otherStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
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
    if(effect.type==QuantityType_Quantity)
    {
        if(effect.quantity<0)
            quantity=-((-effect.quantity*stat.attack)/(otherStat.defense*4));
        else if(effect.quantity>0)//ignore the def for heal
            quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        const QList<quint8> &typeList=CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster].type;
        if(type!=255 && !typeList.isEmpty())
        {
            int index=0;
            while(index<typeList.size())
            {
                const Type &typeDefinition=CatchChallenger::CommonDatapack::commonDatapack.types.at(typeList.at(index));
                if(typeDefinition.multiplicator.contains(type))
                {
                    const qint8 &multiplicator=typeDefinition.multiplicator[type];
                    if(multiplicator>=0)
                        quantity*=multiplicator;
                    else
                        quantity/=-multiplicator;
                }
                index++;
            }
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
    Skill::LifeEffectReturn effect_to_return;
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
        emit error(QString("apply a unknown buff"));
        return -4;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs[effect.buff].level.size())
    {
        emit error(QString("apply buff level out of range"));
        return -4;
    }
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    tempBuff.remainingNumberOfTurn=CommonDatapack::commonDatapack.monsterBuffs[effect.buff].durationNumberOfTurn;
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
        emit error(QString("apply a unknown buff"));
        return;
    }
    if(effect.level>CommonDatapack::commonDatapack.monsterBuffs[effect.buff].level.size())
    {
        emit error(QString("apply buff level out of range"));
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
                        emit message(QString("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(otherMonster->buffs.at(index).level).arg(effect.level));
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
                        emit message(QString("the buff removed %1 have not the same level %2!=%3").arg(effect.buff).arg(currentMonster->buffs.at(index).level).arg(effect.level));
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

bool CommonFightEngine::changeOfMonsterInFight(const quint32 &monsterId)
{
    if(!isInFight())
        return false;
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        return false;
    }
    if(getCurrentMonster()->id==monsterId)
    {
        emit error(QString("try change monster but is already on the current monster"));
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
    emit error(QString("unable to locate the new monster to change"));
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
    const Monster::Stat &wildMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[wildMonsters.first().monster],wildMonsters.first().level);
    if(wildMonsterStat.hp>2)
        if(wildMonsters.first().hp>=(wildMonsterStat.hp/2))
            return false;
    if(wildMonsters.first().level<=trap.min_level)
        return true;
    if(wildMonsters.first().level>=trap.max_level)
        return false;

    //use real random to prevent cheats
    quint8 number=rand()%(trap.max_level-trap.min_level);

    if(number<(trap.max_level-wildMonsters.first().level))
        return true;
    else
        return false;
}

quint32 CommonFightEngine::tryCapture(const quint32 &item)
{
    doTurnIfChangeOfMonster=true;
    if(internalTryCapture(CommonDatapack::commonDatapack.items.trap[item]))
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("capture is successful"));
        #endif
        PlayerMonster newMonster;
        newMonster.buffs=wildMonsters.first().buffs;
        newMonster.captured_with=item;
        newMonster.egg_step=0;
        newMonster.gender=wildMonsters.first().gender;
        newMonster.hp=wildMonsters.first().hp;
        newMonster.id=0;//unknown at this time
        newMonster.level=wildMonsters.first().level;
        newMonster.monster=wildMonsters.first().monster;
        newMonster.remaining_xp=0;
        newMonster.skills=wildMonsters.first().skills;
        newMonster.sp=0;
        return captureAWild(player_informations->playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER,newMonster);
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("capture is failed"));
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
                if(CommonDatapack::commonDatapack.monsterBuffs[player_informations->playerMonster.at(index).buffs.at(sub_index).buff].duration!=Buff::Duration_Always)
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

void CommonFightEngine::wildDrop(const quint32 &monster)
{
    Q_UNUSED(monster);
}

bool CommonFightEngine::checkKOOtherMonstersForGain()
{
    bool winTheTurn=false;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("The wild monster (%1) is KO").arg(wildMonsters.first().monster));
            #endif
            //drop the drop item here
            wildDrop(wildMonsters.first().monster);
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters[wildMonsters.first().monster];
            int sp=wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=wildmonster.give_xp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*wildMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
            #endif
        }
    }
    else if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            winTheTurn=true;
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("The wild monster (%1) is KO").arg(botFightMonsters.first().monster));
            #endif
            //don't drop item because it's not a wild fight
            //give xp/sp here
            const Monster &wildmonster=CommonDatapack::commonDatapack.monsters[botFightMonsters.first().monster];
            int sp=wildmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            int xp=wildmonster.give_xp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
            giveXPSP(xp,sp);
            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
            emit message(QString("You win %1 xp and %2 sp").arg(give_xp).arg(wildmonster.give_sp*botFightMonsters.first().level/CATCHCHALLENGER_MONSTER_LEVEL_MAX));
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

//return true if change level
bool CommonFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=false;
    PlayerMonster * monster=getCurrentMonster();
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters[monster->monster];
    quint32 remaining_xp=monster->remaining_xp;
    quint8 level=monster->level;
    if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        remaining_xp=0;
        xp=0;
    }
    while(monsterInformations.level_to_xp.at(level-1)<(remaining_xp+xp))
    {
        quint32 old_max_hp=monsterInformations.stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        quint32 new_max_hp=monsterInformations.stat.hp*(level+1)/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        xp-=monsterInformations.level_to_xp.at(level-1)-remaining_xp;
        remaining_xp=0;
        level++;
        levelUp(level,getCurrentSelectedMonsterNumber());
        haveChangeOfLevel=true;
        monster->hp+=new_max_hp-old_max_hp;
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("You pass to the level %1").arg(level));
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
        emit message(QString("escape is successful"));
        #endif
        wildMonsters.clear();
        return true;
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("escape is failed"));
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
    bool turnIsEnd=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill,skillLevel);
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
    //do the other monster attack
    if(!turnIsEnd)
    {
        if(doTheOtherMonsterTurn())
            turnIsEnd=true;
    }
    //do the current monster attack
    if(!turnIsEnd && !currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill,skillLevel);
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
}

bool CommonFightEngine::buffIsValid(const Skill::BuffEffect &buffEffect)
{
    if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buffEffect.buff))
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level<=0)
        return false;
    if(buffEffect.level>CommonDatapack::commonDatapack.monsterBuffs[buffEffect.buff].level.size())
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
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs[playerBuff.buff];
            if(buff.duration==Buff::Duration_NumberOfTurn)
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
            const Buff &buff=CommonDatapack::commonDatapack.monsterBuffs[playerBuff.buff];
            const QList<Buff::Effect> &effects=buff.level.at(playerBuff.level-1).fight;
            int sub_index=0;
            while(sub_index<effects.size())
            {
                const Buff::Effect &effect=effects.at(sub_index);
                if(effect.on==Buff::Effect::EffectOn_HP)
                {
                    qint32 quantity=0;
                    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonster->monster],playerMonster->level);
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
            emit error(QString("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(currentMonster->monster).arg(currentMonster->level).arg(skill));
            return false;
        }
    }
    else
        decreaseSkillEndurance(skill);
    doTheTurn(skill,skillLevel,currentMonsterAttackFirst(currentMonster,otherMonster));
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
    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[currentMonster->monster],currentMonster->level);
    Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
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
bool CommonFightEngine::generateWildFightIfCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    bool ok;
    if(isInFight())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(CatchChallenger::MoveOnTheMap::isGrass(*map,x,y) && !map->grassMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Grass==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Grass=getOneSeed(16);
        }
        else
            stepFight_Grass--;
        if(stepFight_Grass==0)
        {
            PlayerMonster monster=getRandomMonster(map->grassMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start grass fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                startTheFight();
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
    }
    if(CatchChallenger::MoveOnTheMap::isWater(*map,x,y) && !map->waterMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Water==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Water=getOneSeed(16);
        }
        else
            stepFight_Water--;
        if(stepFight_Water==0)
        {
            PlayerMonster monster=getRandomMonster(map->waterMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start water fight with monster id %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                startTheFight();
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
    }
    if(!map->caveMonster.empty())
    {
        if(!ableToFight)
        {
            emit error(QString("LocalClientHandlerFight::singleMove(), can't walk into the grass into map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return false;
        }
        if(stepFight_Cave==0)
        {
            if(randomSeeds.size()==0)
            {
                emit error(QString("error: no more random seed here, map: %1 (%2,%3), is in fight").arg(map->map_file).arg(x).arg(y));
                return false;
            }
            else
                stepFight_Cave=getOneSeed(16);
        }
        else
            stepFight_Cave--;
        if(stepFight_Cave==0)
        {
            PlayerMonster monster=getRandomMonster(map->caveMonster,&ok);
            if(ok)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                emit message(QString("Start cave fight with monseter id: %1 level %2").arg(monster.monster).arg(monster.level));
                #endif
                startTheFight();
                wildMonsters << monster;
            }
            else
                emit error(QString("error: no more random seed here to have the get"));
            return ok;
        }
        else
            return false;
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
        emit message(QString("Position out of range: %1 on %2 total skill(s)")
                     .arg(position)
                     .arg(otherMonster->skills.size())
                     );
        position=position%otherMonster->skills.size();
    }
    #endif
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster->skills.at(position);
    emit message(QString("Generated bot/wild attack: %1 (position: %2) at level %3 on %4 total skill(s)")
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
    const Skill &skillDef=CommonDatapack::commonDatapack.monsterSkills[skill];
    const Skill::SkillList &skillList=skillDef.level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QString("You use skill %1 at level %2").arg(skill).arg(skillLevel));
    #endif
    int index;
    //do the skill
    if(!genericMonsterIsKO(currentMonster))
    {
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
                attackReturn.success=true;//the attack have work because at less have a buff
                Skill::LifeEffectReturn lifeEffectReturn;
                lifeEffectReturn.on=life.effect.on;
                lifeEffectReturn.quantity=applyLifeEffect(skillDef.type,life.effect,currentMonster,otherMonster).quantity;
                attackReturn.lifeEffectMonster << lifeEffectReturn;
            }
            index++;
        }
    }
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
                    emit message(QString("Add successfull buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
                #endif
            }
            if(success)
            {
                #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
                if(success)
                    emit message(QString("Add buff: %1 at level: %2 on %3").arg(buff.effect.buff).arg(buff.effect.level).arg(buff.effect.on));
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
    //apply the buff
    if(!genericMonsterIsKO(currentMonster))
    {
        attackReturn.removeBuffEffectMonster << removeOldBuff(currentMonster);
        attackReturn.lifeEffectMonster << buffLifeEffect(currentMonster);
    }
    if(genericMonsterIsKO(currentMonster) && !genericMonsterIsKO(otherMonster))
        doTurnIfChangeOfMonster=false;
    return attackReturn;
}

Skill::AttackReturn CommonFightEngine::doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel)
{
    return genericMonsterAttack(getCurrentMonster(),getOtherMonster(),skill,skillLevel);
}
