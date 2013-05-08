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
    {
        emit error("Battle not valid");
        return;
    }

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
    QPair<LocalClientHandler::AttackReturn,LocalClientHandler::AttackReturn> currentMonsterReturn,otherMonsterReturn;
    currentMonsterReturn.first.hpChange=0;
    currentMonsterReturn.second.hpChange=0;
    otherMonsterReturn.first.hpChange=0;
    otherMonsterReturn.second.hpChange=0;
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
        currentMonsterReturn=doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        if(checkKOMonsters())
            isKO=true;
    }
    //do the other monster attack
    if(!isKO)
    {
        otherMonsterReturn=otherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        if(checkKOMonsters())
            isKO=true;
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            currentMonsterReturn=doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            if(checkKOMonsters())
                isKO=true;
        }
    //send to the return
    sendBattleReturn(currentMonsterStatIsFirstToAttack,currentMonsterReturn,otherMonsterReturn);
    otherPlayerBattle->sendBattleReturn(!currentMonsterStatIsFirstToAttack,otherMonsterReturn,currentMonsterReturn);
    //reset all
    haveUsedTheBattleSkill();
    otherPlayerBattle->haveUsedTheBattleSkill();
}

void LocalClientHandler::sendBattleReturn(const bool currentMonsterStatIsFirstToAttack,const QPair<AttackReturn,AttackReturn> &currentMonsterReturn,const QPair<AttackReturn,AttackReturn> &otherMonsterReturn)
{
    int index;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)currentMonsterStatIsFirstToAttack;
    //-----------------------------------
    out << (qint32)currentMonsterReturn.first.hpChange;
    out << (quint8)currentMonsterReturn.first.addBuff.size();
    index=0;
    while(index<currentMonsterReturn.first.addBuff.size())
    {
        out << (quint32)currentMonsterReturn.first.addBuff.at(index).buff;
        out << (quint8)currentMonsterReturn.first.addBuff.at(index).level;
        index++;
    }
    out << (quint8)currentMonsterReturn.first.removeBuff.size();
    index=0;
    while(index<currentMonsterReturn.first.removeBuff.size())
    {
        out << (quint32)currentMonsterReturn.first.removeBuff.at(index).buff;
        out << (quint8)currentMonsterReturn.first.removeBuff.at(index).level;
        index++;
    }
    //-----------------------------------
    out << (qint32)currentMonsterReturn.second.hpChange;
    out << (quint8)currentMonsterReturn.second.addBuff.size();
    index=0;
    while(index<currentMonsterReturn.second.addBuff.size())
    {
        out << (quint32)currentMonsterReturn.second.addBuff.at(index).buff;
        out << (quint8)currentMonsterReturn.second.addBuff.at(index).level;
        index++;
    }
    out << (quint8)currentMonsterReturn.second.removeBuff.size();
    index=0;
    while(index<currentMonsterReturn.second.removeBuff.size())
    {
        out << (quint32)currentMonsterReturn.second.removeBuff.at(index).buff;
        out << (quint8)currentMonsterReturn.second.removeBuff.at(index).level;
        index++;
    }
    //-----------------------------------
    out << (qint32)otherMonsterReturn.first.hpChange;
    out << (quint8)otherMonsterReturn.first.addBuff.size();
    index=0;
    while(index<otherMonsterReturn.first.addBuff.size())
    {
        out << (quint32)otherMonsterReturn.first.addBuff.at(index).buff;
        out << (quint8)otherMonsterReturn.first.addBuff.at(index).level;
        index++;
    }
    out << (quint8)otherMonsterReturn.first.removeBuff.size();
    index=0;
    while(index<otherMonsterReturn.first.removeBuff.size())
    {
        out << (quint32)otherMonsterReturn.first.removeBuff.at(index).buff;
        out << (quint8)otherMonsterReturn.first.removeBuff.at(index).level;
        index++;
    }
    //-----------------------------------
    out << (qint32)otherMonsterReturn.second.hpChange;
    out << (quint8)otherMonsterReturn.second.addBuff.size();
    index=0;
    while(index<otherMonsterReturn.second.addBuff.size())
    {
        out << (quint32)otherMonsterReturn.second.addBuff.at(index).buff;
        out << (quint8)otherMonsterReturn.second.addBuff.at(index).level;
        index++;
    }
    out << (quint8)otherMonsterReturn.second.removeBuff.size();
    index=0;
    while(index<otherMonsterReturn.second.removeBuff.size())
    {
        out << (quint32)otherMonsterReturn.second.removeBuff.at(index).buff;
        out << (quint8)otherMonsterReturn.second.removeBuff.at(index).level;
        index++;
    }
    //-----------------------------------

    emit sendPacket(0xE0,0x0006,outputData);
}
