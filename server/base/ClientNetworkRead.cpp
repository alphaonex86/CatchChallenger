#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::doDDOSCompute()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue<0 || GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue>CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        qDebug() << "GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue out of range:" << GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        return;
    }
    #endif
    {
        movePacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
            {
                errorOutput("index out of range <0, movePacketKick");
                return;
            }
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
            {
                errorOutput("index out of range >, movePacketKick");
                return;
            }
            if(movePacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitMove*2)
            {
                errorOutput(QString("index out of range in array for index %1, movePacketKick").arg(movePacketKick[index]));
                return;
            }
            #endif
            movePacketKick[index]=movePacketKick[index+1];
            movePacketKickTotalCache+=movePacketKick[index];
            index++;
        }
        movePacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=movePacketKickNewValue;
        movePacketKickTotalCache+=movePacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(movePacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitMove*2)
        {
            errorOutput("bug in DDOS calculation count");
            return;
        }
        #endif
        movePacketKickNewValue=0;
    }
    {
        chatPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, chatPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, chatPacketKick";
            if(chatPacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitChat*2)
                qDebug() << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick";
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitChat*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
        chatPacketKickNewValue=0;
    }
    {
        otherPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, otherPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, otherPacketKick";
            if(otherPacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitOther*2)
                qDebug() << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick";
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitOther*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
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

void Client::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(size);
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
    switch(mainCodeType)
    {
        case 0x03:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(queryNumber);
            #endif
            if(GlobalServerData::serverPrivateVariables.connected_players>=GlobalServerData::serverSettings.max_players)
            {
                *(Client::protocolReplyServerFull+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyServerFull),sizeof(Client::protocolReplyServerFull));
                disconnectClient();
                //errorOutput(Client::text_server_full);DDOS -> full the log
                return;
            }
            //if lot of un logged connection, remove the first
            if(GlobalServerData::serverPrivateVariables.tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
            {
                Client *client=static_cast<Client *>(GlobalServerData::serverPrivateVariables.tokenForAuth[0].client);
                client->disconnectClient();
                delete client;
                GlobalServerData::serverPrivateVariables.tokenForAuthSize--;
                if(GlobalServerData::serverPrivateVariables.tokenForAuthSize>0)
                {
                    quint32 index=0;
                    while(index<GlobalServerData::serverPrivateVariables.tokenForAuthSize)
                    {
                        GlobalServerData::serverPrivateVariables.tokenForAuth[index]=GlobalServerData::serverPrivateVariables.tokenForAuth[index+1];
                        index++;
                    }
                    //don't work:memmove(GlobalServerData::serverPrivateVariables.tokenForAuth,GlobalServerData::serverPrivateVariables.tokenForAuth+sizeof(TokenLink),GlobalServerData::serverPrivateVariables.tokenForAuthSize*sizeof(TokenLink));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(GlobalServerData::serverPrivateVariables.tokenForAuth[0].client==NULL)
                        abort();
                    #endif
                }
                return;
            }
            if(memcmp(data,Client::protocolHeaderToMatch,sizeof(Client::protocolHeaderToMatch))==0)
            {
                TokenLink *token=&GlobalServerData::serverPrivateVariables.tokenForAuth[GlobalServerData::serverPrivateVariables.tokenForAuthSize];
                {
                    token->client=this;
                    #ifdef Q_OS_LINUX
                    if(GlobalServerData::serverPrivateVariables.fpRandomFile==NULL)
                    {
                        int index=0;
                        while(index<CATCHCHALLENGER_TOKENSIZE)
                        {
                            token->value[index]=rand()%256;
                            index++;
                        }
                    }
                    else
                        fread(token->value,CATCHCHALLENGER_TOKENSIZE,1,GlobalServerData::serverPrivateVariables.fpRandomFile);
                    #else
                    int index=0;
                    while(index<CATCHCHALLENGER_TOKENSIZE)
                    {
                        token->value[index]=rand()%256;
                        index++;
                    }
                    #endif
                }
                switch(ProtocolParsing::compressionType)
                {
                    case CompressionType_None:
                        *(Client::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(Client::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                    break;
                    case CompressionType_Zlib:
                        *(Client::protocolReplyCompresssionZlib+1)=queryNumber;
                        memcpy(Client::protocolReplyCompresssionZlib+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompresssionZlib),sizeof(Client::protocolReplyCompresssionZlib));
                    break;
                    case CompressionType_Xz:
                        *(Client::protocolReplyCompressionXz+1)=queryNumber;
                        memcpy(Client::protocolReplyCompressionXz+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionXz),sizeof(Client::protocolReplyCompressionXz));
                    break;
                    default:
                        errorOutput("Compression selected wrong");
                    return;
                }
                GlobalServerData::serverPrivateVariables.tokenForAuthSize++;
                have_send_protocol=true;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                *(Client::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyProtocolNotSupported),sizeof(Client::protocolReplyProtocolNotSupported));
                errorOutput("Wrong protocol");
                return;
            }
        break;
        case 0x04:
            if(!have_send_protocol)
            {
                errorOutput("send login before the protocol");
                return;
            }
            if(is_logging_in_progess)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                *(Client::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginInProgressBuffer),sizeof(Client::loginInProgressBuffer));
                errorOutput("Loggin already in progress");
            }
            else
            {
                is_logging_in_progess=true;
                askLogin(queryNumber,data);
                return;
            }
        break;
        case 0x05:
            if(!have_send_protocol)
            {
                errorOutput("send login before the protocol");
                return;
            }
            if(is_logging_in_progess)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                *(Client::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginInProgressBuffer),sizeof(Client::loginInProgressBuffer));
                errorOutput("Loggin already in progress");
            }
            else
            {
                if(GlobalServerData::serverSettings.automatic_account_creation)
                {
                    is_logging_in_progess=true;
                    createAccount(queryNumber,data);
                    return;
                }
                else
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    *(Client::loginInProgressBuffer+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginInProgressBuffer),sizeof(Client::loginInProgressBuffer));
                    errorOutput("Account creation not premited");
                }
            }
        break;
        default:
            parseError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void Client::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
{
    if(stopIt)
        return;
    if(account_id==0)
    {
        disconnectClient();
        return;
    }
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
    switch(mainCodeType)
    {
        case 0x40:
        {
            quint8 direction;
            if((movePacketKickTotalCache+movePacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitMove)
            {
                errorOutput("Too many move in sort time, check DDOS limit");
                return;
            }
            movePacketKickNewValue++;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(quint8)*2)
            {
                parseError("Wrong size in move packet");
                return;
            }
            #endif
            direction=*(data+sizeof(quint8));
            if(direction<1 || direction>8)
            {
                parseError(QStringLiteral("Bad direction number: %1").arg(direction));
                return;
            }
            moveThePlayer(static_cast<quint8>(*data),static_cast<Direction>(direction));
            return;
        }
        break;
        //Chat
        case 0x43:
        {
            QByteArray newData(data,size);
            QDataStream in(newData);
            in.setVersion(QDataStream::Qt_4_4);
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitChat)
            {
                errorOutput("Too many chat in sort time, check DDOS limit");
                return;
            }
            chatPacketKickNewValue++;
            if(size<((int)sizeof(quint8)))
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
                    QString text;
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError("wrong utf8 to QString size in PM for text size");
                            return;
                        }
                        quint8 textSize;
                        in >> textSize;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                        {
                            parseError("wrong utf8 to QString size in PM for text");
                            return;
                        }
                        QByteArray rawText=newData.mid(in.device()->pos(),textSize);
                        text=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                    }
                    QString pseudo;
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError("wrong utf8 to QString size in PM for pseudo");
                            return;
                        }
                        quint8 pseudoSize;
                        in >> pseudoSize;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                        {
                            parseError("wrong utf8 to QString size in PM for pseudo");
                            return;
                        }
                        QByteArray rawText=newData.mid(in.device()->pos(),pseudoSize);
                        pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                    }

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
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError("wrong utf8 to QString header size");
                    return;
                }
                quint8 textSize;
                in >> textSize;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                {
                    parseError("wrong utf8 to QString size");
                    return;
                }
                QByteArray rawText=newData.mid(in.device()->pos(),textSize);
                QString text=QString::fromUtf8(rawText.data(),rawText.size());
                in.device()->seek(in.device()->pos()+rawText.size());

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
        case 0x61:
        {
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
            {
                errorOutput("Too many packet in sort time, check DDOS limit");
                return;
            }
            otherPacketKickNewValue++;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(quint16))
            {
                parseError("Wrong size in move packet");
                return;
            }
            #endif
            if(size!=(int)sizeof(quint16))
            {
                parseError("Wrong size in move packet");
                return;
            }
            useSkill(be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data))));
            return;
        }
        break;
        default:
            parseError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void Client::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *rawData,const int &size)
{
    if(stopIt)
        return;
    if(account_id==0)
    {
        disconnectClient();
        return;
    }
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
    switch(mainCodeType)
    {
        case 0x42:
        switch(subCodeType)
        {
            //Clan invite accept
            case 0x0004:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=((int)sizeof(quint8)))
                {
                    parseError("wrong remaining size for clan invite");
                    return;
                }
                #endif
                const quint8 &returnCode=*(rawData+sizeof(quint8));
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
                return;
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
                    if(size!=((int)sizeof(quint16)+sizeof(quint32)))
                    {
                        parseError("wrong remaining size for destroy item id");
                        return;
                    }
                    const quint16 &itemId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    const quint32 &quantity=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16))));
                    destroyObject(itemId,quantity);
                    return;
                }
                break;
                //Put object into a trade
                case 0x0003:
                {
                    QByteArray data(rawData,size);
                    QDataStream in(data);
                    in.setVersion(QDataStream::Qt_4_4);
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
                            if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                            {
                                parseError("wrong remaining size for trade add item id");
                                return;
                            }
                            quint16 item;
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
                    if((in.device()->size()-in.device()->pos())!=0)
                    {
                        parseError(QStringLiteral("remaining data: parsenormalOutput(%1,%2): %3 %4")
                                   .arg(mainCodeType)
                                   .arg(subCodeType)
                                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                                   );
                        return;
                    }
                    return;
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
                    QByteArray data(rawData,size);
                    QDataStream in(data);
                    in.setVersion(QDataStream::Qt_4_4);
                    qint64 cash;
                    QList<QPair<quint16, qint32> > items;
                    QList<quint32> withdrawMonsters;
                    QList<quint32> depositeMonsters;
                    if((data.size()-in.device()->pos())<((int)sizeof(qint64)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> cash;
                    quint16 size16;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size16;
                    quint32 index=0;
                    while(index<size16)
                    {
                        quint16 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
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
                        items << QPair<quint16, qint32>(id,quantity);
                        index++;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for trade add monster");
                        return;
                    }
                    quint32 size;
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
                    if((in.device()->size()-in.device()->pos())!=0)
                    {
                        parseError(QStringLiteral("remaining data: parsenormalOutput(%1,%2): %3 %4")
                                   .arg(mainCodeType)
                                   .arg(subCodeType)
                                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                                   );
                        return;
                    }
                    return;
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
                    if(size!=((int)sizeof(quint32)+sizeof(quint16)))
                    {
                        parseError("wrong remaining size for learn skill");
                        return;
                    }
                    const quint32 &monsterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    const quint16 &skill=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint32))));
                    learnSkill(monsterId,skill);
                    return;
                }
                break;
                //Heal all the monster
                case 0x0006:
                    heal();
                break;
                //Request bot fight
                case 0x0007:
                {
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for request bot fight");
                        return;
                    }
                    const quint16 &fightId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    requestFight(fightId);
                    return;
                }
                break;
                //move the monster
                case 0x0008:
                {
                    if(size!=((int)sizeof(quint8)*2))
                    {
                        parseError("wrong remaining size for move monster");
                        return;
                    }
                    const quint8 &moveWay=*rawData;
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
                    const quint8 &position=*(rawData+sizeof(quint8));
                    moveMonster(moveUp,position);
                    return;
                }
                break;
                //change monster in fight
                case 0x0009:
                {
                    if(size!=((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for monster in fight");
                        return;
                    }
                    const quint32 &monsterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    changeOfMonsterInFight(monsterId);
                    return;
                }
                break;
                /// \todo check double validation
                //Monster evolution validated
                case 0x000A:
                {
                    if(size!=((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for monster evolution validated");
                        return;
                    }
                    const quint32 &monsterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    confirmEvolution(monsterId);
                    return;
                }
                break;
                //Monster evolution validated
                case 0x000B:
                {
                    if(size<((int)sizeof(quint16)+(int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for use object on monster");
                        return;
                    }
                    const quint16 &item=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    const quint32 &monsterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16))));
                    useObjectOnMonster(item,monsterId);
                    return;
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
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for quest start");
                        return;
                    }
                    #endif
                    const quint16 &questId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Start,questId);
                    return;
                }
                break;
                //Quest finish
                case 0x0002:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for quest finish");
                        return;
                    }
                    #endif
                    const quint16 &questId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Finish,questId);
                    return;
                }
                break;
                //Quest cancel
                case 0x0003:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for quest cancel");
                        return;
                    }
                    #endif
                    const quint16 &questId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Cancel,questId);
                    return;
                }
                break;
                //Quest next step
                case 0x0004:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseError("wrong remaining size for quest next step");
                        return;
                    }
                    #endif
                    const quint32 &questId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_NextStep,questId);
                    return;
                }
                break;
                //Waiting for city caputre
                case 0x0005:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint8)))
                    {
                        parseError("wrong remaining size for city capture");
                        return;
                    }
                    #endif
                    const quint8 &cancel=*rawData;
                    if(cancel==0x00)
                        waitingForCityCaputre(false);
                    else
                        waitingForCityCaputre(true);
                    return;
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
}

