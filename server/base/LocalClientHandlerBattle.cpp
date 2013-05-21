#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

bool LocalClientHandler::getInBattle()
{
    return (otherPlayerBattle!=NULL);
}

void LocalClientHandler::registerBattleRequest(LocalClientHandler * otherPlayerBattle)
{
    if(getInBattle())
    {
        emit message("Already in battle, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("%1 have requested battle with you").arg(otherPlayerBattle->player_informations->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << otherPlayerBattle->player_informations->public_and_private_informations.public_informations.skinId;
    emit sendBattleRequest(otherPlayerBattle->player_informations->rawPseudo+outputData);
}

void LocalClientHandler::battleCanceled()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleCanceled(true);
    internalBattleCanceled(true);
}

void LocalClientHandler::battleAccepted()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void LocalClientHandler::battleFinished()
{
    if(!battleIsValidated)
        return;
    if(otherPlayerBattle==NULL)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle finished");
    #endif
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    haveCurrentSkill=false;
}

void LocalClientHandler::resetTheBattle()
{
    //reset out of battle
    haveCurrentSkill=false;
    battleIsValidated=false;
    otherPlayerBattle=NULL;
    updateCanDoFight();
}

/*void LocalClientHandler::battleAddBattleMonster(const quint32 &monsterId)
{
    if(!battleIsValidated)
    {
        emit error("Battle not valid");
        return;
    }
    if(battleIsFreezed)
    {
        emit error("Battle is freezed, unable to change something");
        return;
    }
    if(player_informations->public_and_private_informations.playerMonster.size()<=1)
    {
        emit error("Unable to battle your last monster");
        return;
    }
    if(isInFight())
    {
        emit error("You can't battle monster because you are in fight");
        return;
    }

    emit error("Battle monster not found");
}*/

void LocalClientHandler::internalBattleCanceled(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        //emit message("Battle already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle canceled");
    #endif
    if(battleIsValidated)
        updateCanDoFight();
    otherPlayerBattle=NULL;
    if(send)
    {
            emit sendPacket(0xE0,0x0007);
            emit receiveSystemText(QString("Battle declined"));
    }
    battleIsValidated=false;
    haveCurrentSkill=false;
}

void LocalClientHandler::internalBattleAccepted(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        emit message("Can't accept battle if not in battle");
        return;
    }
    if(battleIsValidated)
    {
        emit message("Battle already validated");
        return;
    }
    if(!otherPlayerBattle->getAbleToFight())
    {
        emit error("The other player can't fight");
        return;
    }
    if(!getAbleToFight())
    {
        emit error("You can't fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Battle accepted");
    #endif
    battleIsValidated=true;
    haveCurrentSkill=false;
    if(send)
    {
        QList<PlayerMonster> playerMonstersPreview=otherPlayerBattle->player_informations->public_and_private_informations.playerMonster;
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerBattle->player_informations->public_and_private_informations.public_informations.skinId;
        out << (quint8)playerMonstersPreview.size();
        int index=0;
        while(index<playerMonstersPreview.size() && index<255)
        {
            if(!monsterIsKO(playerMonstersPreview.at(index)))
                out << (quint8)0x01;
            else
                out << (quint8)0x02;
            index++;
        }
        PublicPlayerMonster firstValidOtherPlayerMonster=FacilityLib::playerMonsterToPublicPlayerMonster(otherPlayerBattle->getSelectedMonster());
        out << (quint32)firstValidOtherPlayerMonster.monster;
        out << (quint8)firstValidOtherPlayerMonster.level;
        out << (quint32)firstValidOtherPlayerMonster.hp;
        out << (quint32)firstValidOtherPlayerMonster.captured_with;
        out << (quint8)firstValidOtherPlayerMonster.gender;
        out << (quint32)firstValidOtherPlayerMonster.buffs.size();
        index=0;
        while(index<firstValidOtherPlayerMonster.buffs.size())
        {
            out << (quint32)firstValidOtherPlayerMonster.buffs[index].buff;
            out << (quint8)firstValidOtherPlayerMonster.buffs[index].level;
            index++;
        }
        emit sendPacket(0xE0,0x0008,otherPlayerBattle->player_informations->rawPseudo+outputData);
    }
}

bool LocalClientHandler::haveBattleSkill()
{
    return haveCurrentSkill;
}

void LocalClientHandler::haveUsedTheBattleSkill()
{
    haveCurrentSkill=false;
}

void LocalClientHandler::useBattleSkill(const quint32 &skill,const quint8 &skillLevel)
{
    if(haveCurrentSkill)
    {
        emit error("Have already the current skill");
        return;
    }
    if(battleIsValidated)
    {
        currentSkill=skill;
        haveCurrentSkill=true;
    }
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(otherPlayerBattle==NULL)
    {
        emit error("Internal error: other player is not connected");
        return;
    }
    #endif
    if(!otherPlayerBattle->haveBattleSkill())
    {
        //in waiting of the other player skill
        return;
    }
    //calculate the result
    QList<Skill::AttackReturn> monsterReturnList;
    Monster::Stat currentMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[getSelectedMonster().monster],getSelectedMonster().level);
    Monster::Stat otherMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[otherPlayerBattle->getSelectedMonster().monster],otherPlayerBattle->getSelectedMonster().level);
    bool currentMonsterStatIsFirstToAttack;
    if(currentMonsterStat.speed==otherMonsterStat.speed)
        currentMonsterStatIsFirstToAttack=rand()%2;
    else
        currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    //do the current monster attack
    bool isKO=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=true;
        if(checkKOMonsters())
            isKO=true;
    }
    //do the other monster attack
    if(!isKO)
    {
        monsterReturnList << otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=false;
        if(checkKOMonsters())
            isKO=true;
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            monsterReturnList.last().doByTheCurrentMonster=true;
            if(checkKOMonsters())
                isKO=true;
        }
    syncForEndOfTurn();
    //send to the return
    sendBattleReturn(monsterReturnList.first(),monsterReturnList.last());
    monsterReturnList.first().doByTheCurrentMonster=!monsterReturnList.first().doByTheCurrentMonster;
    monsterReturnList.last().doByTheCurrentMonster=!monsterReturnList.last().doByTheCurrentMonster;
    otherPlayerBattle->sendBattleReturn(monsterReturnList.first(),monsterReturnList.last());
    //reset all
    haveUsedTheBattleSkill();
    otherPlayerBattle->haveUsedTheBattleSkill();
}

