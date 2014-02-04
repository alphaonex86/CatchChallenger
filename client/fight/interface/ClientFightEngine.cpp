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
    CommonFightEngine::setVariable(&this->player_informations_local);
}

ClientFightEngine::~ClientFightEngine()
{
}

void ClientFightEngine::setBattleMonster(const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(!battleCurrentMonster.isEmpty() || !battleStat.isEmpty() || !botFightMonsters.isEmpty())
    {
        emit error("have already monster to set battle monster");
        return;
    }
    if(stat.isEmpty())
    {
        emit error("monster list size can't be empty");
        return;
    }
    if(monsterPlace<=0 || monsterPlace>stat.size())
    {
        emit error("monsterPlace greater than monster list");
        return;
    }
    startTheFight();
    battleCurrentMonster << publicPlayerMonster;
    battleStat=stat;
    battleMonsterPlace << monsterPlace;
    mLastGivenXP=0;
}

void ClientFightEngine::setBotMonster(const QList<PlayerMonster> &botFightMonsters)
{
    if(!battleCurrentMonster.isEmpty() || !battleStat.isEmpty() || !this->botFightMonsters.isEmpty())
    {
        emit error("have already monster to set bot monster");
        return;
    }
    if(botFightMonsters.isEmpty())
    {
        emit error("monster list size can't be empty");
        return;
    }
    startTheFight();
    this->botFightMonsters=botFightMonsters;
    int index=0;
    while(index<botFightMonsters.size())
    {
        botMonstersStat << 0x01;
        index++;
    }
    mLastGivenXP=0;
}

bool ClientFightEngine::addBattleMonster(const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(battleStat.isEmpty())
    {
        emit error("not monster stat list loaded");
        return false;
    }
    if(monsterPlace<=0 || monsterPlace>battleStat.size())
    {
        emit error("monsterPlace greater than monster list");
        return false;
    }
    battleCurrentMonster << publicPlayerMonster;
    battleMonsterPlace << monsterPlace;
    return true;
}

bool ClientFightEngine::haveWin()
{
    if(!wildMonsters.empty())
    {
        if(wildMonsters.size()==1)
        {
            if(wildMonsters.first().hp==0)
            {
                emit message("remain one KO wild monsters");
                return true;
            }
            else
            {
                emit message("remain one wild monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 wild monsters").arg(wildMonsters.size()));
            return false;
        }
    }
    if(!botFightMonsters.empty())
    {
        if(botFightMonsters.size()==1)
        {
            if(botFightMonsters.first().hp==0)
            {
                emit message("remain one KO botMonsters monsters");
                return true;
            }
            else
            {
                emit message("remain one botMonsters monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 bot monsters").arg(botFightMonsters.size()));
            return false;
        }
    }
    if(!battleCurrentMonster.empty())
    {
        if(battleCurrentMonster.size()==1)
        {
            if(battleCurrentMonster.first().hp==0)
            {
                emit message("remain one KO battleCurrentMonster monsters");
                return true;
            }
            else
            {
                emit message("remain one battleCurrentMonster monsters");
                return false;
            }
        }
        else
        {
            emit message(QStringLiteral("remain %1 battle monsters").arg(battleCurrentMonster.size()));
            return false;
        }
    }
    emit message("no remaining monsters");
    return true;
}

bool ClientFightEngine::dropKOOtherMonster()
{
    bool commonReturn=CommonFightEngine::dropKOOtherMonster();

    bool battleReturn=false;
    if(!battleCurrentMonster.isEmpty())
    {
        battleCurrentMonster.removeFirst();
        battleStat[battleMonsterPlace.first()-1]=0x02;//not able to battle
        battleMonsterPlace.removeFirst();
        battleReturn=true;
    }
    bool haveValidMonster=false;
    int index=0;
    while(index<battleStat.size())
    {
        if(battleStat.at(index)==0x01)
        {
            haveValidMonster=true;
            break;
        }
        index++;
    }
    if(!haveValidMonster)
        battleStat.clear();

    return commonReturn || battleReturn;
}

void ClientFightEngine::fightFinished()
{
    battleCurrentMonster.clear();
    battleStat.clear();
    battleMonsterPlace.clear();
    doTurnIfChangeOfMonster=true;
    CommonFightEngine::fightFinished();
}

bool ClientFightEngine::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    else if(!battleStat.empty())
        return true;
    else
        return false;
}

