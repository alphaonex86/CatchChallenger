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
        out << (quint8)otherPlayerBattle->getSelectedMonsterNumber();
        QByteArray firstValidOtherPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(FacilityLib::playerMonsterToPublicPlayerMonster(otherPlayerBattle->getSelectedMonster()));
        emit sendPacket(0xE0,0x0008,otherPlayerBattle->player_informations->rawPseudo+outputData+firstValidOtherPlayerMonster);
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
    LocalClientHandler * tempOtherPlayerBattle=otherPlayerBattle;
    #endif
    if(!tempOtherPlayerBattle->haveBattleSkill())
    {
        //in waiting of the other player skill
        return;
    }
    //calculate the result
    QList<Skill::AttackReturn> monsterReturnList;
    Monster::Stat currentMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[getSelectedMonster().monster],getSelectedMonster().level);
    Monster::Stat otherMonsterStat=getStat(GlobalServerData::serverPrivateVariables.monsters[tempOtherPlayerBattle->getSelectedMonster().monster],tempOtherPlayerBattle->getSelectedMonster().level);
    bool currentMonsterStatIsFirstToAttack;
    if(currentMonsterStat.speed==otherMonsterStat.speed)
        currentMonsterStatIsFirstToAttack=rand()%2;
    else
        currentMonsterStatIsFirstToAttack=(currentMonsterStat.speed>=otherMonsterStat.speed);
    //do the current monster attack
    bool isKO=false;
    bool currentMonsterisKO=false,otherMonsterisKO=false;
    bool currentPlayerLoose=false,otherPlayerLoose=false;
    if(currentMonsterStatIsFirstToAttack)
    {
        monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=true;
        currentMonsterisKO=checkKOCurrentMonsters();
        if(currentMonsterisKO)
        {
            currentPlayerLoose=checkLoose();
            isKO=true;
        }
        else
            checkKOOtherMonstersForGain();
    }
    //do the other monster attack
    if(!isKO)
    {
        monsterReturnList << tempOtherPlayerBattle->doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
        monsterReturnList.last().doByTheCurrentMonster=false;
        otherMonsterisKO=checkKOCurrentMonsters();
        if(otherMonsterisKO)
        {
            otherPlayerLoose=checkLoose();
            isKO=true;
        }
        else
            tempOtherPlayerBattle->checkKOOtherMonstersForGain();
    }
    //do the current monster attack
    if(!isKO)
        if(!currentMonsterStatIsFirstToAttack)
        {
            monsterReturnList << doTheCurrentMonsterAttack(skill,skillLevel,currentMonsterStat,otherMonsterStat);
            monsterReturnList.last().doByTheCurrentMonster=true;
            currentMonsterisKO=checkKOCurrentMonsters();
            if(currentMonsterisKO)
            {
                currentPlayerLoose=checkLoose();
                isKO=true;
            }
            else
                checkKOOtherMonstersForGain();
        }
    syncForEndOfTurn();
    //send to the return
    if(otherMonsterisKO && !otherPlayerLoose)
        sendBattleReturn(monsterReturnList,tempOtherPlayerBattle->getSelectedMonsterNumber(),playerMonsterToPublicPlayerMonster(tempOtherPlayerBattle->getSelectedMonster()));
    else
        sendBattleReturn(monsterReturnList);
    monsterReturnList.first().doByTheCurrentMonster=!monsterReturnList.first().doByTheCurrentMonster;
    monsterReturnList.last().doByTheCurrentMonster=!monsterReturnList.last().doByTheCurrentMonster;
    if(currentMonsterisKO && !currentPlayerLoose)
        tempOtherPlayerBattle->sendBattleReturn(monsterReturnList,getSelectedMonsterNumber(),playerMonsterToPublicPlayerMonster(getSelectedMonster()));
    else
        tempOtherPlayerBattle->sendBattleReturn(monsterReturnList);
    tempOtherPlayerBattle->sendBattleReturn(monsterReturnList);
    //reset all
    haveUsedTheBattleSkill();
    tempOtherPlayerBattle->haveUsedTheBattleSkill();
}

void LocalClientHandler::sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn, const quint8 &monsterPlace, const PublicPlayerMonster &publicPlayerMonster)
{
    QByteArray binarypublicPlayerMonster;
    int index,master_index;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint8)attackReturn.size();
    master_index=0;
    while(master_index<attackReturn.size())
    {
        const Skill::AttackReturn &attackReturnTemp=attackReturn.at(master_index);
        out << (quint8)attackReturnTemp.doByTheCurrentMonster;
        out << (quint8)attackReturnTemp.success;
        out << (quint32)attackReturnTemp.attack;
        index=0;
        out << (quint8)attackReturnTemp.buffEffectMonster.size();
        while(index<attackReturnTemp.buffEffectMonster.size())
        {
            out << (quint32)attackReturnTemp.buffEffectMonster.at(index).buff;
            out << (quint8)attackReturnTemp.buffEffectMonster.at(index).on;
            out << (quint8)attackReturnTemp.buffEffectMonster.at(index).level;
            index++;
        }
        index=0;
        out << (quint8)attackReturnTemp.lifeEffectMonster.size();
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            out << (qint32)attackReturnTemp.lifeEffectMonster.at(index).quantity;
            out << (quint8)attackReturnTemp.lifeEffectMonster.at(index).on;
            index++;
        }
        master_index++;
    }
    out << (quint8)monsterPlace;
    if(monsterPlace!=0x00)
        binarypublicPlayerMonster=FacilityLib::publicPlayerMonsterToBinary(publicPlayerMonster);

    emit sendPacket(0xE0,0x0006,outputData+binarypublicPlayerMonster);
}
