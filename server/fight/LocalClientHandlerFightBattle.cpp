#include "../base/Client.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../base/Client.hpp"

using namespace CatchChallenger;

Client *Client::getOtherPlayerBattle() const
{
    return otherPlayerBattle;
}

bool Client::isInBattle() const
{
    return (otherPlayerBattle!=NULL && battleIsValidated);
}

void Client::registerBattleRequest(Client *otherPlayerBattle)
{
    if(isInBattle())
    {
        normalOutput("Already in battle, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(otherPlayerBattle->public_and_private_informations.public_informations.pseudo+" have requested battle with you");
    #endif
    this->otherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->otherPlayerBattle=this;

    //send the network reply
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xDF;
    posOutput+=1+1+4;

    {
        const std::string &text=otherPlayerBattle->public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=otherPlayerBattle->public_and_private_informations.public_informations.skinId;
    posOutput+=1;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    sendBattleRequest(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::battleCanceled()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleCanceled(true);
    internalBattleCanceled(true);
}

void Client::battleAccepted()
{
    if(otherPlayerBattle!=NULL)
        otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void Client::battleFakeAccepted(Client *otherPlayer)
{
    battleFakeAcceptedInternal(otherPlayer);
    otherPlayer->battleFakeAcceptedInternal(this);
    otherPlayerBattle->internalBattleAccepted(true);
    internalBattleAccepted(true);
}

void Client::battleFakeAcceptedInternal(Client * otherPlayer)
{
    this->otherPlayerBattle=otherPlayer;
}

void Client::battleFinished()
{
    if(!battleIsValidated)
        return;
    if(otherPlayerBattle==NULL)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle finished");
    #endif
    otherPlayerBattle->resetBattleAction();
    resetBattleAction();
    Client *tempOtherPlayerBattle=otherPlayerBattle;
    otherPlayerBattle->battleFinishedReset();
    battleFinishedReset();
    updateCanDoFight();
    tempOtherPlayerBattle->updateCanDoFight();
}

void Client::battleFinishedReset()
{
    otherPlayerBattle=NULL;
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

void Client::resetTheBattle()
{
    //reset out of battle
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    battleIsValidated=false;
    otherPlayerBattle=NULL;
    updateCanDoFight();
}

void Client::internalBattleCanceled(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        //normalOutput(QStringLiteral("Battle already canceled"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle canceled");
    #endif
    bool needUpdateCanDoFight=false;
    if(battleIsValidated)
        needUpdateCanDoFight=true;
    otherPlayerBattle=NULL;
    if(send)
    {
        //send the network message
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x51;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1);
        receiveSystemText("Battle declined");
    }
    battleIsValidated=false;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(needUpdateCanDoFight)
        updateCanDoFight();
}

void Client::internalBattleAccepted(const bool &send)
{
    if(otherPlayerBattle==NULL)
    {
        normalOutput("Can't accept battle if not in battle");
        return;
    }
    if(battleIsValidated)
    {
        normalOutput("Battle already validated");
        return;
    }
    if(!otherPlayerBattle->getAbleToFight())
    {
        errorOutput("The other player can't fight");
        return;
    }
    if(!getAbleToFight())
    {
        errorOutput("You can't fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Battle accepted");
    #endif
    startTheFight();
    battleIsValidated=true;
    mHaveCurrentSkill=false;
    mMonsterChange=false;
    if(send)
    {
        std::vector<PlayerMonster> playerMonstersPreview=otherPlayerBattle->public_and_private_informations.playerMonster;

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x52;
        posOutput+=1+4;

        {
            const std::string &text=otherPlayerBattle->public_and_private_informations.public_informations.pseudo;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=otherPlayerBattle->public_and_private_informations.public_informations.skinId;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(playerMonstersPreview.size());
        posOutput+=1;
        unsigned int index=0;
        while(index<playerMonstersPreview.size() && index<255)
        {
            if(!monsterIsKO(playerMonstersPreview.at(index)))
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            else
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            posOutput+=1;
            index++;
        }
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        posOutput+=1;
        posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,FacilityLib::playerMonsterToPublicPlayerMonster(*otherPlayerBattle->getCurrentMonster()));

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

bool Client::haveBattleAction() const
{
    return mHaveCurrentSkill || mMonsterChange;
}

void Client::resetBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

void Client::haveUsedTheBattleAction()
{
    mHaveCurrentSkill=false;
    mMonsterChange=false;
}

void Client::sendBattleReturn()
{
    unsigned int index,master_index;

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x50;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(attackReturn.size());
    posOutput+=1;
    master_index=0;
    while(master_index<attackReturn.size())
    {
        const Skill::AttackReturn &attackReturnTemp=attackReturn.at(master_index);
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.doByTheCurrentMonster;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.attackReturnCase;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.success;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(attackReturnTemp.attack);
        posOutput+=2;
        //ad buff
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(attackReturnTemp.addBuffEffectMonster.size());
        posOutput+=1;
        while(index<attackReturnTemp.addBuffEffectMonster.size())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).buff;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).on;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.addBuffEffectMonster.at(index).level;
            posOutput+=1;
            index++;
        }
        //remove buff
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(attackReturnTemp.removeBuffEffectMonster.size());
        posOutput+=1;
        while(index<attackReturnTemp.removeBuffEffectMonster.size())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).buff;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).on;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.removeBuffEffectMonster.at(index).level;
            posOutput+=1;
            index++;
        }
        //life effect
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(attackReturnTemp.lifeEffectMonster.size());
        posOutput+=1;
        while(index<attackReturnTemp.lifeEffectMonster.size())
        {
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(attackReturnTemp.lifeEffectMonster.at(index).quantity);
            posOutput+=4;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.lifeEffectMonster.at(index).on;
            posOutput+=1;
            index++;
        }
        //buff effect
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(attackReturnTemp.buffLifeEffectMonster.size());
        posOutput+=1;
        while(index<attackReturnTemp.buffLifeEffectMonster.size())
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(attackReturnTemp.buffLifeEffectMonster.at(index).quantity);
            posOutput+=4;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=attackReturnTemp.buffLifeEffectMonster.at(index).on;
            posOutput+=1;
            index++;
        }
        master_index++;
    }
    if(otherPlayerBattle!=NULL && otherPlayerBattle->haveMonsterChange())
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
        posOutput+=1;
        posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,*getOtherMonster());
    }
    attackReturn.clear();

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::sendBattleMonsterChange()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x50;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=1;//attack return list size
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0;//doByTheCurrentMonster, it's the other monster
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=Skill::AttackReturnCase::AttackReturnCase_MonsterChange;//AttackReturnCase_MonsterChange=0x02
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=selectedMonsterNumberToMonsterPlace(getOtherSelectedMonsterNumber());
    posOutput+=1;
    posOutput+=FacilityLib::publicPlayerMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,*getOtherMonster());

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::emitBattleWin()
{
    fightOrBattleFinish(true,0);
}
