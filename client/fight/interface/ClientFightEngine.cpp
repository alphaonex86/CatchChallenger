#include "ClientFightEngine.h"
#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../base/Api_client_real.h"
#include "../../general/base/FacilityLib.h"

#include <QDebug>

using namespace CatchChallenger;

ClientFightEngine ClientFightEngine::fightEngine;

ClientFightEngine::ClientFightEngine()
{
    resetAll();
    CommonFightEngine::setVariable(&player_informations);
}

ClientFightEngine::~ClientFightEngine()
{
}

bool ClientFightEngine::otherMonsterIsKO()
{
    if(wildMonsters.isEmpty() && battleCurrentMonster.isEmpty() && botFightMonsters.isEmpty())
        return true;
    if(!wildMonsters.isEmpty())
    {
        if(wildMonsters.first().hp==0)
        {
            wildMonsters.first().buffs.clear();
            return true;
        }
    }
    if(!battleCurrentMonster.isEmpty())
    {
        if(battleCurrentMonster.first().hp==0)
        {
            battleCurrentMonster.first().buffs.clear();
            return true;
        }
    }
    if(!botFightMonsters.isEmpty())
    {
        if(botFightMonsters.first().hp==0)
        {
            botFightMonsters.first().buffs.clear();
            return true;
        }
    }
    return false;
}

bool ClientFightEngine::currentMonsterIsKO()
{
    if(playerMonsterList[selectedMonster].hp==0)
    {
        playerMonsterList[selectedMonster].buffs.clear();
        return true;
    }
    return false;
}

bool ClientFightEngine::dropKOCurrentMonster()
{
    if(playerMonsterList[selectedMonster].hp==0)
    {
        playerMonsterList[selectedMonster].buffs.clear();
        updateCanDoFight();
        return true;
    }
    return false;
}

void ClientFightEngine::setBattleMonster(const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(!battleCurrentMonster.isEmpty() || !battleStat.isEmpty() || !botFightMonsters.isEmpty())
    {
        qDebug() << "have already monster to set battle monster";
        return;
    }
    if(stat.isEmpty())
    {
        qDebug() << "monster list size can't be empty";
        return;
    }
    if(monsterPlace<=0 || monsterPlace>stat.size())
    {
        qDebug() << "monsterPlace greater than monster list";
        return;
    }
    battleCurrentMonster << publicPlayerMonster;
    battleStat=stat;
    battleMonsterPlace << monsterPlace;
}

void ClientFightEngine::setBotMonster(const QList<PlayerMonster> &botFightMonsters)
{
    if(!battleCurrentMonster.isEmpty() || !battleStat.isEmpty() || !this->botFightMonsters.isEmpty())
    {
        qDebug() << "have already monster to set bot monster";
        return;
    }
    if(botFightMonsters.isEmpty())
    {
        qDebug() << "monster list size can't be empty";
        return;
    }
    this->botFightMonsters=botFightMonsters;
    int index=0;
    while(index<botFightMonsters.size())
    {
        botMonstersStat << 0x01;
        index++;
    }
}

void ClientFightEngine::addBattleMonster(const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(battleCurrentMonster.isEmpty() || battleStat.isEmpty())
    {
        qDebug() << "have already monster";
        return;
    }
    if(monsterPlace<=0 || monsterPlace>=battleStat.size())
    {
        qDebug() << "monsterPlace greater than monster list";
        return;
    }
    battleCurrentMonster << publicPlayerMonster;
    battleMonsterPlace << monsterPlace;
}

bool ClientFightEngine::haveWin()
{
    if(!wildMonsters.empty())
    {
        if(wildMonsters.size()==1)
        {
            if(wildMonsters.first().hp==0)
            {
                qDebug() << "remain one KO wild monsters";
                return true;
            }
            else
            {
                qDebug() << "remain one wild monsters";
                return false;
            }
        }
        else
        {
            qDebug() << "remain " << wildMonsters.size() << " wild monsters";
            return false;
        }
    }
    if(!botFightMonsters.empty())
    {
        if(botFightMonsters.size()==1)
        {
            if(botFightMonsters.first().hp==0)
            {
                qDebug() << "remain one KO botMonsters monsters";
                return true;
            }
            else
            {
                qDebug() << "remain one botMonsters monsters";
                return false;
            }
        }
        else
        {
            qDebug() << "remain " << botFightMonsters.size() << " botMonsters monsters";
            return false;
        }
    }
    if(!battleCurrentMonster.empty())
    {
        if(battleCurrentMonster.size()==1)
        {
            if(battleCurrentMonster.first().hp==0)
            {
                qDebug() << "remain one KO battleCurrentMonster monsters";
                return true;
            }
            else
            {
                qDebug() << "remain one battleCurrentMonster monsters";
                return false;
            }
        }
        else
        {
            qDebug() << "remain " << battleCurrentMonster.size() << " battleCurrentMonster monsters";
            return false;
        }
    }
    qDebug() << "no remaining monsters";
    return true;
}

