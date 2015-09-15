#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../base/PreparedDBQuery.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

bool Client::getInTrade() const
{
    return (otherPlayerTrade!=NULL);
}

void Client::registerTradeRequest(Client * otherPlayerTrade)
{
    if(getInTrade())
    {
        normalOutput("Already in trade, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(otherPlayerTrade->public_and_private_informations.public_informations.pseudo+" have requested trade with you");
    #endif
    this->otherPlayerTrade=otherPlayerTrade;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << otherPlayerTrade->public_and_private_informations.public_informations.skinId;

    uint32_t pos=0;

    ProtocolParsingBase::tempBigBufferForOutput[pos]=0xE0;
    pos=1+4+1;

    //sender pseudo
    const std::string &pseudo=otherPlayerTrade->public_and_private_informations.public_informations.pseudo;
    ProtocolParsingBase::tempBigBufferForOutput[pos]=pseudo.size();
    pos+=1;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,pseudo.data(),pseudo.size());
    pos+=pseudo.size();
    //skin
    ProtocolParsingBase::tempBigBufferForOutput[pos]=otherPlayerTrade->public_and_private_informations.public_informations.skinId;
    pos+=1;

    //set the dynamic size
    *reinterpret_cast<quint32 *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(pos-1-4);

    sendTradeRequest(ProtocolParsingBase::tempBigBufferForOutput,pos);
}

bool Client::getIsFreezed() const
{
    return tradeIsFreezed;
}

quint64 Client::getTradeCash() const
{
    return tradeCash;
}

std::unordered_map<uint32_t,uint32_t> Client::getTradeObjects() const
{
    return tradeObjects;
}

std::vector<PlayerMonster> Client::getTradeMonster() const
{
    return tradeMonster;
}

void Client::tradeCanceled()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeCanceled(true);
    internalTradeCanceled(false);
}

void Client::tradeAccepted()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeAccepted(true);
    internalTradeAccepted(false);
}

void Client::tradeFinished()
{
    if(!tradeIsValidated)
    {
        errorOutput("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput("Trade is freezed, unable to re-free");
        return;
    }
    tradeIsFreezed=true;
    if(getIsFreezed() && otherPlayerTrade->getIsFreezed())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade finished");
        #endif
        //cash
        otherPlayerTrade->addCash(tradeCash,(otherPlayerTrade->getTradeCash()!=0));
        addCash(otherPlayerTrade->getTradeCash(),(tradeCash!=0));

        //object
        auto i=tradeObjects.begin();
        while(i!=tradeObjects.cend())
        {
            otherPlayerTrade->addObject(i->first,i->second);
            saveObjectRetention(i->first);
            ++i;
        }
        const std::unordered_map<uint32_t,uint32_t> otherPlayerTradeGetTradeObjects=otherPlayerTrade->getTradeObjects();
        auto j=otherPlayerTradeGetTradeObjects.begin();
        while (j!=otherPlayerTradeGetTradeObjects.cend())
        {
            addObject(j->first,j->second);
            otherPlayerTrade->saveObjectRetention(j->first);
            ++j;
        }

        //monster evolution
        {
            unsigned int index=0;
            while(index<tradeMonster.size())
            {
                GlobalServerData::serverPrivateVariables.tradedMonster.insert(tradeMonster.at(index).id);
                index++;
            }
            index=0;
            while(index<otherPlayerTrade->tradeMonster.size())
            {
                GlobalServerData::serverPrivateVariables.tradedMonster.insert(otherPlayerTrade->tradeMonster.at(index).id);
                index++;
            }
        }
        //monster
        otherPlayerTrade->addExistingMonster(tradeMonster);
        addExistingMonster(otherPlayerTrade->tradeMonster);

        otherPlayerTrade->sendMessage(0x5B);
        sendMessage(0x5B);
        otherPlayerTrade->resetTheTrade();
        resetTheTrade();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade freezed");
        #endif
        otherPlayerTrade->sendMessage(0x5A);
    }
}

void Client::resetTheTrade()
{
    //reset out of trade
    tradeIsValidated=false;
    otherPlayerTrade=NULL;
    tradeCash=0;
    tradeObjects.clear();
    tradeMonster.clear();
    updateCanDoFight();
}

void Client::addExistingMonster(std::vector<PlayerMonster> tradeMonster)
{
    unsigned int index=0;
    while(index<tradeMonster.size())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_update_monster_owner;
        stringreplaceOne(queryText,"%1",std::to_string(tradeMonster.at(index).id));
        stringreplaceOne(queryText,"%2",std::to_string(character_id));
        dbQueryWriteCommon(queryText);
        index++;
    }
    public_and_private_informations.playerMonster.insert(public_and_private_informations.playerMonster.cend(),tradeMonster.cbegin(),tradeMonster.cend());
}

