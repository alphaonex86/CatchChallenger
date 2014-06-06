#include "ClientNetworkRead.h"
#include "GlobalServerData.h"
#include "MapServer.h"
#include "ClientNetworkReadWithoutSender.h"
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "Client.h"
#define dbQuery(a) client->clientHeavyLoad.dbQuery(a)
#define message(a) client->normalOutput(a)
#endif

using namespace CatchChallenger;

QRegularExpression ClientNetworkRead::commandRegExp=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)?$"));
QRegularExpression ClientNetworkRead::commandRegExpWithArgs=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)$"));
QRegularExpression ClientNetworkRead::isolateTheMainCommand=QRegularExpression(QLatin1String("^ (.*)$"));

QString ClientNetworkRead::text_server_full=QLatin1String("Server full");
QString ClientNetworkRead::text_slashpmspace=QLatin1String("/pm ");
QString ClientNetworkRead::text_space=QLatin1String(" ");
QString ClientNetworkRead::text_slash=QLatin1String("/");
QString ClientNetworkRead::text_regexresult1=QLatin1String("\\1");
QString ClientNetworkRead::text_regexresult2=QLatin1String("\\2");
QString ClientNetworkRead::text_send_command_slash=QLatin1String("send command: /");
QString ClientNetworkRead::text_playernumber=QLatin1String("playernumber");
QString ClientNetworkRead::text_playerlist=QLatin1String("playerlist");
QString ClientNetworkRead::text_trade=QLatin1String("trade");
QString ClientNetworkRead::text_battle=QLatin1String("battle");
QString ClientNetworkRead::text_give=QLatin1String("give");
QString ClientNetworkRead::text_setevent=QLatin1String("setevent");
QString ClientNetworkRead::text_take=QLatin1String("take");
QString ClientNetworkRead::text_tp=QLatin1String("tp");
QString ClientNetworkRead::text_kick=QLatin1String("kick");
QString ClientNetworkRead::text_chat=QLatin1String("chat");
QString ClientNetworkRead::text_setrights=QLatin1String("setrights");
QString ClientNetworkRead::text_stop=QLatin1String("stop");
QString ClientNetworkRead::text_restart=QLatin1String("restart");
QString ClientNetworkRead::text_unknown_send_command_slash=QLatin1String("unknown send command: /");
QString ClientNetworkRead::text_commands_seem_not_right=QLatin1String("commands seem not right: /");

ClientNetworkRead::ClientNetworkRead(Player_internal_informations *player_informations,ConnectedSocket * socket) :
    ProtocolParsingInput(socket,PacketModeTransmission_Server),
    have_send_protocol(false),
    is_logging_in_progess(false),
    stopIt(false),
    #ifndef EPOLLCATCHCHALLENGERSERVER
    socket(socket),
    #endif
    player_informations(player_informations),
    movePacketKickTotalCache(0),
    movePacketKickNewValue(0),
    chatPacketKickTotalCache(0),
    chatPacketKickNewValue(0),
    otherPacketKickTotalCache(0),
    otherPacketKickNewValue(0)
{
    {
        //int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        int index=0;
        while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
        {
            movePacketKick << 0;
            chatPacketKick << 0;
            otherPacketKick << 0;
            index++;
        }
    }
    queryNumberList.reserve(256);
    {
        int index=0;
        while(index<256)
        {
            queryNumberList << index;
            index++;
        }
    }
}

void ClientNetworkRead::doDDOSCompute()
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

void ClientNetworkRead::stopRead()
{
    stopIt=true;
}

void ClientNetworkRead::askIfIsReadyToStop()
{
    ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.clientList.removeOne(this);
    stopIt=true;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    /*emit */isReadyToStop();
    #endif
}

