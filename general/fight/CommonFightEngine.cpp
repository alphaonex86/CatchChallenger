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
}

bool CommonFightEngine::otherMonsterIsKO()
{
    PublicPlayerMonster * publicPlayerMonster=getOtherMonster();
    if(publicPlayerMonster==NULL)
        return true;
    if(publicPlayerMonster->hp==0)
    {
        publicPlayerMonster->buffs.clear();
        return true;
    }
    return false;
}

bool CommonFightEngine::currentMonsterIsKO()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
        return true;
    if(playerMonster->hp==0)
    {
        playerMonster->buffs.clear();
        return true;
    }
    return false;
}

bool CommonFightEngine::dropKOCurrentMonster()
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
        return true;
    if(playerMonster->hp==0)
    {
        playerMonster->buffs.clear();
        updateCanDoFight();
        return true;
    }
    return false;
}

void CommonFightEngine::healAllMonsters()
{
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).egg_step==0)
            player_informations->playerMonster[index].hp=
                    CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster.at(index).monster].stat.hp*
                    player_informations->playerMonster.at(index).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        index++;
    }
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
                    if((sub_index2==monster.skills.size() && learn.learnSkillLevel==1) || (monster.skills[sub_index2].level+1)==learn.learnSkillLevel)
                    {
                        quint32 sp=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[learn.learnSkill].level.at(learn.learnSkillLevel).sp;
                        if(sp>monster.sp)
                            return false;
                        player_informations->playerMonster[index].sp-=sp;
                        if(learn.learnSkillLevel==1)
                        {
                            PlayerMonster::PlayerSkill temp;
                            temp.skill=skill;
                            temp.level=1;
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
        emit error(QString("error: no wild monster selected"));
        *ok=false;
        playerMonster.monster=0;
        playerMonster.level=0;
        playerMonster.gender=Gender_Unknown;
        return playerMonster;
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
            playerMonster.skills << temp;
        }
        index--;
    }
    *ok=true;
    return playerMonster;
}

/** \warning you need check before the input data */
Monster::Stat CommonFightEngine::getStat(const Monster &monster, const quint8 &level)
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