void LocalClientHandler::sendBattleReturn(const Skill::AttackReturn &firstAttackReturn,const Skill::AttackReturn &secondAttackReturn)
{
    int index;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint8)firstAttackReturn.doByTheCurrentMonster;
    out << (quint8)firstAttackReturn.success;
    out << (quint32)firstAttackReturn.attack;
    index=0;
    out << (quint8)firstAttackReturn.buffEffectMonster.size();
    while(index<firstAttackReturn.buffEffectMonster.size())
    {
        out << (quint32)firstAttackReturn.buffEffectMonster.at(index).buff;
        out << (quint8)firstAttackReturn.buffEffectMonster.at(index).on;
        out << (quint8)firstAttackReturn.buffEffectMonster.at(index).level;
        index++;
    }
    index=0;
    out << (quint8)firstAttackReturn.lifeEffectMonster.size();
    while(index<firstAttackReturn.lifeEffectMonster.size())
    {
        out << (qint32)firstAttackReturn.lifeEffectMonster.at(index).quantity;
        out << (quint8)firstAttackReturn.lifeEffectMonster.at(index).on;
        index++;
    }

    out << (quint8)secondAttackReturn.doByTheCurrentMonster;
    out << (quint8)secondAttackReturn.success;
    out << (quint32)secondAttackReturn.attack;
    index=0;
    out << (quint8)secondAttackReturn.buffEffectMonster.size();
    while(index<secondAttackReturn.buffEffectMonster.size())
    {
        out << (quint32)secondAttackReturn.buffEffectMonster.at(index).buff;
        out << (quint8)secondAttackReturn.buffEffectMonster.at(index).on;
        out << (quint8)secondAttackReturn.buffEffectMonster.at(index).level;
        index++;
    }
    index=0;
    out << (quint8)secondAttackReturn.lifeEffectMonster.size();
    while(index<secondAttackReturn.lifeEffectMonster.size())
    {
        out << (qint32)secondAttackReturn.lifeEffectMonster.at(index).quantity;
        out << (quint8)secondAttackReturn.lifeEffectMonster.at(index).on;
        index++;
    }

    emit sendPacket(0xE0,0x0006,outputData);
}