void ClientNetworkRead::sendNewEvent(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput(QStringLiteral("Sorry, no free query number to send this query of sendNewEvent"));
        #else
        /*emit */error(QStringLiteral("Sorry, no free query number to send this query of sendNewEvent"));
        #endif
        return;
    }
    /*emit */sendQuery(0x79,0x0002,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::teleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    if(queryNumberList.empty())
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput(QStringLiteral("Sorry, no free query number to send this query of teleportation"));
        #else
        /*emit */error(QStringLiteral("Sorry, no free query number to send this query of teleportation"));
        #endif
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
    /*emit */sendQuery(0x79,0x0001,queryNumberList.first(),outputData);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::sendTradeRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        #else
        /*emit */error(QStringLiteral("Sorry, no free query number to send this query of trade"));
        #endif
        return;
    }
    /*emit */sendQuery(0x80,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::sendBattleRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        #else
        /*emit */error(QStringLiteral("Sorry, no free query number to send this query of trade"));
        #endif
        return;
    }
    /*emit */sendQuery(0x90,0x0001,queryNumberList.first(),data);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    /*emit */message(QStringLiteral("parseInputBeforeLogin(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput("Too many packet in sort time, check DDOS limit");
        #else
        /*emit */error("Too many packet in sort time, check DDOS limit");
        #endif
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
                        out << ClientNetworkRead::text_server_full;
                        /*emit */postReply(queryNumber,outputData);
                        #ifdef EPOLLCATCHCHALLENGERSERVER
                        client->errorOutput(ClientNetworkRead::text_server_full);
                        #else
                        /*emit */error(ClientNetworkRead::text_server_full);
                        #endif
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
                                #ifdef EPOLLCATCHCHALLENGERSERVER
                                client->errorOutput("Compression selected wrong");
                                #else
                                /*emit */error("Compression selected wrong");
                                #endif
                            return;
                        }
                        /*emit */postReply(queryNumber,outputData);
                        have_send_protocol=true;
                        ClientNetworkReadWithoutSender::clientNetworkReadWithoutSender.clientList << this;
                        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                        /*emit */message(QStringLiteral("Protocol sended and replied"));
                        #endif
                    }
                    else
                    {
                        out << (quint8)0x02;		//protocol not supported
                        /*emit */postReply(queryNumber,outputData);
                        #ifdef EPOLLCATCHCHALLENGERSERVER
                        client->errorOutput("Wrong protocol");
                        #else
                        /*emit */error("Wrong protocol");
                        #endif
                        return;
                    }
                }
            break;
            case 0x0002:
                if(!have_send_protocol)
                {
                    #ifdef EPOLLCATCHCHALLENGERSERVER
                    client->errorOutput("send login before the protocol");
                    #else
                    /*emit */error("send login before the protocol");
                    #endif
                    return;
                }
                if((data.size()-in.device()->pos())!=(64*2))
                    parseError(QStringLiteral("wrong size with the main ident: %1, because %2 != 20").arg(mainCodeType).arg(data.size()-in.device()->pos()));
                else if(is_logging_in_progess)
                {
                    out << (quint8)1;
                    /*emit */postReply(queryNumber,outputData);
                    #ifdef EPOLLCATCHCHALLENGERSERVER
                    client->errorOutput("Loggin in progress");
                    #else
                    /*emit */error("Loggin in progress");
                    #endif
                }
                else if(player_informations->character_loaded)
                {
                    out << (quint8)1;
                    /*emit */postReply(queryNumber,outputData);
                    #ifdef EPOLLCATCHCHALLENGERSERVER
                    client->errorOutput("Already logged");
                    #else
                    /*emit */error("Already logged");
                    #endif
                }
                else
                {
                    is_logging_in_progess=true;
                    QByteArray data_extracted(data.right(data.size()-in.device()->pos()));
                    const QByteArray &login=data_extracted.mid(0,64);
                    data_extracted.remove(0,64);
                    const QByteArray &hash=data_extracted;
                    /*emit */askLogin(queryNumber,login,hash);
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
        /*emit */needDisconnectTheClient();
        //parseError(QStringLiteral("is not logged, parseMessage(%1)").arg(mainCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    /*emit */message(QStringLiteral("parseMessage(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x40:
        {
            if((movePacketKickTotalCache+movePacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitMove)
            {
                #ifdef EPOLLCATCHCHALLENGERSERVER
                client->errorOutput("Too many move in sort time, check DDOS limit");
                #else
                /*emit */error("Too many move in sort time, check DDOS limit");
                #endif
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
        }
        if(direction<1 || direction>8)
        {
            parseError(QStringLiteral("Bad direction number: %1").arg(direction));
            return;
        }
        /*emit */moveThePlayer(previousMovedUnit,(Direction)direction);
        break;
        case 0x61:
        {
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
            {
                #ifdef EPOLLCATCHCHALLENGERSERVER
                client->errorOutput("Too many packet in sort time, check DDOS limit");
                #else
                /*emit */error("Too many packet in sort time, check DDOS limit");
                #endif
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
            /*emit */useSkill(skill);
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
        /*emit */needDisconnectTheClient();
        //parseError(QStringLiteral("is not logged, parseMessage(%1,%2)").arg(mainCodeType).arg(subCodeType));
        return;
    }
    if(mainCodeType!=0x42 && subCodeType!=0x0003)
    {
        if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
        {
            #ifdef EPOLLCATCHCHALLENGERSERVER
            client->errorOutput("Too many packet in sort time, check DDOS limit");
            #else
            /*emit */error("Too many packet in sort time, check DDOS limit");
            #endif
            return;
        }
        otherPacketKickNewValue++;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    /*emit */message(QStringLiteral("parseMessage(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
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
                    #ifdef EPOLLCATCHCHALLENGERSERVER
                    client->errorOutput("Too many chat in sort time, check DDOS limit");
                    #else
                    /*emit */error("Too many chat in sort time, check DDOS limit");
                    #endif
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
                        /*emit */message(ClientNetworkRead::text_slashpmspace+pseudo+ClientNetworkRead::text_space+text);
                        /*emit */sendPM(text,pseudo);
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

                    if(!text.startsWith(ClientNetworkRead::text_slash))
                    {
                        if(chatType==Chat_type_local)
                        {
                            if(CommonSettings::commonSettings.chat_allow_local)
                                /*emit */sendLocalChatText(text);
                            else
                            {
                                parseError("can't send chat local because is disabled: "+QString::number(chatType));
                                return;
                            }
                        }
                        else
                        {
                            if(CommonSettings::commonSettings.chat_allow_clan || CommonSettings::commonSettings.chat_allow_all)
                                /*emit */sendChatText((Chat_type)chatType,text);
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
                            command.replace(commandRegExp,ClientNetworkRead::text_regexresult1);

                            //isolate the arguements
                            if(text.contains(commandRegExp))
                            {
                                text.replace(commandRegExp,ClientNetworkRead::text_regexresult2);
                                text.replace(isolateTheMainCommand,ClientNetworkRead::text_regexresult1);
                            }
                            else
                                text=QString();

                            //the normal player command
                            {
                                if(command==ClientNetworkRead::text_playernumber)
                                {
                                    /*emit */sendBroadCastCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                    return;
                                }
                                else if(command==ClientNetworkRead::text_playerlist)
                                {
                                    /*emit */sendBroadCastCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+" "+text);
                                    return;
                                }
                                else if(command==ClientNetworkRead::text_trade)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                    return;
                                }
                                else if(command==ClientNetworkRead::text_battle)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                    return;
                                }
                            }
                            //the admin command
                            if(player_informations->public_and_private_informations.public_informations.type==Player_type_gm || player_informations->public_and_private_informations.public_informations.type==Player_type_dev)
                            {
                                if(command==ClientNetworkRead::text_give)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_setevent)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_take)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_tp)
                                {
                                    /*emit */sendHandlerCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_kick)
                                {
                                    /*emit */sendBroadCastCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_chat)
                                {
                                    /*emit */sendBroadCastCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_setrights)
                                {
                                    /*emit */sendBroadCastCommand(command,text);
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else if(command==ClientNetworkRead::text_stop || command==ClientNetworkRead::text_restart)
                                {
                                    #ifndef EPOLLCATCHCHALLENGERSERVER
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    #endif
                                    /*emit */message(ClientNetworkRead::text_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                                else
                                {
                                    /*emit */message(ClientNetworkRead::text_unknown_send_command_slash+command+ClientNetworkRead::text_space+text);
                                    receiveSystemText(ClientNetworkRead::text_unknown_send_command_slash+command+ClientNetworkRead::text_space+text);
                                }
                            }
                            else
                            {
                                /*emit */message(ClientNetworkRead::text_unknown_send_command_slash+command+ClientNetworkRead::text_space+text);
                                receiveSystemText(ClientNetworkRead::text_unknown_send_command_slash+command+ClientNetworkRead::text_space+text);
                            }
                        }
                        else
                            /*emit */message(ClientNetworkRead::text_commands_seem_not_right+text);
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
                        /*emit */clanInvite(true);
                    break;
                    case 0x02:
                        /*emit */clanInvite(false);
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
                    /*emit */destroyObject(itemId,quantity);
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
                            /*emit */tradeAddTradeCash(cash);
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
                            /*emit */tradeAddTradeObject(item,quantity);
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
                            /*emit */tradeAddTradeMonster(monsterId);
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
                    /*emit */tradeFinished();
                break;
                //trade canceled after the accept
                case 0x0005:
                    /*emit */tradeCanceled();
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
                    /*emit */wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
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
                    /*emit */tryEscape();
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
                    /*emit */learnSkill(monsterId,skill);
                }
                break;
                //Heal all the monster
                case 0x0006:
                    /*emit */heal();
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
                    /*emit */requestFight(fightId);
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
                    /*emit */moveMonster(moveUp,position);
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
                    /*emit */changeOfMonsterInFight(monsterId);
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
                    /*emit */confirmEvolution(monsterId);
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
                    /*emit */useObjectOnMonster(item,monsterId);
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
                    /*emit */newQuestAction(QuestAction_Start,questId);
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
                    /*emit */newQuestAction(QuestAction_Finish,questId);
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
                    /*emit */newQuestAction(QuestAction_Cancel,questId);
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
                    /*emit */newQuestAction(QuestAction_NextStep,questId);
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
                        /*emit */waitingForCityCaputre(false);
                    else
                        /*emit */waitingForCityCaputre(true);
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
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput("Too many packet in sort time, check DDOS limit");
        #else
        /*emit */error("Too many packet in sort time, check DDOS limit");
        #endif
        return;
    }
    otherPacketKickNewValue++;
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    /*emit */message(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
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
                /*emit */addCharacter(queryNumber,profileIndex,pseudo,skin);
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
                /*emit */removeCharacter(queryNumber,characterId);
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
                /*emit */selectCharacter(queryNumber,characterId);
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
                /*emit */datapackList(queryNumber,files,timestamps);
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
                        /*emit */clanAction(queryNumber,clanActionId,tempString);
                    }
                    break;
                    case 0x02:
                    case 0x03:
                        /*emit */clanAction(queryNumber,clanActionId,QString());
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
                /*emit */plantSeed(queryNumber,plant_id);
            }
            break;
            //Collect mature plant
            case 0x0007:
                /*emit */collectPlant(queryNumber);
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
                /*emit */useRecipe(queryNumber,recipe_id);
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
                /*emit */useObject(queryNumber,objectId);
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
                /*emit */getShopList(queryNumber,shopId);
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
                /*emit */buyObject(queryNumber,shopId,objectId,quantity,price);
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
                /*emit */sellObject(queryNumber,shopId,objectId,quantity,price);
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
                /*emit */getFactoryList(queryNumber,factoryId);
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
                /*emit */buyFactoryObject(queryNumber,factoryId,objectId,quantity,price);
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
                /*emit */sellFactoryObject(queryNumber,factoryId,objectId,quantity,price);
            }
            break;
            case 0x0010:
                /*emit */getMarketList(queryNumber);
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
                    /*emit */buyMarketObject(queryNumber,marketObjectId,quantity);
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
                    /*emit */buyMarketMonster(queryNumber,monsterId);
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
                    /*emit */putMarketObject(queryNumber,objectId,quantity,price);
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
                    /*emit */putMarketMonster(queryNumber,monsterId,price);
                }
            }
            break;
            case 0x0013:
                /*emit */recoverMarketCash(queryNumber);
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
                    /*emit */withdrawMarketObject(queryNumber,objectId,quantity);
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
                    /*emit */withdrawMarketMonster(queryNumber,monsterId);
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
    /*emit */message(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
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
                /*emit */message("/*emit */teleportValidatedTo() from protocol");
                /*emit */teleportValidatedTo(lastTeleportation.first().map,lastTeleportation.first().x,lastTeleportation.first().y,lastTeleportation.first().orientation);
                lastTeleportation.removeFirst();
            break;
            //Event change
            case 0x0002:
                /*emit */removeFirstEventInQueue();
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
                            /*emit */tradeAccepted();
                        break;
                        case 0x02:
                            /*emit */tradeCanceled();
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
                            /*emit */battleAccepted();
                        break;
                        case 0x02:
                            /*emit */battleCanceled();
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
        /*emit */message(QStringLiteral("Packed dropped, due to: %1").arg(errorString));
    else
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        client->errorOutput(errorString);
        #else
        /*emit */error(errorString);
        #endif
    }
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
    /*emit */sendFullPacket(0xC2,0x0005,outputData);
}

