#include "ClientNetworkRead.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

QRegularExpression ClientNetworkRead::commandRegExp=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)?$"));
QRegularExpression ClientNetworkRead::commandRegExpWithArgs=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)$"));
QRegularExpression ClientNetworkRead::isolateTheMainCommand=QRegularExpression(QLatin1String("^ (.*)$"));

ClientNetworkRead::ClientNetworkRead(Player_internal_informations *player_informations,ConnectedSocket * socket) :
    ProtocolParsingInput(socket,PacketModeTransmission_Server)
{
    have_send_protocol=false;
    is_logging_in_progess=false;
    stopIt=false;
    this->player_informations=player_informations;
    this->socket=socket;

    queryNumberList.reserve(256);
    int index=0;
    while(index<256)
    {
        queryNumberList << index;
        index++;
    }
}

void ClientNetworkRead::stopRead()
{
    stopIt=true;
//	if(socket)
//		disconnect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));
}

void ClientNetworkRead::askIfIsReadyToStop()
{
    stopIt=true;
    emit isReadyToStop();
}

void ClientNetworkRead::teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    if(queryNumberList.empty())
    {
        emit error(QStringLiteral("Sorry, no free query number to send this query of teleportation"));
        return;
    }
    TeleportationPoint teleportationPoint;
    teleportationPoint.map=map;
    teleportationPoint.x=x;
    teleportationPoint.y=y;
    teleportationPoint.orientation=orientation;
    lastTeleportation << teleportationPoint;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    out << (COORD_TYPE)x;
    out << (COORD_TYPE)y;
    out << (quint8)orientation;
    emit sendQuery(0x79,0x0001,queryNumberList.first(),outputData);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::sendTradeRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        emit error(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    emit sendQuery(0x80,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::sendBattleRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        emit error(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    emit sendQuery(0x90,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QStringLiteral("parseInputBeforeLogin(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            case 0x0001:
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    return;
                {
                    QString protocol;
                    in >> protocol;
                    if(GlobalServerData::serverPrivateVariables.connected_players>=GlobalServerData::serverSettings.max_players)
                    {
                        out << (quint8)0x03;		//server full
                        out << QStringLiteral("Server full");
                        emit postReply(queryNumber,outputData);
                        emit error(QStringLiteral("Server full (%1/%2)").arg(GlobalServerData::serverPrivateVariables.connected_players).arg(GlobalServerData::serverSettings.max_players));
                        return;
                    }
                    if(protocol==PROTOCOL_HEADER)
                    {
                        out << (quint8)0x01;		//protocol supported
                        switch(ProtocolParsing::compressionType)
                        {
                            case CompressionType_None:
                                out << (quint8)0x00;
                            break;
                            case CompressionType_Zlib:
                                out << (quint8)0x01;
                            break;
                            case CompressionType_Xz:
                                out << (quint8)0x02;
                            break;
                            default:
                                emit error("Compression selected wrong");
                            return;
                        }
                        emit postReply(queryNumber,outputData);
                        have_send_protocol=true;
                        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                        emit message(QStringLiteral("Protocol sended and replied"));
                        #endif
                    }
                    else
                    {
                        out << (quint8)0x02;		//protocol not supported
                        emit postReply(queryNumber,outputData);
                        emit error("Wrong protocol");
                        return;
                    }
                }
            break;
            case 0x0002:
                if(!have_send_protocol)
                {
                    emit error("send login before the protocol");
                    return;
                }
                if((data.size()-in.device()->pos())!=(64*2))
                    parseError(QStringLiteral("wrong size with the main ident: %1, because %2 != 20").arg(mainCodeType).arg(data.size()-in.device()->pos()));
                else if(is_logging_in_progess)
                {
                    out << (quint8)1;
                    emit postReply(queryNumber,outputData);
                    emit error("Loggin in progress");
                }
                else if(player_informations->character_loaded)
                {
                    out << (quint8)1;
                    emit postReply(queryNumber,outputData);
                    emit error("Already logged");
                }
                else
                {
                    is_logging_in_progess=true;
                    QByteArray data_extracted;
                    data_extracted=data.right(data.size()-in.device()->pos());
                    QByteArray hash,login;
                    login=data_extracted.mid(0,64);
                    data_extracted.remove(0,64);
                    hash=data_extracted;
                    emit askLogin(queryNumber,login,hash);
                    return;
                }
            break;
            default:
                parseError("wrong data before login with mainIdent: "+QString::number(mainCodeType)+", subIdent: "+QString::number(subCodeType));
            break;
        }
        break;
        default:
            parseError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parseInputBeforeLogin(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
}

void ClientNetworkRead::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!player_informations->character_loaded)
    {
        //wrong protocol
        emit needDisconnectTheClient();
        //parseError(QStringLiteral("is not logged, parseMessage(%1)").arg(mainCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QStringLiteral("parseMessage(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x40:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
            {
                parseError("Wrong size in move packet");
                return;
            }
            in >> previousMovedUnit;
            in >> direction;
        }
        if(direction<1 || direction>8)
        {
            parseError(QStringLiteral("Bad direction number: %1").arg(direction));
            return;
        }
        emit moveThePlayer(previousMovedUnit,(Direction)direction);
        break;
        case 0x61:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
            {
                parseError("Wrong size in move packet");
                return;
            }
            quint32 skill;
            in >> skill;
            emit useSkill(skill);
        }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parseMessage(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void ClientNetworkRead::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!player_informations->character_loaded)
    {
        //wrong protocol
        emit needDisconnectTheClient();
        //parseError(QStringLiteral("is not logged, parseMessage(%1,%2)").arg(mainCodeType).arg(subCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QStringLiteral("parseMessage(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x42:
        switch(subCodeType)
        {
            //Chat
            case 0x0003:
            {
                if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                {
                    parseError("wrong remaining size for chat");
                    return;
                }
                quint8 chatType;
                in >> chatType;
                if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
                {
                    parseError("chat type error: "+QString::number(chatType));
                    return;
                }
                if(chatType==Chat_type_pm)
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString text;
                    in >> text;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString pseudo;
                    in >> pseudo;
                    emit message(QStringLiteral("/pm ")+pseudo+QLatin1String(" ")+text);
                    emit sendPM(text,pseudo);
                }
                else
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString text;
                    in >> text;

                    if(!text.startsWith(QLatin1String("/")))
                    {
                        if(chatType==Chat_type_local)
                            emit sendLocalChatText(text);
                        else
                            emit sendChatText((Chat_type)chatType,text);
                    }
                    else
                    {
                        if(text.contains(commandRegExp))
                        {
                            //isolate the main command (the first word)
                            QString command=text;
                            command.replace(commandRegExp,QLatin1String("\\1"));

                            //isolate the arguements
                            if(text.contains(commandRegExp))
                            {
                                text.replace(commandRegExp,QLatin1String("\\2"));
                                text.replace(isolateTheMainCommand,QLatin1String("\\1"));
                            }
                            else
                                text=QLatin1String("");

                            //the normal player command
                            {
                                if(command==QLatin1String("playernumber"))
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                    return;
                                }
                                else if(command==QLatin1String("playerlist"))
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                    return;
                                }
                                else if(command==QLatin1String("trade"))
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                    return;
                                }
                                else if(command==QLatin1String("battle"))
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                    return;
                                }
                            }
                            //the admin command
                            if(player_informations->public_and_private_informations.public_informations.type==Player_type_gm || player_informations->public_and_private_informations.public_informations.type==Player_type_dev)
                            {
                                if(command==QLatin1String("give"))
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("take"))
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("tp"))
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("kick"))
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("chat"))
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("setrights"))
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("stop") || command==QLatin1String("restart"))
                                {
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else if(command==QLatin1String("addbots") || command==QLatin1String("removebots"))
                                {
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    emit message(QStringLiteral("send command: /")+command+QLatin1String(" ")+text);
                                }
                                else
                                {
                                    emit message(QStringLiteral("unknown send command: /")+command+QLatin1String(" and \"")+text+QLatin1String("\""));
                                    receiveSystemText(QLatin1String("unknown send command: /")+command+QLatin1String(" and \"")+text+QLatin1String("\""));
                                }
                            }
                            else
                            {
                                emit message(QStringLiteral("unknown send command: /")+command+QLatin1String(" and \"")+text+QLatin1String("\""));
                                receiveSystemText(QLatin1String("unknown send command: /")+command+QLatin1String(" and \"")+text+QLatin1String("\""));
                            }
                        }
                        else
                            emit message(QStringLiteral("commands seem not right: \"")+text+QLatin1String("\""));
                    }
                }
                return;
            }
            break;
            //Clan invite accept
            case 0x0004:
            {
                if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                {
                    parseError("wrong remaining size for chat");
                    return;
                }
                quint8 returnCode;
                in >> returnCode;
                switch(returnCode)
                {
                    case 0x01:
                        emit clanInvite(true);
                    break;
                    case 0x02:
                        emit clanInvite(false);
                    break;
                    default:
                        parseError(QStringLiteral("wrong return code for clan invite ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                }
            }
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        //inventory
        case 0x50:
            switch(subCodeType)
            {
                //Destroy an object
                case 0x0002:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for destroy item id");
                        return;
                    }
                    quint32 itemId;
                    in >> itemId;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for destroy quantity");
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    emit destroyObject(itemId,quantity);
                }
                break;
                //Destroy an object
                case 0x0003:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint8 type;
                    in >> type;
                    switch(type)
                    {
                        //cash
                        case 0x01:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint64)))
                            {
                                parseError("wrong remaining size for trade add cash");
                                return;
                            }
                            quint64 cash;
                            in >> cash;
                            emit tradeAddTradeCash(cash);
                        }
                        break;
                        //item
                        case 0x02:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError("wrong remaining size for trade add item id");
                                return;
                            }
                            quint32 item;
                            in >> item;
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError("wrong remaining size for trade add item quantity");
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            emit tradeAddTradeObject(item,quantity);
                        }
                        break;
                        //monster
                        case 0x03:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError("wrong remaining size for trade add monster");
                                return;
                            }
                            quint32 monsterId;
                            in >> monsterId;
                            emit tradeAddTradeMonster(monsterId);
                        }
                        break;
                        default:
                            parseError("wrong type for trade add");
                            return;
                        break;
                    }
                }
                break;
                //trade finished after the accept
                case 0x0004:
                    emit tradeFinished();
                break;
                //trade canceled after the accept
                case 0x0005:
                    emit tradeCanceled();
                break;
                //deposite/withdraw to the warehouse
                case 0x0006:
                {
                    qint64 cash;
                    QList<QPair<quint32, qint32> > items;
                    QList<quint32> withdrawMonsters;
                    QList<quint32> depositeMonsters;
                    if((data.size()-in.device()->pos())<((int)sizeof(qint64)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> cash;
                    quint32 size;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size;
                    quint32 index=0;
                    while(index<size)
                    {
                        quint32 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        qint32 quantity;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> quantity;
                        items << QPair<quint32, qint32>(id,quantity);
                        index++;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size;
                    index=0;
                    while(index<size)
                    {
                        quint32 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        withdrawMonsters << id;
                        index++;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size;
                    index=0;
                    while(index<size)
                    {
                        quint32 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        depositeMonsters << id;
                        index++;
                    }
                    emit wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
                }
                break;
                default:
                    parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        //battle
        case 0x60:
            switch(subCodeType)
            {
                //Try escape
                case 0x0002:
                    emit tryEscape();
                break;
                //Learn skill
                case 0x0004:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 skill;
                    in >> skill;
                    emit learnSkill(monsterId,skill);
                }
                break;
                //Heal all the monster
                case 0x0006:
                    emit heal();
                break;
                //Request bot fight
                case 0x0007:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 fightId;
                    in >> fightId;
                    emit requestFight(fightId);
                }
                break;
                //move the monster
                case 0x0008:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint8 moveWay;
                    in >> moveWay;
                    bool moveUp;
                    switch(moveWay)
                    {
                        case 0x01:
                            moveUp=true;
                        break;
                        case 0x02:
                            moveUp=false;
                        break;
                        default:
                            parseError("wrong move up value");
                        return;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint8 position;
                    in >> position;
                    emit moveMonster(moveUp,position);
                }
                break;
                //change monster in fight
                case 0x0009:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    emit changeOfMonsterInFight(monsterId);
                }
                break;
                //Monster evolution validated
                case 0x000A:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    emit confirmEvolution(monsterId);
                }
                break;
                //Monster evolution validated
                case 0x000B:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 item;
                    in >> item;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add type");
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    emit useObjectOnMonster(item,monsterId);
                }
                break;
                default:
                    parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        //quest
        case 0x6a:
            switch(subCodeType)
            {
                //Quest start
                case 0x0001:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for quest start");
                        return;
                    }
                    quint32 questId;
                    in >> questId;
                    emit newQuestAction(QuestAction_Start,questId);
                }
                break;
                //Quest finish
                case 0x0002:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for quest finish");
                        return;
                    }
                    quint32 questId;
                    in >> questId;
                    emit newQuestAction(QuestAction_Finish,questId);
                }
                break;
                //Quest cancel
                case 0x0003:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for quest cancel");
                        return;
                    }
                    quint32 questId;
                    in >> questId;
                    emit newQuestAction(QuestAction_Cancel,questId);
                }
                break;
                //Quest next step
                case 0x0004:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for quest next step");
                        return;
                    }
                    quint32 questId;
                    in >> questId;
                    emit newQuestAction(QuestAction_NextStep,questId);
                }
                break;
                //Waiting for city caputre
                case 0x0005:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                    {
                        parseError("wrong remaining size for city capture");
                        return;
                    }
                    quint8 cancel;
                    in >> cancel;
                    if(cancel==0x00)
                        emit waitingForCityCaputre(false);
                    else
                        emit waitingForCityCaputre(true);
                }
                break;
                default:
                    parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parseMessage(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

//have query with reply
void ClientNetworkRead::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    parseError(QStringLiteral("no query with only the main code for now, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
    return;
}

void ClientNetworkRead::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    const bool goodQueryBeforeCharacterLoaded=mainCodeType==0x02 &&
            (subCodeType==0x03 ||
             subCodeType==0x04 ||
             subCodeType==0x0C ||
             subCodeType==0x05
                );
    if(player_informations->account_id==0 || (!player_informations->character_loaded && !goodQueryBeforeCharacterLoaded))
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            //Add character
            case 0x0003:
            {
                quint8 profileIndex;
                QString pseudo;
                QString skin;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> profileIndex;
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                {
                    parseError(QStringLiteral("error to get pseudo with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> pseudo;
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                {
                    parseError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> skin;
                emit addCharacter(queryNumber,profileIndex,pseudo,skin);
            }
            break;
            //Remove character
            case 0x0004:
            {
                quint32 characterId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> characterId;
                emit removeCharacter(queryNumber,characterId);
            }
            break;
            //Select character
            case 0x0005:
            {
                quint32 characterId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> characterId;
                emit selectCharacter(queryNumber,characterId);
            }
            break;
            //Send datapack file list
            case 0x000C:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 number_of_file;
                in >> number_of_file;
                QStringList files;
                QList<quint64> timestamps;
                QString tempFileName;
                quint64 tempTimestamps;
                quint32 index=0;
                while(index<number_of_file)
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("error at datapack file list query"));
                        return;
                    }
                    in >> tempFileName;
                    files << tempFileName;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                    {
                        parseError(QStringLiteral("wrong size for id with main ident: %1, subIdent: %2, remaining: %3, lower than: %4")
                            .arg(mainCodeType)
                            .arg(subCodeType)
                            .arg(in.device()->size()-in.device()->pos())
                            .arg((int)sizeof(quint32))
                            );
                        return;
                    }
                    in >> tempTimestamps;
                    timestamps << tempTimestamps;
                    index++;
                }
                emit datapackList(queryNumber,files,timestamps);
            }
            break;
            //Clan action
            case 0x000D:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 clanActionId;
                in >> clanActionId;
                switch(clanActionId)
                {
                    case 0x01:
                    case 0x04:
                    case 0x05:
                    {
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            parseError(QStringLiteral("error at datapack file list query"));
                            return;
                        }
                        QString tempString;
                        in >> tempString;
                        emit clanAction(queryNumber,clanActionId,tempString);
                    }
                    break;
                    case 0x02:
                    case 0x03:
                        emit clanAction(queryNumber,clanActionId,QString());
                    break;
                    default:
                    parseError(QStringLiteral("unknown clan action code"));
                    return;
                }
            }
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x10:
        switch(subCodeType)
        {
            //Use seed into dirt
            case 0x0006:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 plant_id;
                in >> plant_id;
                emit plantSeed(queryNumber,plant_id);
            }
            break;
            //Collect mature plant
            case 0x0007:
                emit collectPlant(queryNumber);
            break;
            //Usage of recipe
            case 0x0008:
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 recipe_id;
                in >> recipe_id;
                emit useRecipe(queryNumber,recipe_id);
            break;
            //Use object
            case 0x0009:
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 objectId;
                in >> objectId;
                emit useObject(queryNumber,objectId);
            break;
            //Get shop list
            case 0x000A:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 shopId;
                in >> shopId;
                emit getShopList(queryNumber,shopId);
            }
            break;
            //Buy object
            case 0x000B:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 shopId;
                in >> shopId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 objectId;
                in >> objectId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 quantity;
                in >> quantity;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 price;
                in >> price;
                emit buyObject(queryNumber,shopId,objectId,quantity,price);
            }
            break;
            //Sell object
            case 0x000C:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 shopId;
                in >> shopId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 objectId;
                in >> objectId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 quantity;
                in >> quantity;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 price;
                in >> price;
                emit sellObject(queryNumber,shopId,objectId,quantity,price);
            }
            break;
            case 0x000D:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 factoryId;
                in >> factoryId;
                emit getFactoryList(queryNumber,factoryId);
            }
            break;
            case 0x000E:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 factoryId;
                in >> factoryId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 objectId;
                in >> objectId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 quantity;
                in >> quantity;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 price;
                in >> price;
                emit buyFactoryObject(queryNumber,factoryId,objectId,quantity,price);
            }
            break;
            case 0x000F:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 factoryId;
                in >> factoryId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 objectId;
                in >> objectId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 quantity;
                in >> quantity;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 price;
                in >> price;
                emit sellFactoryObject(queryNumber,factoryId,objectId,quantity,price);
            }
            break;
            case 0x0010:
                emit getMarketList(queryNumber);
            break;
            case 0x0011:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 queryType;
                in >> queryType;
                switch(queryType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                        parseError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 marketObjectId;
                    in >> marketObjectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    emit buyMarketObject(queryNumber,marketObjectId,quantity);
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    emit buyMarketMonster(queryNumber,monsterId);
                }
            }
            break;
            case 0x0012:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 queryType;
                in >> queryType;
                switch(queryType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                        parseError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 objectId;
                    in >> objectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 price;
                    in >> price;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(double))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    double bitcoin;
                    in >> bitcoin;
                    emit putMarketObject(queryNumber,objectId,quantity,price,bitcoin);
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 price;
                    in >> price;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(double))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    double bitcoin;
                    in >> bitcoin;
                    emit putMarketMonster(queryNumber,monsterId,price,bitcoin);
                }
            }
            break;
            case 0x0013:
                emit recoverMarketCash(queryNumber);
            break;
            case 0x0014:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 queryType;
                in >> queryType;
                switch(queryType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                        parseError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 objectId;
                    in >> objectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    emit withdrawMarketObject(queryNumber,objectId,quantity);
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    emit withdrawMarketMonster(queryNumber,monsterId);
                }
            }
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