void Client::tradeAddTradeCash(const quint64 &cash)
{
    if(!tradeIsValidated)
    {
        errorOutput("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput("Trade is freezed, unable to change something");
        return;
    }
    if(cash==0)
    {
        errorOutput("Can't add 0 cash!");
        return;
    }
    if(cash>public_and_private_informations.cash)
    {
        errorOutput("Trade cash superior to the actual cash");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Add cash to trade: "+std::to_string(cash));
    #endif
    tradeCash+=cash;
    public_and_private_informations.cash-=cash;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << cash;
    otherPlayerTrade->sendMessage(0x57,outputData.constData(),outputData.size());
}

void Client::tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity)
{
    if(!tradeIsValidated)
    {
        errorOutput("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput("Trade is freezed, unable to change something");
        return;
    }
    if(quantity==0)
    {
        errorOutput("Can add 0 of quantity");
        return;
    }
    if(quantity>objectQuantity(item))
    {
        errorOutput("Trade object "+std::to_string(item)+" in quantity "+std::to_string(quantity)+" superior to the actual quantity");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Add object to trade: "+std::to_string(item)+" (quantity: "+std::to_string(quantity)+")");
    #endif
    if(tradeObjects.find(item)!=tradeObjects.cend())
        tradeObjects[item]+=quantity;
    else
        tradeObjects[item]=quantity;
    public_and_private_informations.items[item]-=quantity;
    if(public_and_private_informations.items.at(item)==0)
        public_and_private_informations.items.erase(item);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << item;
    out << quantity;
    otherPlayerTrade->sendMessage(0x57,outputData.constData(),outputData.size());
}

void Client::tradeAddTradeMonster(const uint32_t &monsterId)
{
    if(!tradeIsValidated)
    {
        errorOutput("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput("Trade is freezed, unable to change something");
        return;
    }
    if(public_and_private_informations.playerMonster.size()<=1)
    {
        errorOutput("Unable to trade your last monster");
        return;
    }
    if(isInFight())
    {
        errorOutput("You can't trade monster because you are in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Add monster to trade: "+std::to_string(monsterId));
    #endif
    unsigned int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            if(!remainMonstersToFight(monsterId))
            {
                errorOutput("You can't trade this msonter because you will be without monster to fight");
                return;
            }
            tradeMonster.push_back(public_and_private_informations.playerMonster.at(index));
            public_and_private_informations.playerMonster.erase(public_and_private_informations.playerMonster.begin()+index);
            updateCanDoFight();
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (uint8_t)0x03;
            const PlayerMonster &monster=tradeMonster.back();
            out << monster.id;
            out << monster.monster;
            out << monster.level;
            out << monster.remaining_xp;
            out << monster.hp;
            if(CommonSettingsServer::commonSettingsServer.useSP)
                out << monster.sp;
            out << monster.catched_with;
            out << (uint8_t)monster.gender;
            out << monster.egg_step;
            out << monster.character_origin;
            int sub_index=0;
            int sub_size=monster.buffs.size();
            out << (uint32_t)sub_size;
            while(sub_index<sub_size)
            {
                out << monster.buffs.at(sub_index).buff;
                out << monster.buffs.at(sub_index).level;
                sub_index++;
            }
            sub_index=0;
            sub_size=monster.skills.size();
            out << (uint32_t)sub_size;
            while(sub_index<sub_size)
            {
                out << monster.skills.at(sub_index).skill;
                out << monster.skills.at(sub_index).level;
                sub_index++;
            }
            otherPlayerTrade->sendMessage(0x57,outputData.constData(),outputData.size());
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                std::string queryText=PreparedDBQueryCommon::db_query_update_monster_position;
                stringreplaceOne(queryText,"%1",std::to_string(index+1));
                stringreplaceOne(queryText,"%2",std::to_string(playerMonster.id));
                dbQueryWriteCommon(queryText);
                index++;
            }
            return;
        }
        index++;
    }
    errorOutput("Trade monster not found");
}

void Client::internalTradeCanceled(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        //normalOutput("Trade already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Trade canceled");
    #endif
    if(tradeIsValidated)
    {
        public_and_private_informations.cash+=tradeCash;
        tradeCash=0;
        auto i=tradeObjects.begin();
        while(i!=tradeObjects.cend())
        {
            if(public_and_private_informations.items.find(i->first)!=public_and_private_informations.items.cend())
                public_and_private_informations.items[i->first]+=i->second;
            else
                public_and_private_informations.items[i->first]=i->second;
            ++i;
        }
        tradeObjects.clear();
        public_and_private_informations.playerMonster.insert(public_and_private_informations.playerMonster.cend(),tradeMonster.cbegin(),tradeMonster.cend());
        tradeMonster.clear();
        updateCanDoFight();
    }
    otherPlayerTrade=NULL;
    if(send)
    {
        if(tradeIsValidated)
            sendMessage(0x59);
        else
            receiveSystemText("Trade declined");
    }
    tradeIsValidated=false;
}

void Client::internalTradeAccepted(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        normalOutput("Can't accept trade if not in trade");
        return;
    }
    if(tradeIsValidated)
    {
        normalOutput("Trade already validated");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Trade accepted");
    #endif
    tradeIsValidated=true;
    tradeIsFreezed=false;
    tradeCash=0;
    if(send)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << otherPlayerTrade->public_and_private_informations.public_informations.skinId;
        const QByteArray newData(otherPlayerTrade->rawPseudo+outputData);
        sendMessage(0x58,newData.constData(),newData.size());
    }
}
