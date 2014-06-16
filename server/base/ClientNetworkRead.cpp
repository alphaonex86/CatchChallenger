#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::doDDOSCompute()
{
    {
        movePacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            movePacketKick[index]=movePacketKick[index+1];
            movePacketKickTotalCache+=movePacketKick[index];
            index++;
        }
        movePacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=movePacketKickNewValue;
        movePacketKickTotalCache+=movePacketKickNewValue;
        movePacketKickNewValue=0;
    }
    {
        chatPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        chatPacketKickNewValue=0;
    }
    {
        otherPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        otherPacketKickNewValue=0;
    }
}

void Client::sendNewEvent(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of sendNewEvent"));
        return;
    }
    sendQuery(0x79,0x0002,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void Client::teleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of teleportation"));
        return;
    }
    PlayerOnMap teleportationPoint;
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
    sendQuery(0x79,0x0001,queryNumberList.first(),outputData);
    queryNumberList.removeFirst();
}

void Client::sendTradeRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    sendQuery(0x80,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void Client::sendBattleRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    sendQuery(0x90,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void Client::parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parseInputBeforeLogin(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
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
                        out << Client::text_server_full;
                        postReply(queryNumber,outputData);
                        errorOutput(Client::text_server_full);
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
                                errorOutput("Compression selected wrong");
                            return;
                        }
                        postReply(queryNumber,outputData);
                        have_send_protocol=true;
                        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                        normalOutput(QStringLiteral("Protocol sended and replied"));
                        #endif
                    }
                    else
                    {
                        out << (quint8)0x02;		//protocol not supported
                        postReply(queryNumber,outputData);
                        errorOutput("Wrong protocol");
                        return;
                    }
                }
            break;
            case 0x0002:
                if(!have_send_protocol)
                {
                    errorOutput("send login before the protocol");
                    return;
                }
                if((data.size()-in.device()->pos())!=(64*2))
                    parseError(QStringLiteral("wrong size with the main ident: %1, because %2 != 20").arg(mainCodeType).arg(data.size()-in.device()->pos()));
                else if(is_logging_in_progess)
                {
                    out << (quint8)1;
                    postReply(queryNumber,outputData);
                    errorOutput("Loggin in progress");
                }
                else if(character_loaded)
                {
                    out << (quint8)1;
                    postReply(queryNumber,outputData);
                    errorOutput("Already logged");
                }
                else
                {
                    is_logging_in_progess=true;
                    QByteArray data_extracted(data.right(data.size()-in.device()->pos()));
                    const QByteArray &login=data_extracted.mid(0,64);
                    data_extracted.remove(0,64);
                    const QByteArray &hash=data_extracted;
                    askLogin(queryNumber,login,hash);
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

void Client::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!character_loaded)
    {
        //wrong protocol
        disconnectClient();
        //parseError(QStringLiteral("is not logged, parsenormalOutput(%1)").arg(mainCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parsenormalOutput(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x40:
        {
            quint8 previousMovedUnit,direction;
            if((movePacketKickTotalCache+movePacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitMove)
            {
                errorOutput("Too many move in sort time, check DDOS limit");
                return;
            }
            movePacketKickNewValue++;
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
            {
                parseError("Wrong size in move packet");
                return;
            }
            in >> previousMovedUnit;
            in >> direction;
            if(direction<1 || direction>8)
            {
                parseError(QStringLiteral("Bad direction number: %1").arg(direction));
                return;
            }
            moveThePlayer(previousMovedUnit,static_cast<Direction>(direction));
        }
        break;
        case 0x61:
        {
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
            {
                errorOutput("Too many packet in sort time, check DDOS limit");
                return;
            }
            otherPacketKickNewValue++;
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
            {
                parseError("Wrong size in move packet");
                return;
            }
            quint32 skill;
            in >> skill;
            useSkill(skill);
        }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("remaining data: parsenormalOutput(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void Client::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!character_loaded)
    {
        //wrong protocol
        disconnectClient();
        //parseError(QStringLiteral("is not logged, parsenormalOutput(%1,%2)").arg(mainCodeType).arg(subCodeType));
        return;
    }
    if(mainCodeType!=0x42 && subCodeType!=0x0003)
    {
        if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
        {
            errorOutput("Too many packet in sort time, check DDOS limit");
            return;
        }
        otherPacketKickNewValue++;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parsenormalOutput(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
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
                if((chatPacketKickTotalCache+chatPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitChat)
                {
                    errorOutput("Too many chat in sort time, check DDOS limit");
                    return;
                }
                chatPacketKickNewValue++;
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
                    if(CommonSettings::commonSettings.chat_allow_private)
                    {
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                            return;
                        QString text;
                        in >> text;
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                            return;
                        QString pseudo;
                        in >> pseudo;
                        normalOutput(Client::text_slashpmspace+pseudo+Client::text_space+text);
                        sendPM(text,pseudo);
                    }
                    else
                    {
                        parseError("can't send pm because is disabled: "+QString::number(chatType));
                        return;
                    }
                }
                else
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString text;
                    in >> text;

                    if(!text.startsWith(Client::text_slash))
                    {
                        if(chatType==Chat_type_local)
                        {
                            if(CommonSettings::commonSettings.chat_allow_local)
                                sendLocalChatText(text);
                            else
                            {
                                parseError("can't send chat local because is disabled: "+QString::number(chatType));
                                return;
                            }
                        }
                        else
                        {
                            if(CommonSettings::commonSettings.chat_allow_clan || CommonSettings::commonSettings.chat_allow_all)
                                sendChatText((Chat_type)chatType,text);
                            else
                            {
                                parseError("can't send chat other because is disabled: "+QString::number(chatType));
                                return;
                            }
                        }
                    }
                    else
                    {
                        if(text.contains(commandRegExp))
                        {
                            //isolate the main command (the first word)
                            QString command=text;
                            command.replace(commandRegExp,Client::text_regexresult1);

                            //isolate the arguements
                            if(text.contains(commandRegExp))
                            {
                                text.replace(commandRegExp,Client::text_regexresult2);
                                text.replace(isolateTheMainCommand,Client::text_regexresult1);
                            }
                            else
                                text=QString();

                            //the normal player command
                            {
                                if(command==Client::text_playernumber)
                                {
                                    sendBroadCastCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                    return;
                                }
                                else if(command==Client::text_playerlist)
                                {
                                    sendBroadCastCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+" "+text);
                                    return;
                                }
                                else if(command==Client::text_trade)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                    return;
                                }
                                else if(command==Client::text_battle)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                    return;
                                }
                            }
                            //the admin command
                            if(public_and_private_informations.public_informations.type==Player_type_gm || public_and_private_informations.public_informations.type==Player_type_dev)
                            {
                                if(command==Client::text_give)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_setevent)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_take)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_tp)
                                {
                                    sendHandlerCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_kick)
                                {
                                    sendBroadCastCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_chat)
                                {
                                    sendBroadCastCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_setrights)
                                {
                                    sendBroadCastCommand(command,text);
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else if(command==Client::text_stop || command==Client::text_restart)
                                {
                                    #ifndef EPOLLCATCHCHALLENGERSERVER
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    #endif
                                    normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                                }
                                else
                                {
                                    normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                                    receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                                }
                            }
                            else
                            {
                                normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                                receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                            }
                        }
                        else
                            normalOutput(Client::text_commands_seem_not_right+text);
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
                    parseError("wrong remaining size for clan invite");
                    return;
                }
                quint8 returnCode;
                in >> returnCode;
                switch(returnCode)
                {
                    case 0x01:
                        clanInvite(true);
                    break;
                    case 0x02:
                        clanInvite(false);
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
                    destroyObject(itemId,quantity);
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
                            tradeAddTradeCash(cash);
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
                            tradeAddTradeObject(item,quantity);
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
                            tradeAddTradeMonster(monsterId);
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
                    tradeFinished();
                break;
                //trade canceled after the accept
                case 0x0005:
                    tradeCanceled();
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
                    wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
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
                    tryEscape();
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
                    learnSkill(monsterId,skill);
                }
                break;
                //Heal all the monster
                case 0x0006:
                    heal();
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
                    requestFight(fightId);
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
                    moveMonster(moveUp,position);
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
                    changeOfMonsterInFight(monsterId);
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
                    confirmEvolution(monsterId);
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
                    useObjectOnMonster(item,monsterId);
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
                    newQuestAction(QuestAction_Start,questId);
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
                    newQuestAction(QuestAction_Finish,questId);
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
                    newQuestAction(QuestAction_Cancel,questId);
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
                    newQuestAction(QuestAction_NextStep,questId);
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
                        waitingForCityCaputre(false);
                    else
                        waitingForCityCaputre(true);
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
        parseError(QStringLiteral("remaining data: parsenormalOutput(%1,%2,%3): %4 %5")
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
void Client::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    parseError(QStringLiteral("no query with only the main code for now, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    const bool goodQueryBeforeCharacterLoaded=mainCodeType==0x02 &&
            (subCodeType==0x03 ||
             subCodeType==0x04 ||
             subCodeType==0x0C ||
             subCodeType==0x05
                );
    if(account_id==0 || (!character_loaded && !goodQueryBeforeCharacterLoaded))
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
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
                addCharacter(queryNumber,profileIndex,pseudo,skin);
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
                removeCharacter(queryNumber,characterId);
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
                selectCharacter(queryNumber,characterId);
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
                datapackList(queryNumber,files,timestamps);
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
                        clanAction(queryNumber,clanActionId,tempString);
                    }
                    break;
                    case 0x02:
                    case 0x03:
                        clanAction(queryNumber,clanActionId,QString());
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
                plantSeed(queryNumber,plant_id);
            }
            break;
            //Collect mature plant
            case 0x0007:
                collectPlant(queryNumber);
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
                useRecipe(queryNumber,recipe_id);
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
                useObject(queryNumber,objectId);
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
                getShopList(queryNumber,shopId);
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
                buyObject(queryNumber,shopId,objectId,quantity,price);
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
                sellObject(queryNumber,shopId,objectId,quantity,price);
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
                getFactoryList(queryNumber,factoryId);
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
                buyFactoryProduct(queryNumber,factoryId,objectId,quantity,price);
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
                sellFactoryResource(queryNumber,factoryId,objectId,quantity,price);
            }
            break;
            case 0x0010:
                getMarketList(queryNumber);
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
                    buyMarketObject(queryNumber,marketObjectId,quantity);
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
                    buyMarketMonster(queryNumber,monsterId);
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
                    putMarketObject(queryNumber,objectId,quantity,price);
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
                    putMarketMonster(queryNumber,monsterId,price);
                }
            }
            break;
            case 0x0013:
                recoverMarketCash(queryNumber);
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
                    withdrawMarketObject(queryNumber,objectId,quantity);
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
                    withdrawMarketMonster(queryNumber,monsterId);
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
void Client::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    parseError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    if(stopIt)
        return;
    if(!character_loaded)
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
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
                normalOutput("teleportValidatedTo() from protocol");
                teleportValidatedTo(lastTeleportation.first().map,lastTeleportation.first().x,lastTeleportation.first().y,lastTeleportation.first().orientation);
                lastTeleportation.removeFirst();
            break;
            //Event change
            case 0x0002:
                removeFirstEventInQueue();
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
                            tradeAccepted();
                        break;
                        case 0x02:
                            tradeCanceled();
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
                            battleAccepted();
                        break;
                        case 0x02:
                            battleCanceled();
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

void Client::parseError(const QString &errorString)
{
    if(GlobalServerData::serverSettings.tolerantMode)
        normalOutput(QStringLiteral("Packed dropped, due to: %1").arg(errorString));
    else
    {
        errorOutput(errorString);
        return;
    }
}

/// \warning it need be complete protocol trame
void Client::fake_receive_data(const QByteArray &data)
{
    Q_UNUSED(data);
//	parseInputAfterLogin(data);
}

void Client::purgeReadBuffer()
{
    canStartReadData=true;
    parseIncommingData();
}