//send reply
void ClientNetworkRead::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    parseError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void ClientNetworkRead::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    if(stopIt)
        return;
    if(!player_informations->character_loaded)
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x79:
        switch(subCodeType)
        {
            //teleportation
            case 0x0001:
                emit message("emit teleportValidatedTo() from protocol");
                emit teleportValidatedTo(lastTeleportation.first().map,lastTeleportation.first().x,lastTeleportation.first().y,lastTeleportation.first().orientation);
                lastTeleportation.removeFirst();
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x80:
        switch(subCodeType)
        {
            //Another player request a trade
            case 0x0001:
                {
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            emit tradeAccepted();
                        break;
                        case 0x02:
                            emit tradeCanceled();
                        break;
                        default:
                            parseError(QStringLiteral("ident: %1, sub ident: %2, unknown return code: %3").arg(mainCodeType).arg(subCodeType).arg(returnCode));
                        break;
                    }
                }
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x90:
        switch(subCodeType)
        {
            //Another player request a trade
            case 0x0001:
                {
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            emit battleAccepted();
                        break;
                        case 0x02:
                            emit battleCanceled();
                        break;
                        default:
                            parseError(QStringLiteral("ident: %1, sub ident: %2, unknown return code: %3").arg(mainCodeType).arg(subCodeType).arg(returnCode));
                        break;
                    }
                }
            break;
            default:
                parseError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void ClientNetworkRead::parseError(const QString &errorString)
{
    if(GlobalServerData::serverSettings.tolerantMode)
        emit message(QStringLiteral("Packed dropped, due to: %1").arg(errorString));
    else
        emit error(errorString);
}

/// \warning it need be complete protocol trame
void ClientNetworkRead::fake_receive_data(const QByteArray &data)
{
    Q_UNUSED(data);
//	parseInputAfterLogin(data);
}

void ClientNetworkRead::purgeReadBuffer()
{
    canStartReadData=true;
    parseIncommingData();
}

void ClientNetworkRead::receiveSystemText(const QString &text)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)Chat_type_system;
    out << text;
    emit sendFullPacket(0xC2,0x0005,outputData);
}
