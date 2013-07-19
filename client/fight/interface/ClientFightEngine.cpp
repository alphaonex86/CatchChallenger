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

void ClientFightEngine::fightFinished()
{
    battleCurrentMonster.clear();
    battleStat.clear();
    battleMonsterPlace.clear();
    CommonFightEngine::fightFinished();
}

bool ClientFightEngine::isInFight() const
{
    if(CommonFightEngine::isInFight())
        return true;
    else if(!battleCurrentMonster.empty())
        return true;
    else
        return false;
}

void ClientFightEngine::resetAll()
{
    player_informations_local.playerMonster.clear();
    attackReturnList.clear();

    battleCurrentMonster.clear();
    battleStat.clear();

    CommonFightEngine::resetAll();
}

bool ClientFightEngine::internalTryEscape()
{
    if(wildMonsters.isEmpty())
    {
        qDebug() << "No wild monster to internal escape";
        return false;
    }
    qDebug() << "BaseWindow::on_toolButtonFightQuit_clicked(): emit tryEscape()";
    CatchChallenger::Api_client_real::client->tryEscape();
    return CommonFightEngine::internalTryEscape();
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
        qDebug() << "No current monster at apply current life effect";
        return false;
    }
    #ifdef DEBUG_CLIENT_BATTLE
    qDebug() << "applyCurrentLifeEffectReturn on " << effectReturn.on;
    #endif
    qint32 quantity;
    Monster::Stat stat=getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[playerMonster->monster],playerMonster->level);
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
            if(quantity<0 && (-quantity)>(qint32)playerMonster->hp)
            {
                qDebug() << "applyCurrentLifeEffect() current monster is KO";
                playerMonster->hp=0;
                playerMonster->buffs.clear();
                updateCanDoFight();
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

Skill::AttackReturn ClientFightEngine::generateOtherAttack()
{
    attackReturnList << CommonFightEngine::generateOtherAttack();
    return attackReturnList.last();
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

void ClientFightEngine::setVariable(Player_private_and_public_informations player_informations_local)
{
    this->player_informations_local=player_informations_local;
    //CommonFightEngine::setVariable(&this->player_informations_local);
    updateCanDoFight();
}