bool ClientFightEngine::dropKOOtherMonster()
{
    if(!otherMonsterIsKO())
        return false;
    if(!wildMonsters.isEmpty())
    {
        wildMonsters.removeFirst();
        return true;
    }
    if(!botFightMonsters.isEmpty())
    {
        botFightMonsters.removeFirst();
        return true;
    }
    if(!battleCurrentMonster.isEmpty())
    {
        battleCurrentMonster.removeFirst();
        battleStat[battleMonsterPlace.first()-1]=0x02;//not able to battle
        battleMonsterPlace.removeFirst();
        return true;
    }
    return false;
}

void ClientFightEngine::finishTheBattle()
{
    wildMonsters.clear();
    botFightMonsters.clear();
    battleCurrentMonster.clear();
    battleStat.clear();
    battleMonsterPlace.clear();
}

void ClientFightEngine::healAllMonsters()
{
    int index=0;
    while(index<playerMonsterList.size())
    {
        if(playerMonsterList.at(index).egg_step==0)
            playerMonsterList[index].hp=
                    CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonsterList.at(index).monster].stat.hp*
                    playerMonsterList.at(index).level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        index++;
    }
    updateCanDoFight();
}

bool ClientFightEngine::isInFight()
{
    if(!wildMonsters.empty())
        return true;
    else if(!botFightMonsters.empty())
        return true;
    else if(!battleCurrentMonster.empty())
        return true;
    else
        return false;
}

bool ClientFightEngine::learnSkill(const quint32 &monsterId,const quint32 &skill)
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

void ClientFightEngine::setPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    this->playerMonsterList=playerMonster;
    updateCanDoFight();
}

void ClientFightEngine::addPlayerMonster(const QList<PlayerMonster> &playerMonster)
{
    playerMonsterList << playerMonster;
    updateCanDoFight();
}

QList<PlayerMonster> ClientFightEngine::getPlayerMonster()
{
    return playerMonsterList;
}

bool ClientFightEngine::removeMonster(const quint32 &monsterId)
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

void ClientFightEngine::resetAll()
{
    randomSeeds.clear();
    playerMonsterList.clear();
    ableToFight=false;

    attackReturnList.clear();

    battleCurrentMonster.clear();
    battleStat.clear();

    wildMonsters.clear();
    botFightMonsters.clear();

    CommonFightEngine::resetAll();
}

bool ClientFightEngine::tryEscape()
{
    if(wildMonsters.isEmpty())
    {
        qDebug() << "No wild monster to escape";
        return false;
    }
    if(internalTryEscape())
    {
        wildMonsters.clear();
        return true;
    }
    else
        return false;
}

bool ClientFightEngine::internalTryEscape()
{
    if(wildMonsters.isEmpty())
    {
        qDebug() << "No wild monster to internal escape";
        return false;
    }
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

void ClientFightEngine::addXPSP()
{
    if(!battleCurrentMonster.isEmpty())
    {
        emit newError(tr("Internal error"),"Don't win directly XP/SP with battle with other player");
        return;
    }
    if(wildMonsters.isEmpty() && botFightMonsters.isEmpty())
    {
        emit newError(tr("Internal error"),"No wild monster to add XP/SP");
        return;
    }
    PlayerMonster publicOtherMonster;
    if(!wildMonsters.isEmpty())
        publicOtherMonster=wildMonsters.first();
    else
        publicOtherMonster=botFightMonsters.first();
    const Monster &otherMonsterDef=CatchChallenger::CommonDatapack::commonDatapack.monsters[publicOtherMonster.monster];
    const Monster &currentMonster=CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonsterList[selectedMonster].monster];
    playerMonsterList[selectedMonster].sp+=otherMonsterDef.give_sp*publicOtherMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    quint32 give_xp=otherMonsterDef.give_xp*publicOtherMonster.level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
    quint32 xp=playerMonsterList[selectedMonster].remaining_xp;
    quint32 level=playerMonsterList[selectedMonster].level;
    while(currentMonster.level_to_xp.at(level-1)<(xp+give_xp))
    {
        quint32 old_max_hp=currentMonster.stat.hp*level/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        quint32 new_max_hp=currentMonster.stat.hp*(level+1)/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
        give_xp-=currentMonster.level_to_xp.at(level-1)-xp;
        xp=0;
        level++;
        playerMonsterList[selectedMonster].hp+=new_max_hp-old_max_hp;
        playerMonsterList[selectedMonster].level=level;
    }
    xp+=give_xp;
    playerMonsterList[selectedMonster].remaining_xp=xp;
}

bool ClientFightEngine::canDoFightAction()
{
    if(randomSeeds.size()>5)
        return true;
    else
        return false;
}

