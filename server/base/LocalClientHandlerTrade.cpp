#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

bool Client::getInTrade()
{
    return (otherPlayerTrade!=NULL);
}

void Client::registerTradeRequest(Client * otherPlayerTrade)
{
    if(getInTrade())
    {
        normalOutput(QLatin1String("Already in trade, internal error"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("%1 have requested trade with you").arg(otherPlayerTrade->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerTrade=otherPlayerTrade;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << otherPlayerTrade->public_and_private_informations.public_informations.skinId;
    sendTradeRequest(otherPlayerTrade->rawPseudo+outputData);
}

bool Client::getIsFreezed()
{
    return tradeIsFreezed;
}

quint64 Client::getTradeCash()
{
    return tradeCash;
}

QHash<quint32,quint32> Client::getTradeObjects()
{
    return tradeObjects;
}

QList<PlayerMonster> Client::getTradeMonster()
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
        errorOutput(QLatin1String("Trade not valid"));
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput(QLatin1String("Trade is freezed, unable to re-free"));
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
        QHashIterator<quint32,quint32> i(tradeObjects);
        while (i.hasNext()) {
            i.next();
            otherPlayerTrade->addObject(i.key(),i.value());
            saveObjectRetention(i.key());
        }
        QHashIterator<quint32,quint32> j(otherPlayerTrade->getTradeObjects());
        while (j.hasNext()) {
            j.next();
            addObject(j.key(),j.value());
            otherPlayerTrade->saveObjectRetention(j.key());
        }

        //monster evolution
        {
            int index=0;
            while(index<tradeMonster.size())
            {
                GlobalServerData::serverPrivateVariables.tradedMonster << tradeMonster.at(index).id;
                index++;
            }
            index=0;
            while(index<otherPlayerTrade->tradeMonster.size())
            {
                GlobalServerData::serverPrivateVariables.tradedMonster << otherPlayerTrade->tradeMonster.at(index).id;
                index++;
            }
        }
        //monster
        otherPlayerTrade->addExistingMonster(tradeMonster);
        addExistingMonster(otherPlayerTrade->tradeMonster);

        otherPlayerTrade->sendFullPacket(0xD0,0x0008);
        sendFullPacket(0xD0,0x0008);
        otherPlayerTrade->resetTheTrade();
        resetTheTrade();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(QLatin1String("Trade freezed"));
        #endif
        otherPlayerTrade->sendFullPacket(0xD0,0x0007);
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

void Client::addExistingMonster(QList<PlayerMonster> tradeMonster)
{
    int index=0;
    while(index<tradeMonster.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                dbQueryWrite(QStringLiteral("UPDATE `monster` SET `character`=%2 WHERE `id`=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                dbQueryWrite(QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                dbQueryWrite(QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(character_id)
                             );
            break;
        }
        index++;
    }
    public_and_private_informations.playerMonster << tradeMonster;
}

void Client::tradeAddTradeCash(const quint64 &cash)
{
    if(!tradeIsValidated)
    {
        errorOutput(QLatin1String("Trade not valid"));
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput(QLatin1String("Trade is freezed, unable to change something"));
        return;
    }
    if(cash==0)
    {
        errorOutput(QLatin1String("Can't add 0 cash!"));
        return;
    }
    if(cash>public_and_private_informations.cash)
    {
        errorOutput(QLatin1String("Trade cash superior to the actual cash"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("Add cash to trade: %1").arg(cash));
    #endif
    tradeCash+=cash;
    public_and_private_informations.cash-=cash;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << cash;
    otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
}

void Client::tradeAddTradeObject(const quint32 &item,const quint32 &quantity)
{
    if(!tradeIsValidated)
    {
        errorOutput(QLatin1String("Trade not valid"));
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput(QLatin1String("Trade is freezed, unable to change something"));
        return;
    }
    if(quantity==0)
    {
        errorOutput(QLatin1String("Can add 0 of quantity"));
        return;
    }
    if(quantity>objectQuantity(item))
    {
        errorOutput(QStringLiteral("Trade object %1 in quantity %2 superior to the actual quantity").arg(item).arg(quantity));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("Add object to trade: %1 (quantity: %2)").arg(item).arg(quantity));
    #endif
    if(tradeObjects.contains(item))
        tradeObjects[item]+=quantity;
    else
        tradeObjects[item]=quantity;
    public_and_private_informations.items[item]-=quantity;
    if(public_and_private_informations.items.value(item)==0)
        public_and_private_informations.items.remove(item);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
}

void Client::tradeAddTradeMonster(const quint32 &monsterId)
{
    if(!tradeIsValidated)
    {
        errorOutput(QLatin1String("Trade not valid"));
        return;
    }
    if(tradeIsFreezed)
    {
        errorOutput(QLatin1String("Trade is freezed, unable to change something"));
        return;
    }
    if(public_and_private_informations.playerMonster.size()<=1)
    {
        errorOutput(QLatin1String("Unable to trade your last monster"));
        return;
    }
    if(isInFight())
    {
        errorOutput(QLatin1String("You can't trade monster because you are in fight"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("Add monster to trade: %1").arg(monsterId));
    #endif
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        if(public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            if(!remainMonstersToFight(monsterId))
            {
                errorOutput(QLatin1String("You can't trade this msonter because you will be without monster to fight"));
                return;
            }
            tradeMonster << public_and_private_informations.playerMonster.at(index);
            public_and_private_informations.playerMonster.removeAt(index);
            updateCanDoFight();
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x03;
            const PlayerMonster &monster=tradeMonster.last();
            out << (quint32)monster.id;
            out << (quint32)monster.monster;
            out << (quint8)monster.level;
            out << (quint32)monster.remaining_xp;
            out << (quint32)monster.hp;
            out << (quint32)monster.sp;
            out << (quint32)monster.catched_with;
            out << (quint8)monster.gender;
            out << (quint32)monster.egg_step;
            int sub_index=0;
            int sub_size=monster.buffs.size();
            out << (quint32)sub_size;
            while(sub_index<sub_size)
            {
                out << (quint32)monster.buffs.at(sub_index).buff;
                out << (quint8)monster.buffs.at(sub_index).level;
                sub_index++;
            }
            sub_index=0;
            sub_size=monster.skills.size();
            out << (quint32)sub_size;
            while(sub_index<sub_size)
            {
                out << (quint32)monster.skills.at(sub_index).skill;
                out << (quint8)monster.skills.at(sub_index).level;
                sub_index++;
            }
            otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        dbQueryWrite(QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_PostgreSQL:
                        dbQueryWrite(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                }
                index++;
            }
            return;
        }
        index++;
    }
    errorOutput(QLatin1String("Trade monster not found"));
}

void Client::internalTradeCanceled(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        //normalOutput("Trade already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QLatin1String("Trade canceled"));
    #endif
    if(tradeIsValidated)
    {
        public_and_private_informations.cash+=tradeCash;
        tradeCash=0;
        QHashIterator<quint32,quint32> i(tradeObjects);
        while (i.hasNext()) {
            i.next();
            if(public_and_private_informations.items.contains(i.key()))
                public_and_private_informations.items[i.key()]+=i.value();
            else
                public_and_private_informations.items[i.key()]=i.value();
        }
        tradeObjects.clear();
        public_and_private_informations.playerMonster << tradeMonster;
        tradeMonster.clear();
        updateCanDoFight();
    }
    otherPlayerTrade=NULL;
    if(send)
    {
        if(tradeIsValidated)
            sendFullPacket(0xD0,0x0006);
        else
            receiveSystemText(QLatin1String("Trade declined"));
    }
    tradeIsValidated=false;
}

void Client::internalTradeAccepted(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        normalOutput(QLatin1String("Can't accept trade if not in trade"));
        return;
    }
    if(tradeIsValidated)
    {
        normalOutput(QLatin1String("Trade already validated"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QLatin1String("Trade accepted"));
    #endif
    tradeIsValidated=true;
    tradeIsFreezed=false;
    tradeCash=0;
    if(send)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerTrade->public_and_private_informations.public_informations.skinId;
        sendFullPacket(0xD0,0x0005,otherPlayerTrade->rawPseudo+outputData);
    }
}
