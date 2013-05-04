#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

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
        PlayerMonster firstValidOtherPlayerMonster=otherPlayerBattle->getFirstValidMonster();
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