void ClientFightEngine::useSkill(const quint32 &skill)
{
    CatchChallenger::Api_client_real::client->useSkill(skill);
    if(wildMonsters.isEmpty() && botFightMonsters.isEmpty())
        return;
    if(!wildMonsters.isEmpty())
    {
        useSkillAgainstWildMonster(skill);
        return;
    }
    if(!botFightMonsters.isEmpty())
    {
        useSkillAgainstBotMonster(skill);
        return;
    }
}

void ClientFightEngine::useSkillAgainstWildMonster(const quint32 &skill)
{
    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonsterList.at(selectedMonster).monster],playerMonsterList.at(selectedMonster).level);
    const PlayerMonster &publicOtherMonster=wildMonsters.first();
    Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[publicOtherMonster.monster],publicOtherMonster.level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill);
        currentMonsterisKO=(getCurrentMonster()->hp==0);
        if(currentMonsterisKO)
        {
            qDebug() << "current player is KO";
            currentPlayerLoose=!ableToFight;
            isKO=true;
        }
        else
        {
            qDebug() << "check other player monster";
            otherMonsterisKO=otherMonsterIsKO();
            if(otherMonsterisKO)
                isKO=true;
        }
    }
    //do the other monster attack
    if(!isKO)
    {
        attackReturnList << CommonFightEngine::generateOtherAttack();
        otherMonsterisKO=(getOtherMonster()->hp==0);
        if(otherMonsterisKO)
        {
            qDebug() << "middle other player is KO";
            dropKOOtherMonster();
            otherPlayerLoose=isInFight();
            isKO=true;
        }
        else
        {
            qDebug() << "middle current player is KO";
            currentMonsterisKO=(getCurrentMonster()->hp==0);
            if(currentMonsterisKO)
                isKO=true;
        }
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            doTheCurrentMonsterAttack(skill);
            currentMonsterisKO=(getCurrentMonster()->hp==0);
            if(currentMonsterisKO)
            {
                qDebug() << "current player is KO";
                currentPlayerLoose=!ableToFight;
                isKO=true;
            }
            else
            {
                qDebug() << "check other player monster";
                otherMonsterisKO=otherMonsterIsKO();
                if(otherMonsterisKO)
                    isKO=true;
            }
        }
    if(currentPlayerLoose)
        qDebug() << "The wild monster put all your monster KO";
    if(otherPlayerLoose)
        qDebug() << "You have put KO the wild monster";
}

void ClientFightEngine::useSkillAgainstBotMonster(const quint32 &skill)
{
    Monster::Stat currentMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonsterList.at(selectedMonster).monster],playerMonsterList.at(selectedMonster).level);
    const PlayerMonster &publicOtherMonster=botFightMonsters.first();
    Monster::Stat otherMonsterStat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[publicOtherMonster.monster],publicOtherMonster.level);
    bool currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        doTheCurrentMonsterAttack(skill);
        currentMonsterisKO=(getCurrentMonster()->hp==0);
        if(currentMonsterisKO)
        {
            qDebug() << "current player is KO";
            currentPlayerLoose=!ableToFight;
            isKO=true;
        }
        else
        {
            qDebug() << "check other player monster";
            otherMonsterisKO=otherMonsterIsKO();
            if(otherMonsterisKO)
                isKO=true;
        }
    }
    //do the other monster attack
    if(!isKO)
    {
        attackReturnList << CommonFightEngine::generateOtherAttack();
        otherMonsterisKO=(getOtherMonster()->hp==0);
        if(otherMonsterisKO)
        {
            qDebug() << "middle other player is KO";
            dropKOOtherMonster();
            otherPlayerLoose=isInFight();
            isKO=true;
        }
        else
        {
            qDebug() << "middle current player is KO";
            currentMonsterisKO=(getCurrentMonster()->hp==0);
            if(currentMonsterisKO)
                isKO=true;
        }
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            doTheCurrentMonsterAttack(skill);
            currentMonsterisKO=(getCurrentMonster()->hp==0);
            if(currentMonsterisKO)
            {
                qDebug() << "current player is KO";
                currentPlayerLoose=!ableToFight;
                isKO=true;
            }
            else
            {
                qDebug() << "check other player monster";
                otherMonsterisKO=otherMonsterIsKO();
                if(otherMonsterisKO)
                    isKO=true;
            }
        }
    if(currentPlayerLoose)
        qDebug() << "The bot fight put all your monster KO";
    if(otherPlayerLoose)
        qDebug() << "You have put KO the bot fight";
}