//signals for epoll
#ifdef EPOLLCATCHCHALLENGERSERVER
//normal signals
void ClientNetworkRead::needDisconnectTheClient()
{
    client->disconnectClient();
}

void ClientNetworkRead::sendFullPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data) const
{
    client->clientNetworkWrite.sendFullPacket(mainCodeType,subCodeType,data);
}

void ClientNetworkRead::sendPacket(const quint8 &mainCodeType,const QByteArray &data) const
{
    client->clientNetworkWrite.sendPacket(mainCodeType,data);
}

void ClientNetworkRead::sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data) const
{
    client->clientNetworkWrite.sendQuery(mainIdent,subIdent,queryNumber,data);
}

//send reply
void ClientNetworkRead::postReply(const quint8 &queryNumber,const QByteArray &data) const
{
    client->clientNetworkWrite.postReply(queryNumber,data);
}

//packet parsed (heavy)
void ClientNetworkRead::askLogin(const quint8 &query_id,const QByteArray &login,const QByteArray &hash) const
{
    client->clientHeavyLoad.askLogin(query_id,login,hash);
}

void ClientNetworkRead::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps) const
{
    client->clientHeavyLoad.datapackList(query_id,files,timestamps);
}

//packet parsed (map management)
void ClientNetworkRead::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction) const
{
    client->clientMapManagement->moveThePlayer(previousMovedUnit,direction);
    client->localClientHandler.moveThePlayer(previousMovedUnit,direction);
    client->clientLocalBroadcast.moveThePlayer(previousMovedUnit,direction);
}

