#include "../Client.hpp"
#include "../ClientList.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../GlobalServerData.hpp"
#include <cstring>

using namespace CatchChallenger;

bool Client::getInTrade() const
{
    return (otherPlayerTrade!=PLAYER_INDEX_FOR_CONNECTED_MAX);
}

void Client::registerTradeRequest(Client &otherPlayerTrade)
{
    if(getInTrade())
    {
        normalOutput("Already in trade, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(otherPlayerTrade->public_and_private_informations.public_informations.pseudo+" have requested trade with you");
    #endif
    this->otherPlayerTrade=otherPlayerTrade.getIndexConnect();

    uint32_t pos=0;

    ProtocolParsingBase::tempBigBufferForOutput[pos]=0xE0;
    pos=1+1+4;

    //sender pseudo
    const std::string &pseudo=otherPlayerTrade.public_and_private_informations.public_informations.pseudo;
    ProtocolParsingBase::tempBigBufferForOutput[pos]=static_cast<uint8_t>(pseudo.size());
    pos+=1;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,pseudo.data(),pseudo.size());
    pos+=pseudo.size();
    //skin
    ProtocolParsingBase::tempBigBufferForOutput[pos]=otherPlayerTrade.public_and_private_informations.public_informations.skinId;
    pos+=1;

    //set the dynamic size
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(pos-1-1-4);

    sendTradeRequest(ProtocolParsingBase::tempBigBufferForOutput,pos);
}

bool Client::getIsFreezed() const
{
    return tradeIsFreezed;
}

uint64_t Client::getTradeCash() const
{
    return tradeCash;
}

std::unordered_map<uint16_t,uint32_t> Client::getTradeObjects() const
{
    return tradeObjects;
}

std::vector<PlayerMonster> Client::getTradeMonster() const
{
    return tradeMonster;
}

void Client::tradeCanceled()
{
    if(otherPlayerTrade!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        ClientList::list->rw(otherPlayerTrade).internalTradeCanceled(true);
    internalTradeCanceled(false);
}

void Client::tradeAccepted()
{
    if(otherPlayerTrade!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        ClientList::list->rw(otherPlayerTrade).internalTradeAccepted(true);
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
    Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
    if(getIsFreezed() && otherPlayerTrade.getIsFreezed())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade finished");
        #endif
        //cash
        otherPlayerTrade.addCash(tradeCash,(otherPlayerTrade.getTradeCash()!=0));
        addCash(otherPlayerTrade.getTradeCash(),(tradeCash!=0));

        //object
        auto i=tradeObjects.begin();
        while(i!=tradeObjects.cend())
        {
            otherPlayerTrade.addObject(i->first,i->second);
            saveObjectRetention(i->first);
            ++i;
        }
        const std::unordered_map<uint16_t,uint32_t> otherPlayerTradeGetTradeObjects=otherPlayerTrade.getTradeObjects();
        auto j=otherPlayerTradeGetTradeObjects.begin();
        while (j!=otherPlayerTradeGetTradeObjects.cend())
        {
            addObject(j->first,j->second);
            otherPlayerTrade.saveObjectRetention(j->first);
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
            while(index<otherPlayerTrade.tradeMonster.size())
            {
                GlobalServerData::serverPrivateVariables.tradedMonster.insert(otherPlayerTrade.tradeMonster.at(index).id);
                index++;
            }
        }
        //monster
        otherPlayerTrade.transferExistingMonster(tradeMonster);
        transferExistingMonster(otherPlayerTrade.tradeMonster);


        //send the network message
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x5B;

        otherPlayerTrade.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,0x01);
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,0x01);
        otherPlayerTrade.resetTheTrade();
        resetTheTrade();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade freezed");
        #endif

        //send the network message
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x5A;//fixed size

        otherPlayerTrade.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1);
    }
}

void Client::resetTheTrade()
{
    //reset out of trade
    tradeIsValidated=false;
    otherPlayerTrade=PLAYER_INDEX_FOR_CONNECTED_MAX;
    tradeCash=0;
    tradeObjects.clear();
    tradeMonster.clear();
    updateCanDoFight();
}

void Client::transferExistingMonster(std::vector<PlayerMonster> tradeMonster)
{
    const std::vector<uint8_t> &positionsList=addPlayerMonster(tradeMonster);
    unsigned int index=0;
    while(index<tradeMonster.size())
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_owner.asyncWrite({
                    std::to_string(character_id),
                    std::to_string(positionsList.at(index)),
                    std::to_string(tradeMonster.at(index).id)
                    });
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        index++;
    }
}

void Client::tradeAddTradeCash(const uint64_t &cash)
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

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x57;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+8);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    const uint64_t converted_cash=htole64(cash);
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&converted_cash,8);
    posOutput+=8;

    Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
    otherPlayerTrade.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x57;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+2+4);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(item);
    posOutput+=2;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(quantity);
    posOutput+=4;

    Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
    otherPlayerTrade.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::tradeAddTradeMonster(const uint8_t &monsterPosition)
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
    if(public_and_private_informations.monsters.size()<=1)
    {
        errorOutput("Unable to trade your last monster");
        return;
    }
    if(isInFight())
    {
        errorOutput("You can't trade monster because you are in fight");
        return;
    }
    if(monsterPosition>=public_and_private_informations.monsters.size())
    {
        errorOutput("Trade monster not found");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Add monster to trade: "+std::to_string(monsterId));
    #endif
    unsigned int index=monsterPosition;
    if(!remainMonstersToFightWithoutThisMonster(monsterPosition))
    {
        errorOutput("You can't trade this msonter because you will be without monster to fight");
        return;
    }
    tradeMonster.push_back(public_and_private_informations.monsters.at(index));
    public_and_private_informations.monsters.erase(public_and_private_informations.monsters.begin()+index);
    updateCanDoFight();

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x57;
    posOutput+=1+4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
    posOutput+=1;
    const PlayerMonster &monster=tradeMonster.back();

    posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,monster,character_id_db);

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
    otherPlayerTrade.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    /*while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
        std::string queryText=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_position;
        stringreplaceOne(queryText,"%1",std::to_string(index+1));
        stringreplaceOne(queryText,"%2",std::to_string(playerMonster.id));
        dbQueryWriteCommon(queryText);
        index++;
    }*/
    return;
}

void Client::internalTradeCanceled(const bool &send)
{
    if(otherPlayerTrade==PLAYER_INDEX_FOR_CONNECTED_MAX)
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
        public_and_private_informations.monsters.insert(public_and_private_informations.monsters.cend(),tradeMonster.cbegin(),tradeMonster.cend());
        tradeMonster.clear();
        updateCanDoFight();
    }
    otherPlayerTrade=PLAYER_INDEX_FOR_CONNECTED_MAX;
    if(send)
    {
        if(tradeIsValidated)
        {
            ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x59;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,0x01);
        }
        else
            receiveSystemText("Trade declined");
    }
    tradeIsValidated=false;
}

void Client::internalTradeAccepted(const bool &send)
{
    if(otherPlayerTrade==PLAYER_INDEX_FOR_CONNECTED_MAX)
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
        //send the network message
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x58;
        unsigned int pos=1+4;

        //sender pseudo
        Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
        const std::string &pseudo=otherPlayerTrade.public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[pos]=static_cast<uint8_t>(pseudo.size());
        pos+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,pseudo.data(),pseudo.size());
        pos+=pseudo.size();
        //skin
        ProtocolParsingBase::tempBigBufferForOutput[pos]=otherPlayerTrade.public_and_private_informations.public_informations.skinId;
        pos+=1;

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(pos-1-4);//set the dynamic size

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,pos);
    }
}