void ClientFightEngine::doTheCurrentMonsterAttack(const quint32 &skill)
{
    #ifdef DEBUG_CLIENT_BATTLE
    qDebug() << "doTheCurrentMonsterAttack: " << skill;
    #endif
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
    attackReturn.attack=skill;
    attackReturn.doByTheCurrentMonster=true;
    attackReturn.success=false;
    const Skill::SkillList &skillList=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[playerMonsterList.at(selectedMonster).skills.at(index).skill].level.at(playerMonsterList.at(selectedMonster).skills.at(index).level-1);
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



bool ClientFightEngine::applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn)
{
    #ifdef DEBUG_CLIENT_BATTLE
    qDebug() << "applyCurrentLifeEffectReturn on " << effectReturn.on;
    #endif
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonsterList.at(selectedMonster).monster],playerMonsterList.at(selectedMonster).level);
    switch(effectReturn.on)
    {
        case ApplyOn_AloneEnemy:
        case ApplyOn_AllEnemy:
        {

            PublicPlayerMonster *publicPlayerMonster;
            if(!wildMonsters.isEmpty())
                publicPlayerMonster=&wildMonsters.first();
            else if(!botFightMonsters.isEmpty())
                publicPlayerMonster=&botFightMonsters.first();
            else if(!battleCurrentMonster.isEmpty())
                publicPlayerMonster=&battleCurrentMonster.first();
            else
            {
                emit newError(tr("Internal error"),"unknown other monster type");
                return false;
            }
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp on the ennemy " << quantity;
            if(quantity<0 && (-quantity)>(qint32)publicPlayerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is KO";
                publicPlayerMonster->hp=0;
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-publicPlayerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() ennemy is fully healled";
                publicPlayerMonster->hp=stat.hp;
            }
            else
                publicPlayerMonster->hp+=quantity;
        }
        break;
        case ApplyOn_Themself:
        case ApplyOn_AllAlly:
            quantity=effectReturn.quantity;
            qDebug() << "applyCurrentLifeEffect() add hp " << quantity;
            if(quantity<0 && (-quantity)>(qint32)playerMonsterList[selectedMonster].hp)
            {
                qDebug() << "applyCurrentLifeEffect() current monster is KO";
                playerMonsterList[selectedMonster].hp=0;
                playerMonsterList[selectedMonster].buffs.clear();
                updateCanDoFight();
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-playerMonsterList[selectedMonster].hp))
            {
                qDebug() << "applyCurrentLifeEffect() you are fully healled";
                playerMonsterList[selectedMonster].hp=stat.hp;
            }
            else
                playerMonsterList[selectedMonster].hp+=quantity;
        break;
        default:
            qDebug() << "Not apply match, can't apply the buff";
        break;
    }
    return true;
}

void ClientFightEngine::addAndApplyAttackReturnList(const QList<Skill::AttackReturn> &attackReturnList)
{
    qDebug() << "addAndApplyAttackReturnList()";
    int index=0;
    int sub_index=0;
    while(index<attackReturnList.size())
    {
        const Skill::AttackReturn &attackReturn=attackReturnList.at(index);
        if(attackReturn.success)
        {
            sub_index=0;
            while(sub_index<attackReturn.buffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.buffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                applyCurrentBuffEffect(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.lifeEffectMonster.size())
            {
                Skill::LifeEffectReturn lifeEffect=attackReturn.lifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    lifeEffect.on=invertApplyOn(lifeEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << lifeEffect.on << ", quantity:" << lifeEffect.quantity;
                if(!applyCurrentLifeEffectReturn(lifeEffect))
                {
                    emit newError(tr("Internal error"),"Error applying the life effect");
                    return;
                }
                sub_index++;
            }
        }
        index++;
    }
    this->attackReturnList << attackReturnList;
}

PublicPlayerMonster * ClientFightEngine::getOtherMonster()
{
    if(!battleCurrentMonster.isEmpty())
        return &battleCurrentMonster.first();
    return CommonFightEngine::getOtherMonster();
}

quint8 ClientFightEngine::getOtherSelectedMonsterNumber()
{
    return 0;
}

QList<Skill::AttackReturn> ClientFightEngine::getAttackReturnList() const
{
    return attackReturnList;
}

void ClientFightEngine::removeTheFirstLifeEffectAttackReturn()
{
    attackReturnList.first().lifeEffectMonster.removeFirst();
    if(attackReturnList.first().lifeEffectMonster.isEmpty())
        attackReturnList.removeFirst();
}

bool ClientFightEngine::firstLifeEffectQuantityChange(qint32 quantity)
{
    if(attackReturnList.isEmpty())
    {
        emit newError(tr("Internal error"),"try add quantity to non existant life effect");
        return false;
    }
    if(attackReturnList.first().lifeEffectMonster.isEmpty())
    {
        emit newError(tr("Internal error"),"try add quantity to life effect list empty");
        return false;
    }
    attackReturnList.first().lifeEffectMonster.first().quantity+=quantity;
    return true;
}

void ClientFightEngine::setVariable(Player_private_and_public_informations player_informations)
{
    this->player_informations=player_informations;
}