void ClientNetworkRead::teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation) const
{
    client->localClientHandler.teleportValidatedTo(map,x,y,orientation);
    client->clientLocalBroadcast.teleportValidatedTo(map,x,y,orientation);
    client->clientMapManagement->teleportValidatedTo(map,x,y,orientation);
}

//reply
void ClientNetworkRead::removeFirstEventInQueue() const
{
    client->localClientHandler.removeFirstEventInQueue();
}

//character
void ClientNetworkRead::addCharacter(const quint8 &query_id, const quint8 &profileIndex,const QString &pseudo,const QString &skin) const
{
    client->clientHeavyLoad.addCharacter(query_id,profileIndex,pseudo,skin);
}

void ClientNetworkRead::removeCharacter(const quint8 &query_id, const quint32 &characterId) const
{
    client->clientHeavyLoad.removeCharacter(query_id,characterId);
}

void ClientNetworkRead::selectCharacter(const quint8 &query_id, const quint32 &characterId) const
{
    client->clientHeavyLoad.selectCharacter(query_id,characterId);
}

//trade
void ClientNetworkRead::tradeCanceled() const
{
    client->localClientHandler.tradeCanceled();
}

void ClientNetworkRead::tradeAccepted() const
{
    client->localClientHandler.tradeAccepted();
}