//have query with reply
void Client::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    if(stopIt)
        return;
    Q_UNUSED(data);
    const bool goodQueryBeforeLoginLoaded=
            mainCodeType==0x03 ||
            mainCodeType==0x04
            ;
    if(account_id==0 || (!character_loaded && goodQueryBeforeLoginLoaded))
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    if(account_id==0)
    {
        parseError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    if(!character_loaded)
    {
        parseError(QStringLiteral("charaters is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    parseError(QStringLiteral("no query with only the main code for now, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
{
    if(stopIt)
        return;
    if(account_id==0)
    {
        parseError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    const bool goodQueryBeforeCharacterLoaded=mainCodeType==0x02 &&
            (subCodeType==0x03 ||
             subCodeType==0x04 ||
             subCodeType==0x0C ||
             subCodeType==0x05
                );
    if(!character_loaded && !goodQueryBeforeCharacterLoaded)
    {
        parseError(QStringLiteral("charaters is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
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
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            //Add character
            case 0x0003:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
                quint8 profileIndex;
                QString pseudo;
                quint8 skinId;
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
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> skinId;
                addCharacter(queryNumber,profileIndex,pseudo,skinId);
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
                return;
            }
            break;
            //Remove character
            case 0x0004:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint32 &characterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                removeCharacter(queryNumber,characterId);
            }
            break;
            //Select character
            case 0x0005:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint32 &characterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                selectCharacter(queryNumber,characterId);
            }
            break;
            //Send datapack file list
            case 0x000C:
            {
                if(!CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
                {
                    errorOutput("Can't use because mirror is defined");
                    return;
                }
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
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
                return;
            }
            break;
            //Clan action
            case 0x000D:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
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
                return;
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
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint8 &plant_id=*rawData;
                plantSeed(queryNumber,plant_id);
                return;
            }
            break;
            //Collect mature plant
            case 0x0007:
                collectPlant(queryNumber);
            break;
            //Usage of recipe
            case 0x0008:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &recipe_id=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                useRecipe(queryNumber,recipe_id);
                return;
            }
            break;
            //Use object
            case 0x0009:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &objectId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                useObject(queryNumber,objectId);
                return;
            }
            break;
            //Get shop list
            case 0x000A:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                getShopList(queryNumber,shopId);
                return;
            }
            break;
            //Buy object
            case 0x000B:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                buyObject(queryNumber,shopId,objectId,quantity,price);
                return;
            }
            break;
            //Sell object
            case 0x000C:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                sellObject(queryNumber,shopId,objectId,quantity,price);
                return;
            }
            break;
            case 0x000D:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                getFactoryList(queryNumber,factoryId);
                return;
            }
            break;
            case 0x000E:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                buyFactoryProduct(queryNumber,factoryId,objectId,quantity,price);
                return;
            }
            break;
            case 0x000F:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                sellFactoryResource(queryNumber,factoryId,objectId,quantity,price);
                return;
            }
            break;
            case 0x0010:
                getMarketList(queryNumber);
                return;
            break;
            case 0x0011:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
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
                return;
            }
            break;
            case 0x0012:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint16 objectId;
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
                return;
            }
            break;
            case 0x0013:
                recoverMarketCash(queryNumber);
                return;
            break;
            case 0x0014:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
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
                return;
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
}

//send reply
void Client::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(account_id==0)
    {
        disconnectClient();
        return;
    }
    if(!character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    parseError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(account_id==0)
    {
        disconnectClient();
        return;
    }
    if(!character_loaded)
    {
        parseError(QStringLiteral("is not logged, parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    if(stopIt)
        return;
    /*bugif(!character_loaded)
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }*/
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
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
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("wrong size with the full reply data main ident: %1, sub ident: %2, data: %3").arg(mainCodeType).arg(subCodeType).arg(QString(QByteArray(data,size).toHex())));
                        return;
                    }
                    #endif
                    const quint8 &returnCode=*data;
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
                    return;
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
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("wrong size with the full reply data main ident: %1, sub ident: %2, data: %3").arg(mainCodeType).arg(subCodeType).arg(QString(QByteArray(data,size).toHex())));
                        return;
                    }
                    #endif
                    const quint8 &returnCode=*data;
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
                    return;
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
    parseIncommingData();
}