void ClientFightEngine::resetAll()
{
    mLastGivenXP=0;

    player_informations_local.playerMonster.clear();
    attackReturnList.clear();

    battleCurrentMonster.clear();
    battleStat.clear();

    CommonFightEngine::resetAll();
}

bool ClientFightEngine::internalTryEscape()
{
    emit message("BaseWindow::on_toolButtonFightQuit_clicked(): emit tryEscape()");
    CatchChallenger::Api_client_real::client->tryEscape();
    return CommonFightEngine::internalTryEscape();
}

void ClientFightEngine::tryCatchClient(const quint32 &item)
{
    emit message("ClientFightEngine::tryCapture(): emit tryCapture()");
    CatchChallenger::Api_client_real::client->useObject(item);
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
    playerMonster_catchInProgress << newMonster;
}

quint32 ClientFightEngine::catchAWild(const bool &toStorage, const PlayerMonster &newMonster)
{
    Q_UNUSED(toStorage);
    Q_UNUSED(newMonster);
    return 0;
}

Skill::AttackReturn ClientFightEngine::doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel)
{
    attackReturnList << CommonFightEngine::doTheCurrentMonsterAttack(skill,skillLevel);
    return attackReturnList.last();
}

bool ClientFightEngine::applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn)
{
    PlayerMonster * playerMonster=getCurrentMonster();
    if(playerMonster==NULL)
    {
        emit message("No current monster at apply current life effect");
        return false;
    }
    #ifdef DEBUG_CLIENT_BATTLE
    emit message("applyCurrentLifeEffectReturn on: "+QString::number(effectReturn.on));
    #endif
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(playerMonster->monster),playerMonster->level);
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
                ableToFight=false;
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
            if(quantity<0 && (-quantity)>(qint32)playerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() current monster is KO";
                playerMonster->hp=0;
            }
            else if(quantity>0 && quantity>(qint32)(stat.hp-playerMonster->hp))
            {
                qDebug() << "applyCurrentLifeEffect() you are fully healled";
                playerMonster->hp=stat.hp;
            }
            else
                playerMonster->hp+=quantity;
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
            while(sub_index<attackReturn.addBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.addBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                addCurrentBuffEffect(buff);
                sub_index++;
            }
            sub_index=0;
            while(sub_index<attackReturn.removeBuffEffectMonster.size())
            {
                Skill::BuffEffect buff=attackReturn.removeBuffEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buff.on=invertApplyOn(buff.on);
                qDebug() << "addAndApplyAttackReturnList() buff on " << buff.on << ", buff:" << buff.buff << ", buff level:" << buff.level;
                removeBuffEffectFull(buff);
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
            sub_index=0;
            while(sub_index<attackReturn.buffLifeEffectMonster.size())
            {
                Skill::LifeEffectReturn buffEffect=attackReturn.buffLifeEffectMonster.at(sub_index);
                if(!attackReturn.doByTheCurrentMonster)
                    buffEffect.on=invertApplyOn(buffEffect.on);
                qDebug() << "addAndApplyAttackReturnList() life effect on " << buffEffect.on << ", quantity:" << buffEffect.quantity;
                if(!applyCurrentLifeEffectReturn(buffEffect))
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

PublicPlayerMonster * ClientFightEngine::getOtherMonster() const
{
    if(!battleCurrentMonster.isEmpty())
        return (PublicPlayerMonster *)&battleCurrentMonster.first();
    return CommonFightEngine::getOtherMonster();
}

quint8 ClientFightEngine::getOtherSelectedMonsterNumber() const
{
    return 0;
}

QList<Skill::AttackReturn> ClientFightEngine::getAttackReturnList() const
{
    return attackReturnList;
}

Skill::AttackReturn ClientFightEngine::getFirstAttackReturn() const
{
    return attackReturnList.first();
}

Skill::AttackReturn ClientFightEngine::generateOtherAttack()
{
    attackReturnList << CommonFightEngine::generateOtherAttack();
    return attackReturnList.last();
}

void ClientFightEngine::removeTheFirstLifeEffectAttackReturn()
{
    if(attackReturnList.isEmpty())
        return;
    if(!attackReturnList.first().lifeEffectMonster.isEmpty())
        attackReturnList.first().lifeEffectMonster.removeFirst();
}

void ClientFightEngine::removeTheFirstBuffEffectAttackReturn()
{
    if(attackReturnList.isEmpty())
        return;
    if(!attackReturnList.first().buffLifeEffectMonster.isEmpty())
        attackReturnList.first().buffLifeEffectMonster.removeFirst();
}

void ClientFightEngine::removeTheFirstAddBuffEffectAttackReturn()
{
    if(attackReturnList.isEmpty())
        return;
    if(!attackReturnList.first().addBuffEffectMonster.isEmpty())
        attackReturnList.first().addBuffEffectMonster.removeFirst();
}

void ClientFightEngine::removeTheFirstRemoveBuffEffectAttackReturn()
{
    if(attackReturnList.isEmpty())
        return;
    if(!attackReturnList.first().removeBuffEffectMonster.isEmpty())
        attackReturnList.first().removeBuffEffectMonster.removeFirst();
}

void ClientFightEngine::removeTheFirstAttackReturn()
{
    if(!attackReturnList.isEmpty())
        attackReturnList.removeFirst();
}

bool ClientFightEngine::firstAttackReturnHaveMoreEffect()
{
    if(attackReturnList.isEmpty())
        return false;
    if(!attackReturnList.first().lifeEffectMonster.isEmpty())
        return true;
    if(!attackReturnList.first().addBuffEffectMonster.isEmpty())
        return true;
    if(!attackReturnList.first().removeBuffEffectMonster.isEmpty())
        return true;
    if(!attackReturnList.first().buffLifeEffectMonster.isEmpty())
        return true;
    return false;
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

void ClientFightEngine::setVariableContent(Player_private_and_public_informations player_informations_local)
{
    this->player_informations_local=player_informations_local;
    updateCanDoFight();
}

bool ClientFightEngine::isInBattle() const
{
    return !battleStat.isEmpty();
}

bool ClientFightEngine::haveBattleOtherMonster() const
{
    return !battleCurrentMonster.isEmpty();
}

bool ClientFightEngine::useSkill(const quint32 &skill)
{
    mLastGivenXP=0;
    Api_client_real::client->useSkill(skill);
    if(isInBattle())
        return true;
    return CommonFightEngine::useSkill(skill);
}

void ClientFightEngine::catchIsDone()
{
    wildMonsters.removeFirst();
}

bool ClientFightEngine::doTheOtherMonsterTurn()
{
    if(!isInBattle())
        return CommonFightEngine::doTheOtherMonsterTurn();
    return true;
}

void ClientFightEngine::levelUp(const quint8 &level, const quint8 &monsterIndex)
{
    const PlayerMonster &monster=player_informations->playerMonster.at(monsterIndex);
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster.monster);
    int index=0;
    while(index<monsterInformations.evolutions.size())
    {
        const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
        if(evolution.type==Monster::EvolutionType_Level && evolution.level==level)//the current monster is not updated
        {
            mEvolutionByLevelUp << monsterIndex;
            return;
        }
        index++;
    }
}

PlayerMonster * ClientFightEngine::evolutionByLevelUp()
{
    if(mEvolutionByLevelUp.isEmpty())
        return NULL;
    quint8 monsterIndex=mEvolutionByLevelUp.first();
    mEvolutionByLevelUp.removeFirst();
    return &player_informations->playerMonster[monsterIndex];
}

void ClientFightEngine::confirmEvolution(const quint32 &monterId)
{
    CatchChallenger::Api_client_real::client->confirmEvolution(monterId);
    int index=0;
    while(index<player_informations->playerMonster.size())
    {
        if(player_informations->playerMonster.at(index).id==monterId)
        {
            const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters[player_informations->playerMonster.at(index).monster];
            int sub_index=0;
            while(sub_index<monsterInformations.evolutions.size())
            {
                if(monsterInformations.evolutions.at(sub_index).type==Monster::EvolutionType_Level)
                {
                    player_informations->playerMonster[index].monster=monsterInformations.evolutions.at(sub_index).evolveTo;
                    Monster::Stat stat=getStat(monsterInformations,player_informations->playerMonster[index].level);
                    if(player_informations->playerMonster[index].hp>stat.hp)
                        player_informations->playerMonster[index].hp=stat.hp;
                    return;
                }
                sub_index++;
            }
            qDebug() << "Evolution not found";
            return;
        }
        index++;
    }
    qDebug() << "Monster for evolution not found";
}

//return true if change level, multiplicator do at datapack loading
bool ClientFightEngine::giveXPSP(int xp,int sp)
{
    bool haveChangeOfLevel=CommonFightEngine::giveXPSP(xp,sp);
    mLastGivenXP=xp;
    return haveChangeOfLevel;
}

quint32 ClientFightEngine::lastGivenXP()
{
    quint32 tempLastGivenXP=mLastGivenXP;
    mLastGivenXP=0;
    return tempLastGivenXP;
}