void ClientNetworkRead::tradeFinished() const
{
    client->localClientHandler.tradeFinished();
}

void ClientNetworkRead::tradeAddTradeCash(const quint64 &cash) const
{
    client->localClientHandler.tradeAddTradeCash(cash);
}

void ClientNetworkRead::tradeAddTradeObject(const quint32 &item,const quint32 &quantity) const
{
    client->localClientHandler.tradeAddTradeObject(item,quantity);
}

void ClientNetworkRead::tradeAddTradeMonster(const quint32 &monsterId) const
{
    client->localClientHandler.tradeAddTradeMonster(monsterId);
}

//packet parsed (broadcast)
void ClientNetworkRead::sendPM(const QString &text,const QString &pseudo) const
{
    client->clientBroadCast.sendPM(text,pseudo);
}

void ClientNetworkRead::sendChatText(const Chat_type &chatType,const QString &text) const
{
    client->clientBroadCast.sendChatText(chatType,text);
}

void ClientNetworkRead::sendLocalChatText(const QString &text) const
{
    client->clientLocalBroadcast.sendLocalChatText(text);
}

void ClientNetworkRead::sendBroadCastCommand(const QString &command,const QString &extraText) const
{
    client->clientBroadCast.sendBroadCastCommand(command,extraText);
}

