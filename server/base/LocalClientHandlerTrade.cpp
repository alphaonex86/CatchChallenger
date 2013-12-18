#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

bool LocalClientHandler::getInTrade()
{
    return (otherPlayerTrade!=NULL);
}

void LocalClientHandler::registerTradeRequest(LocalClientHandler * otherPlayerTrade)
{
    if(getInTrade())
    {
        emit message("Already in trade, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("%1 have requested trade with you").arg(otherPlayerTrade->player_informations->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerTrade=otherPlayerTrade;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << otherPlayerTrade->player_informations->public_and_private_informations.public_informations.skinId;
    emit sendTradeRequest(otherPlayerTrade->player_informations->rawPseudo+outputData);
}

bool LocalClientHandler::getIsFreezed()
{
    return tradeIsFreezed;
}

quint64 LocalClientHandler::getTradeCash()
{
    return tradeCash;
}

QHash<quint32,quint32> LocalClientHandler::getTradeObjects()
{
    return tradeObjects;
}

QList<PlayerMonster> LocalClientHandler::getTradeMonster()
{
    return tradeMonster;
}

void LocalClientHandler::tradeCanceled()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeCanceled(true);
    internalTradeCanceled(false);
}

void LocalClientHandler::tradeAccepted()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeAccepted(true);
    internalTradeAccepted(false);
}

void LocalClientHandler::tradeFinished()
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to re-free");
        return;
    }
    tradeIsFreezed=true;
    if(getIsFreezed() && otherPlayerTrade->getIsFreezed())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade finished");
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

        emit otherPlayerTrade->sendFullPacket(0xD0,0x0008);
        emit sendFullPacket(0xD0,0x0008);
        otherPlayerTrade->resetTheTrade();
        resetTheTrade();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade freezed");
        #endif
        emit otherPlayerTrade->sendFullPacket(0xD0,0x0007);
    }
}

void LocalClientHandler::resetTheTrade()
{
    //reset out of trade
    tradeIsValidated=false;
    otherPlayerTrade=NULL;
    tradeCash=0;
    tradeObjects.clear();
    tradeMonster.clear();
    localClientHandlerFight.updateCanDoFight();
}

void LocalClientHandler::addExistingMonster(QList<PlayerMonster> tradeMonster)
{
    int index=0;
    while(index<tradeMonster.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QStringLiteral("UPDATE `monster` SET `character`=%2 WHERE `id`=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(player_informations->character_id)
                             );
            break;
        }
        index++;
    }
    player_informations->public_and_private_informations.playerMonster << tradeMonster;
}

void LocalClientHandler::tradeAddTradeCash(const quint64 &cash)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(cash==0)
    {
        emit error("Can't add 0 cash!");
        return;
    }
    if(cash>player_informations->public_and_private_informations.cash)
    {
        emit error("Trade cash superior to the actual cash");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("Add cash to trade: %1").arg(cash));
    #endif
    tradeCash+=cash;
    player_informations->public_and_private_informations.cash-=cash;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << cash;
    emit otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
}

void LocalClientHandler::tradeAddTradeObject(const quint32 &item,const quint32 &quantity)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(quantity==0)
    {
        emit error("Can add 0 of quantity");
        return;
    }
    if(quantity>objectQuantity(item))
    {
        emit error(QStringLiteral("Trade object %1 in quantity %2 superior to the actual quantity").arg(item).arg(quantity));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("Add object to trade: %1 (quantity: %2)").arg(item).arg(quantity));
    #endif
    if(tradeObjects.contains(item))
        tradeObjects[item]+=quantity;
    else
        tradeObjects[item]=quantity;
    player_informations->public_and_private_informations.items[item]-=quantity;
    if(player_informations->public_and_private_informations.items[item]==0)
        player_informations->public_and_private_informations.items.remove(item);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    emit otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
}

void LocalClientHandler::tradeAddTradeMonster(const quint32 &monsterId)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(player_informations->public_and_private_informations.playerMonster.size()<=1)
    {
        emit error("Unable to trade your last monster");
        return;
    }
    if(localClientHandlerFight.isInFight())
    {
        emit error("You can't trade monster because you are in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("Add monster to trade: %1").arg(monsterId));
    #endif
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        if(player_informations->public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            if(!localClientHandlerFight.remainMonstersToFight(monsterId))
            {
                emit error("You can't trade this msonter because you will be without monster to fight");
                return;
            }
            tradeMonster << player_informations->public_and_private_informations.playerMonster.at(index);
            player_informations->public_and_private_informations.playerMonster.removeAt(index);
            localClientHandlerFight.updateCanDoFight();
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
            emit otherPlayerTrade->sendFullPacket(0xD0,0x0004,outputData);
            while(index<player_informations->public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=player_informations->public_and_private_informations.playerMonster.at(index);
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
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
    emit error("Trade monster not found");
}

void LocalClientHandler::internalTradeCanceled(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        //emit message("Trade already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Trade canceled");
    #endif
    if(tradeIsValidated)
    {
        player_informations->public_and_private_informations.cash+=tradeCash;
        tradeCash=0;
        QHashIterator<quint32,quint32> i(tradeObjects);
        while (i.hasNext()) {
            i.next();
            if(player_informations->public_and_private_informations.items.contains(i.key()))
                player_informations->public_and_private_informations.items[i.key()]+=i.value();
            else
                player_informations->public_and_private_informations.items[i.key()]=i.value();
        }
        tradeObjects.clear();
        player_informations->public_and_private_informations.playerMonster << tradeMonster;
        tradeMonster.clear();
        localClientHandlerFight.updateCanDoFight();
    }
    otherPlayerTrade=NULL;
    if(send)
    {
        if(tradeIsValidated)
            emit sendFullPacket(0xD0,0x0006);
        else
            emit receiveSystemText(QStringLiteral("Trade declined"));
    }
    tradeIsValidated=false;
}

void LocalClientHandler::internalTradeAccepted(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        emit message("Can't accept trade if not in trade");
        return;
    }
    if(tradeIsValidated)
    {
        emit message("Trade already validated");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Trade accepted");
    #endif
    tradeIsValidated=true;
    tradeIsFreezed=false;
    tradeCash=0;
    if(send)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerTrade->player_informations->public_and_private_informations.public_informations.skinId;
        emit sendFullPacket(0xD0,0x0005,otherPlayerTrade->player_informations->rawPseudo+outputData);
    }
}