void CommonFightEngine::updateCanDoFight()
{
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

Skill::AttackReturn CommonFightEngine::generateOtherAttack()
{
    Skill::AttackReturn attackReturn;
    attackReturn.doByTheCurrentMonster=false;
    attackReturn.success=false;
    PlayerMonster otherMonster;
    if(!wildMonsters.isEmpty())
        otherMonster=wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        otherMonster=botFightMonsters.first();
    else
    {
        emit error("no other monster found");
        return attackReturn;
    }
    if(otherMonster.skills.empty())
        return attackReturn;
    int position;
    if(otherMonster.skills.size()==1)
        position=0;
    else
        position=getOneSeed(otherMonster.skills.size());
    const PlayerMonster::PlayerSkill &otherMonsterSkill=otherMonster.skills.at(position);
    attackReturn.attack=otherMonsterSkill.skill;
    const Skill::SkillList &skillList=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[otherMonsterSkill.skill].level.at(otherMonsterSkill.level-1);
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
    return attackReturn;
}

Skill::LifeEffectReturn CommonFightEngine::applyOtherLifeEffect(const Skill::LifeEffect &effect)
{
    PlayerMonster *otherMonster;
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    if(!wildMonsters.isEmpty())
        otherMonster=&wildMonsters.first();
    else if(!botFightMonsters.isEmpty())
        otherMonster=&botFightMonsters.first();
    else
    {
        emit error(QString("Unable to locate the other monster to generate other attack"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster[selectedMonster].monster],player_informations->playerMonster[selectedMonster].level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*otherMonster->level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->playerMonster[selectedMonster].hp*effect.quantity)/100;
            //kill
            if(quantity<0 && (-quantity)>=(qint32)player_informations->playerMonster[selectedMonster].hp)
            {
                player_informations->playerMonster[selectedMonster].hp=0;
                player_informations->playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            //full heal
            else if(quantity>0 && quantity>=(qint32)(stat.hp-player_informations->playerMonster[selectedMonster].hp))
                player_informations->playerMonster[selectedMonster].hp=stat.hp;
            //other life change
            else
                player_informations->playerMonster[selectedMonster].hp+=quantity;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*otherMonster->level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*otherMonster->level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(otherMonster->hp*effect.quantity)/100;
            //kill
            if(quantity<0 && (-quantity)>=(qint32)otherMonster->hp)
                otherMonster->hp=0;
            //full heal
            else if(quantity>0 && quantity>=(qint32)(stat.hp-otherMonster->hp))
                otherMonster->hp=stat.hp;
            //other life change
            else
                otherMonster->hp+=quantity;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void CommonFightEngine::applyOtherBuffEffect(const Skill::BuffEffect &effect)
{
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        return;
    }
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            player_informations->playerMonster[selectedMonster].buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(wildMonsters.isEmpty())
                wildMonsters.first().buffs << tempBuff;
            else if(botFightMonsters.isEmpty())
                botFightMonsters.first().buffs << tempBuff;
            else
            {
                emit error(QString("Unable to locate the other monster to apply other buff effect"));
                return;
            }
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
}

Skill::LifeEffectReturn CommonFightEngine::applyCurrentLifeEffect(const Skill::LifeEffect &effect)
{
    if(player_informations==NULL)
    {
        emit error(QString("player_informations is NULL"));
        Skill::LifeEffectReturn effect_to_return;
        effect_to_return.on=effect.on;
        effect_to_return.quantity=0;
        return effect_to_return;
    }
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[player_informations->playerMonster.at(selectedMonster).monster],player_informations->playerMonster.at(selectedMonster).level);
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {

            PublicPlayerMonster *publicPlayerMonster=getOtherMonster();
            if(effect.type==QuantityType_Quantity)
            {
                Monster::Stat otherStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[publicPlayerMonster->monster],publicPlayerMonster->level);
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->playerMonster.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*otherStat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->playerMonster.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(publicPlayerMonster->hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)publicPlayerMonster->hp)
                publicPlayerMonster->hp=0;
            else if(quantity>0 && quantity>(qint32)(stat.hp-publicPlayerMonster->hp))
                publicPlayerMonster->hp=stat.hp;
            else
                publicPlayerMonster->hp+=quantity;
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            if(effect.type==QuantityType_Quantity)
            {
                if(effect.quantity<0)
                {
                    quantity=-((-effect.quantity*stat.attack*player_informations->playerMonster.at(selectedMonster).level)/(CATCHCHALLENGER_MONSTER_LEVEL_MAX*stat.defense));
                    if(quantity==0)
                        quantity=-1;
                }
                else if(effect.quantity>0)//ignore the def for heal
                {
                    quantity=effect.quantity*player_informations->playerMonster.at(selectedMonster).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                    if(quantity==0)
                        quantity=1;
                }
            }
            else
                quantity=(player_informations->playerMonster[selectedMonster].hp*effect.quantity)/100;
            if(quantity<0 && (-quantity)>(qint32)player_informations->playerMonster[selectedMonster].hp)
            {
                player_informations->playerMonster[selectedMonster].hp=0;
                player_informations->playerMonster[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-player_informations->playerMonster[selectedMonster].hp))
                player_informations->playerMonster[selectedMonster].hp=stat.hp;
            else
                player_informations->playerMonster[selectedMonster].hp+=quantity;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
    Skill::LifeEffectReturn effect_to_return;
    effect_to_return.on=effect.on;
    effect_to_return.quantity=quantity;
    return effect_to_return;
}

void CommonFightEngine::applyCurrentBuffEffect(const Skill::BuffEffect &effect)
{
    PlayerBuff tempBuff;
    tempBuff.buff=effect.buff;
    tempBuff.level=effect.level;
    switch(effect.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
            getOtherMonster()->buffs << tempBuff;
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            getCurrentMonster()->buffs << tempBuff;
        break;
        default:
            emit error("Not apply match, can't apply the buff");
        break;
    }
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
    quint16 number=static_cast<quint8>(randomSeeds.at(0));
    if(max!=0)
    {
        number*=max;
        number/=255;
    }
    randomSeeds.remove(0,1);
    return number;
}

bool CommonFightEngine::internalTryEscape()
{
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

void CommonFightEngine::fightFinished()
{
    wildMonsters.clear();
    botFightMonsters.clear();
}

void CommonFightEngine::addPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    player_informations->playerMonster << playerMonster;
    updateCanDoFight();
}

QList<PlayerMonster> CommonFightEngine::getPlayerMonster() const
{
    return player_informations->playerMonster;
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
    if(!botFightMonsters.isEmpty())
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
    else
    {
        emit message("unknown other monster type");
        return false;
    }
    return winTheTurn;
}

//return true if change level
bool CommonFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=false;
    const Monster &currentmonster=CommonDatapack::commonDatapack.monsters[getCurrentMonster()->monster];
    quint32 remaining_xp=getCurrentMonster()->remaining_xp;
    quint32 level=getCurrentMonster()->level;
    if(level>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        remaining_xp=0;
        xp=0;
    }
    while(currentmonster.level_to_xp.at(level-1)<(remaining_xp+xp))
    {
        quint32 old_max_hp=currentmonster.stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        quint32 new_max_hp=currentmonster.stat.hp*(level+1)/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        xp-=currentmonster.level_to_xp.at(level-1)-remaining_xp;
        remaining_xp=0;
        level++;
        haveChangeOfLevel=true;
        getCurrentMonster()->hp+=new_max_hp-old_max_hp;
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
    getCurrentMonster()->remaining_xp=remaining_xp;
    if(haveChangeOfLevel)
        getCurrentMonster()->level=level;
    getCurrentMonster()->sp+=sp;

    if(!isInFight())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message("You win the battle");
        #endif
    }
    return haveChangeOfLevel;
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
        generateOtherAttack();
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
    //do the current monster attack
    if(!turnIsEnd)
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

Skill::AttackReturn CommonFightEngine::doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel)
{
    /// \todo use the variable currentMonsterStat and otherMonsterStat to have better speed
    Skill::AttackReturn tempReturnBuff;
    tempReturnBuff.doByTheCurrentMonster=true;
    tempReturnBuff.attack=skill;
    const Skill::SkillList &skillList=CommonDatapack::commonDatapack.monsterSkills[skill].level.at(skillLevel-1);
    #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
    emit message(QString("You use skill %1 at level %2").arg(skill).arg(skillLevel));
    #endif
    int index=0;
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
            tempReturnBuff.success=true;
            tempReturnBuff.buffEffectMonster << buff.effect;
            applyCurrentBuffEffect(buff.effect);
        }
        index++;
    }
    index=0;
    while(index<skillList.life.size())
    {
        tempReturnBuff.success=true;
        const Skill::Life &life=skillList.life.at(index);
        bool success;
        if(life.success==100)
            success=true;
        else
            success=(getOneSeed(100)<life.success);
        if(success)
        {
            Skill::LifeEffectReturn lifeEffectReturn;
            lifeEffectReturn.on=life.effect.on;
            lifeEffectReturn.quantity=applyCurrentLifeEffect(life.effect).quantity;
            tempReturnBuff.lifeEffectMonster << lifeEffectReturn;
        }
        index++;
    }
    return tempReturnBuff;
}

bool CommonFightEngine::useSkill(const quint32 &skill)
{
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
    int index=0;
    while(index<getCurrentMonster()->skills.size())
    {
        if(getCurrentMonster()->skills.at(index).skill==skill)
            break;
        index++;
    }
    if(index==getCurrentMonster()->skills.size())
    {
        emit error(QString("Unable to fight because the current monster (%1, level: %2) have not the skill %3").arg(getCurrentMonster()->monster).arg(getCurrentMonster()->level).arg(skill));
        return false;
    }
    const PublicPlayerMonster * otherMonster=getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("Unable to locate the other monster");
        return false;
    }
    quint8 skillLevel=getCurrentMonster()->skills.at(index).level;
    doTheTurn(skill,skillLevel,currentMonsterAttackFirst(currentMonster,otherMonster));
    return true;
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