void ClientNetworkRead::sendHandlerCommand(const QString &command,const QString &extraText) const
{
    client->localClientHandler.sendHandlerCommand(command,extraText);
}

//plant
void ClientNetworkRead::plantSeed(const quint8 &query_id,const quint8 &plant_id) const
{
    client->clientLocalBroadcast.plantSeed(query_id,plant_id);
}

void ClientNetworkRead::collectPlant(const quint8 &query_id) const
{
    client->clientLocalBroadcast.collectPlant(query_id);
}

//crafting
void ClientNetworkRead::useRecipe(const quint8 &query_id,const quint32 &recipe_id) const
{
    client->localClientHandler.useRecipe(query_id,recipe_id);
}

//inventory
void ClientNetworkRead::destroyObject(const quint32 &itemId,const quint32 &quantity) const
{
    client->localClientHandler.destroyObject(itemId,quantity);
}

void ClientNetworkRead::useObject(const quint8 &query_id,const quint32 &itemId) const
{
    client->localClientHandler.useObject(query_id,itemId);
}

void ClientNetworkRead::useObjectOnMonster(const quint32 &object,const quint32 &monster) const
{
    client->localClientHandler.useObjectOnMonster(object,monster);
}

void ClientNetworkRead::wareHouseStore(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters) const
{
    client->localClientHandler.wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
}

//shop
void ClientNetworkRead::getShopList(const quint32 &query_id,const quint32 &shopId) const
{
    client->localClientHandler.getShopList(query_id,shopId);
}

void ClientNetworkRead::buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const
{
    client->localClientHandler.buyObject(query_id,shopId,objectId,quantity,price);
}

void ClientNetworkRead::sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const
{
    client->localClientHandler.sellObject(query_id,shopId,objectId,quantity,price);
}

//factory
void ClientNetworkRead::getFactoryList(const quint32 &query_id,const quint32 &shopId) const
{
    client->localClientHandler.getFactoryList(query_id,shopId);
}

void ClientNetworkRead::buyFactoryObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const
{
    client->localClientHandler.buyFactoryProduct(query_id,shopId,objectId,quantity,price);
}

void ClientNetworkRead::sellFactoryObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const
{
    client->localClientHandler.sellFactoryResource(query_id,shopId,objectId,quantity,price);
}

//fight
void ClientNetworkRead::tryEscape() const
{
    client->localClientHandler.tryEscape();
}

void ClientNetworkRead::useSkill(const quint32 &skill) const
{
    client->localClientHandler.useSkill(skill);
}

void ClientNetworkRead::learnSkill(const quint32 &monsterId,const quint32 &skill) const
{
    client->localClientHandler.learnSkill(monsterId,skill);
}

void ClientNetworkRead::heal() const
{
    client->localClientHandler.heal();
}

void ClientNetworkRead::requestFight(const quint32 &fightId) const
{
    client->localClientHandler.requestFight(fightId);
}

void ClientNetworkRead::moveMonster(const bool &up,const quint8 &number) const
{
    client->localClientHandler.moveMonster(up,number);
}

void ClientNetworkRead::changeOfMonsterInFight(const quint32 &monsterId) const
{
    client->localClientHandler.changeOfMonsterInFight(monsterId);
}

void ClientNetworkRead::confirmEvolution(const quint32 &monsterId)
{
    client->localClientHandler.confirmEvolution(monsterId);
}

//quest
void ClientNetworkRead::newQuestAction(const QuestAction &action,const quint32 &questId) const
{
    client->localClientHandler.newQuestAction(action,questId);
}

//battle
void ClientNetworkRead::battleCanceled() const
{
    client->localClientHandler.battleCanceled();
}

void ClientNetworkRead::battleAccepted() const
{
    client->localClientHandler.battleAccepted();
}

//clan
void ClientNetworkRead::clanAction(const quint8 &query_id,const quint8 &action,const QString &text) const
{
    client->localClientHandler.clanAction(query_id,action,text);
}

void ClientNetworkRead::clanInvite(const bool &accept) const
{
    client->localClientHandler.clanInvite(accept);
}

void ClientNetworkRead::waitingForCityCaputre(const bool &cancel) const
{
    client->localClientHandler.waitingForCityCaputre(cancel);
}

//market
void ClientNetworkRead::getMarketList(const quint32 &query_id) const
{
    client->localClientHandler.getMarketList(query_id);
}

void ClientNetworkRead::buyMarketObject(const quint32 &query_id,const quint32 &marketObjectId,const quint32 &quantity) const
{
    client->localClientHandler.buyMarketObject(query_id,marketObjectId,quantity);
}

void ClientNetworkRead::buyMarketMonster(const quint32 &query_id,const quint32 &monsterId) const
{
    client->localClientHandler.buyMarketMonster(query_id,monsterId);
}

void ClientNetworkRead::putMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const
{
    client->localClientHandler.putMarketObject(query_id,objectId,quantity,price);
}

void ClientNetworkRead::putMarketMonster(const quint32 &query_id,const quint32 &monsterId,const quint32 &price) const
{
    client->localClientHandler.putMarketMonster(query_id,monsterId,price);
}

void ClientNetworkRead::recoverMarketCash(const quint32 &query_id) const
{
    client->localClientHandler.recoverMarketCash(query_id);
}

void ClientNetworkRead::withdrawMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity) const
{
    client->localClientHandler.withdrawMarketObject(query_id,objectId,quantity);
}

void ClientNetworkRead::withdrawMarketMonster(const quint32 &query_id,const quint32 &monsterId) const
{
    client->localClientHandler.withdrawMarketMonster(query_id,monsterId);
}
#endif
