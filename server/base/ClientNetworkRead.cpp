#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "../../general/base/CommonSettingsCommon.h"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.h"
#endif

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
    sendQuery(0x79,0x02,queryNumberList.first(),data.constData(),data.size());
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
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    out << (COORD_TYPE)x;
    out << (COORD_TYPE)y;
    out << (quint8)orientation;
    sendQuery(0x79,0x01,queryNumberList.first(),outputData.constData(),outputData.size());
    queryNumberList.removeFirst();
}

void Client::sendTradeRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    sendQuery(0x80,0x01,queryNumberList.first(),data.constData(),data.size());
    queryNumberList.removeFirst();
}

void Client::sendBattleRequest(const QByteArray &data)
{
    if(queryNumberList.empty())
    {
        errorOutput(QStringLiteral("Sorry, no free query number to send this query of trade"));
        return;
    }
    sendQuery(0x90,0x01,queryNumberList.first(),data.constData(),data.size());
    queryNumberList.removeFirst();
}

void Client::parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size)
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
            if(memcmp(data,Client::protocolHeaderToMatch,sizeof(Client::protocolHeaderToMatch))==0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                replyOutputSize.remove(queryNumber);
                if(GlobalServerData::serverPrivateVariables.connected_players>=GlobalServerData::serverSettings.max_players)
                {
                    *(Client::protocolReplyServerFull+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyServerFull),sizeof(Client::protocolReplyServerFull));
                    disconnectClient();
                    //errorOutput(Client::text_server_full);DDOS -> full the log
                    return;
                }
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                //if lot of un logged connection, remove the first
                if(BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
                {
                    Client *client=static_cast<Client *>(BaseServerLogin::tokenForAuth[0].client);
                    client->disconnectClient();
                    delete client;
                    BaseServerLogin::tokenForAuthSize--;
                    if(BaseServerLogin::tokenForAuthSize>0)
                    {
                        quint32 index=0;
                        while(index<BaseServerLogin::tokenForAuthSize)
                        {
                            BaseServerLogin::tokenForAuth[index]=BaseServerLogin::tokenForAuth[index+1];
                            index++;
                        }
                        //don't work:memmove(BaseServerLogin::tokenForAuth,BaseServerLogin::tokenForAuth+sizeof(TokenLink),BaseServerLogin::tokenForAuthSize*sizeof(TokenLink));
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(BaseServerLogin::tokenForAuth[0].client==NULL)
                            abort();
                        #endif
                    }
                    return;
                }
                #endif
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                BaseServerLogin::TokenLink *token=&BaseServerLogin::tokenForAuth[BaseServerLogin::tokenForAuthSize];
                {
                    token->client=this;
                    #ifdef Q_OS_LINUX
                    if(BaseServerLogin::fpRandomFile==NULL)
                    {
                        //failback
                        /* allow poor quality number:
                         * 1) more easy to run, allow start include if /dev/urandom can't be read
                         * 2) it's for very small server (Lan) or internal communication */
                        #if ! defined(CATCHCHALLENGER_CLIENT) && ! defined(CATCHCHALLENGER_SOLO)
                        /// \warning total insecure implementation
                        abort();
                        #endif
                        int index=0;
                        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            token->value[index]=rand()%256;
                            index++;
                        }
                    }
                    else
                    {
                        const qint32 readedByte=fread(token->value,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,BaseServerLogin::fpRandomFile);
                        if(readedByte!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            /// \todo check why client not disconnected if pass here
                            errorOutput(
                                        QStringLiteral("Not correct number of byte to generate the token readedByte!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT: %1!=%2, errno: %3")
                                        .arg(readedByte)
                                        .arg(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                                        .arg(errno)
                                        );
                            return;
                        }
                    }
                    #else
                    /// \warning total insecure implementation
                    // not abort(); because under not linux will not work
                    int index=0;
                    while(index<CATCHCHALLENGER_TOKENSIZE)
                    {
                        token->value[index]=rand()%256;
                        index++;
                    }
                    #endif
                }
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(ProtocolParsing::compressionTypeServer)
                {
                    case CompressionType_None:
                        *(Client::protocolReplyCompressionNone+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompressionNone+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                    break;
                    case CompressionType_Zlib:
                        *(Client::protocolReplyCompresssionZlib+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompresssionZlib+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompresssionZlib),sizeof(Client::protocolReplyCompresssionZlib));
                    break;
                    case CompressionType_Xz:
                        *(Client::protocolReplyCompressionXz+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompressionXz+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionXz),sizeof(Client::protocolReplyCompressionXz));
                    break;
                    default:
                        errorOutput("Compression selected wrong");
                    return;
                }
                #else
                *(Client::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(Client::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                #endif
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                BaseServerLogin::tokenForAuthSize++;
                #endif
                have_send_protocol=true;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(Client::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyProtocolNotSupported),sizeof(Client::protocolReplyProtocolNotSupported));*/
                errorOutput("Wrong protocol");
                return;
            }
        break;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case 0x04:
            if(!have_send_protocol)
            {
                errorOutput("send login before the protocol");
                return;
            }
            if(is_logging_in_progess)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(queryNumber);
                #endif
                //replyOutputSize.remove(queryNumber);
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
                //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                #endif
                //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                errorOutput("Loggin already in progress at create account");
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
                    //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                    #endif
                    //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                    //not reply to prevent DDOS attack
                    errorOutput("Account creation not premited");
                }
            }
        break;
        #endif
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void Client::parseMessage(const quint8 &mainCodeType,const char * const data,const int unsigned &size)
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
                parseNetworkReadError("Wrong size in move packet");
                return;
            }
            #endif
            direction=*(data+sizeof(quint8));
            if(direction<1 || direction>8)
            {
                parseNetworkReadError(QStringLiteral("Bad direction number: %1").arg(direction));
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
            in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitChat)
            {
                errorOutput("Too many chat in sort time, check DDOS limit");
                return;
            }
            chatPacketKickNewValue++;
            if(size<((int)sizeof(quint8)))
            {
                parseNetworkReadError("wrong remaining size for chat");
                return;
            }
            quint8 chatType;
            in >> chatType;
            if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
            {
                parseNetworkReadError("chat type error: "+QString::number(chatType));
                return;
            }
            if(chatType==Chat_type_pm)
            {
                if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
                {
                    QString text;
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseNetworkReadError("wrong utf8 to QString size in PM for text size");
                            return;
                        }
                        quint8 textSize;
                        in >> textSize;
                        if(textSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                            {
                                parseNetworkReadError("wrong utf8 to QString size in PM for text");
                                return;
                            }
                            const QByteArray &rawText=newData.mid(in.device()->pos(),textSize);
                            text=QString::fromUtf8(rawText.data(),rawText.size());
                            in.device()->seek(in.device()->pos()+rawText.size());
                        }
                    }
                    QString pseudo;
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseNetworkReadError("wrong utf8 to QString size in PM for pseudo");
                            return;
                        }
                        quint8 pseudoSize;
                        in >> pseudoSize;
                        if(pseudoSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                            {
                                parseNetworkReadError("wrong utf8 to QString size in PM for pseudo");
                                return;
                            }
                            const QByteArray &rawText=newData.mid(in.device()->pos(),pseudoSize);
                            pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                            in.device()->seek(in.device()->pos()+rawText.size());
                        }
                    }

                    normalOutput(Client::text_slashpmspace+pseudo+Client::text_space+text);
                    sendPM(text,pseudo);
                }
                else
                {
                    parseNetworkReadError("can't send pm because is disabled: "+QString::number(chatType));
                    return;
                }
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError("wrong utf8 to QString header size");
                    return;
                }
                QString text;
                quint8 textSize;
                in >> textSize;
                if(textSize>0)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                    {
                        parseNetworkReadError("wrong utf8 to QString size");
                        return;
                    }
                    const QByteArray &rawText=newData.mid(in.device()->pos(),textSize);
                    text=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                }

                if(!text.startsWith(Client::text_slash))
                {
                    if(chatType==Chat_type_local)
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
                            sendLocalChatText(text);
                        else
                        {
                            parseNetworkReadError("can't send chat local because is disabled: "+QString::number(chatType));
                            return;
                        }
                    }
                    else
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_clan || CommonSettingsServer::commonSettingsServer.chat_allow_all)
                            sendChatText((Chat_type)chatType,text);
                        else
                        {
                            parseNetworkReadError("can't send chat other because is disabled: "+QString::number(chatType));
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
                parseNetworkReadError("Wrong size in move packet");
                return;
            }
            #endif
            if(size!=(int)sizeof(quint16))
            {
                parseNetworkReadError("Wrong size in move packet");
                return;
            }
            useSkill(le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data))));
            return;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void Client::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const rawData,const unsigned int &size)
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
    if(mainCodeType!=0x42 && subCodeType!=0x03)
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
            case 0x04:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=((int)sizeof(quint8)))
                {
                    parseNetworkReadError("wrong remaining size for clan invite");
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
                        parseNetworkReadError(QStringLiteral("wrong return code for clan invite ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                }
                return;
            }
            break;
            default:
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        //inventory
        case 0x50:
            switch(subCodeType)
            {
                //Destroy an object
                case 0x02:
                {
                    if(size!=((int)sizeof(quint16)+sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for destroy item id");
                        return;
                    }
                    const quint16 &itemId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    const quint32 &quantity=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16))));
                    destroyObject(itemId,quantity);
                    return;
                }
                break;
                //Put object into a trade
                case 0x03:
                {
                    QByteArray data(rawData,size);
                    QDataStream in(data);
                    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                    if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                    {
                        parseNetworkReadError("wrong remaining size for trade add type");
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
                                parseNetworkReadError("wrong remaining size for trade add cash");
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
                                parseNetworkReadError("wrong remaining size for trade add item id");
                                return;
                            }
                            quint16 item;
                            in >> item;
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseNetworkReadError("wrong remaining size for trade add item quantity");
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
                                parseNetworkReadError("wrong remaining size for trade add monster");
                                return;
                            }
                            quint32 monsterId;
                            in >> monsterId;
                            tradeAddTradeMonster(monsterId);
                        }
                        break;
                        default:
                            parseNetworkReadError("wrong type for trade add");
                            return;
                        break;
                    }
                    if((in.device()->size()-in.device()->pos())!=0)
                    {
                        parseNetworkReadError(QStringLiteral("remaining data: parsenormalOutput(%1,%2): %3 %4")
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
                case 0x04:
                    tradeFinished();
                break;
                //trade canceled after the accept
                case 0x05:
                    tradeCanceled();
                break;
                //deposite/withdraw to the warehouse
                case 0x06:
                {
                    QByteArray data(rawData,size);
                    QDataStream in(data);
                    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                    qint64 cash;
                    QList<QPair<quint16, qint32> > items;
                    QList<quint32> withdrawMonsters;
                    QList<quint32> depositeMonsters;
                    if((data.size()-in.device()->pos())<((int)sizeof(qint64)))
                    {
                        parseNetworkReadError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> cash;
                    quint16 size16;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size16;
                    quint32 index=0;
                    while(index<size16)
                    {
                        quint16 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                        {
                            parseNetworkReadError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        qint32 quantity;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseNetworkReadError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> quantity;
                        items << QPair<quint16, qint32>(id,quantity);
                        index++;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for trade add monster");
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
                            parseNetworkReadError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        withdrawMonsters << id;
                        index++;
                    }
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for trade add monster");
                        return;
                    }
                    in >> size;
                    index=0;
                    while(index<size)
                    {
                        quint32 id;
                        if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                        {
                            parseNetworkReadError("wrong remaining size for trade add monster");
                            return;
                        }
                        in >> id;
                        depositeMonsters << id;
                        index++;
                    }
                    wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
                    if((in.device()->size()-in.device()->pos())!=0)
                    {
                        parseNetworkReadError(QStringLiteral("remaining data: parsenormalOutput(%1,%2): %3 %4")
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
                case 0x07:
                    takeAnObjectOnMap();
                break;
                default:
                    parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        //battle
        case 0x60:
            switch(subCodeType)
            {
                //Try escape
                case 0x02:
                    tryEscape();
                break;
                //Learn skill
                case 0x04:
                {
                    if(size!=((int)sizeof(quint32)+sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for learn skill");
                        return;
                    }
                    const quint32 &monsterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    const quint16 &skill=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint32))));
                    learnSkill(monsterId,skill);
                    return;
                }
                break;
                //Heal all the monster
                case 0x06:
                    heal();
                break;
                //Request bot fight
                case 0x07:
                {
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for request bot fight");
                        return;
                    }
                    const quint16 &fightId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    requestFight(fightId);
                    return;
                }
                break;
                //move the monster
                case 0x08:
                {
                    if(size!=((int)sizeof(quint8)*2))
                    {
                        parseNetworkReadError("wrong remaining size for move monster");
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
                            parseNetworkReadError("wrong move up value");
                        return;
                    }
                    const quint8 &position=*(rawData+sizeof(quint8));
                    moveMonster(moveUp,position);
                    return;
                }
                break;
                //change monster in fight, monster id in db
                case 0x09:
                {
                    if(size!=((int)sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for monster in fight");
                        return;
                    }
                    const quint32 &monsterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    changeOfMonsterInFight(monsterId);
                    return;
                }
                break;
                /// \todo check double validation
                //Monster evolution validated
                case 0x0A:
                {
                    if(size!=((int)sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for monster evolution validated");
                        return;
                    }
                    const quint32 &monsterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                    confirmEvolution(monsterId);
                    return;
                }
                break;
                //Use object on monster
                case 0x0B:
                {
                    if(size<((int)sizeof(quint16)+(int)sizeof(quint32)))
                    {
                        parseNetworkReadError("wrong remaining size for use object on monster");
                        return;
                    }
                    const quint16 &item=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    const quint32 &monsterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16))));
                    useObjectOnMonster(item,monsterId);
                    return;
                }
                break;
                default:
                    parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        //quest
        case 0x6a:
            switch(subCodeType)
            {
                //Quest start
                case 0x01:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for quest start");
                        return;
                    }
                    #endif
                    const quint16 &questId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Start,questId);
                    return;
                }
                break;
                //Quest finish
                case 0x02:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for quest finish");
                        return;
                    }
                    #endif
                    const quint16 &questId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Finish,questId);
                    return;
                }
                break;
                //Quest cancel
                case 0x03:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for quest cancel");
                        return;
                    }
                    #endif
                    const quint16 &questId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_Cancel,questId);
                    return;
                }
                break;
                //Quest next step
                case 0x04:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint16)))
                    {
                        parseNetworkReadError("wrong remaining size for quest next step");
                        return;
                    }
                    #endif
                    const quint32 &questId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                    newQuestAction(QuestAction_NextStep,questId);
                    return;
                }
                break;
                //Waiting for city caputre
                case 0x05:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=((int)sizeof(quint8)))
                    {
                        parseNetworkReadError("wrong remaining size for city capture");
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
                    parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void Client::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
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
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    if(!character_loaded)
    {
        parseNetworkReadError(QStringLiteral("charaters is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    parseNetworkReadError(QStringLiteral("no query with only the main code for now, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *rawData,const unsigned int &size)
{
    if(stopIt)
        return;
    if(account_id==0
            #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            &&
            !(mainCodeType==0x02 && subCodeType==0x06)
            #endif
            )
    {
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    const bool goodQueryBeforeCharacterLoaded=mainCodeType==0x02 &&
            (
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                subCodeType==0x03 ||
                subCodeType==0x04 ||
                subCodeType==0x05 ||
                #else
                subCodeType==0x06 ||
                #endif
                subCodeType==0x0C
                );
    if(!character_loaded && !goodQueryBeforeCharacterLoaded)
    {
        parseNetworkReadError(QStringLiteral("charaters is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
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
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            //Add character
            case 0x03:
            {
                if(character_loaded)
                {
                    parseNetworkReadError(QStringLiteral("charaters is logged, deny charaters add/select/delete, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                quint8 profileIndex;
                QString pseudo;
                quint8 skinId;
                quint8 charactersGroupIndex;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> charactersGroupIndex;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> profileIndex;
                //pseudo
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseNetworkReadError("wrong utf8 to QString size in PM for text size");
                        return;
                    }
                    quint8 textSize;
                    in >> textSize;
                    if(textSize>0)
                    {
                        if(textSize>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
                        {
                            parseNetworkReadError(QStringLiteral("pseudo size is too big: %1 because is greater than %2").arg(pseudo.size()).arg(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
                            return;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                        {
                            parseNetworkReadError("wrong utf8 to QString size in PM for text");
                            return;
                        }
                        const QByteArray &rawText=data.mid(in.device()->pos(),textSize);
                        pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                    }
                }
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> skinId;
                addCharacter(queryNumber,profileIndex,pseudo,skinId);
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
            case 0x04:
            {
                if(character_loaded)
                {
                    parseNetworkReadError(QStringLiteral("charaters is logged, deny charaters add/select/delete, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint8)+sizeof(quint32))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                //skip charactersGroupIndex with rawData+1
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                removeCharacterLater(queryNumber,characterId);
            }
            break;
            //Select character
            case 0x05:
            {
                if(character_loaded)
                {
                    parseNetworkReadError(QStringLiteral("charaters is logged, deny charaters add/select/delete, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint8)+sizeof(quint32)+sizeof(quint32))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                //skip charactersGroupIndex with rawData+4+1
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1+4)));
                selectCharacter(queryNumber,characterId);
            }
            break;
            #else
            //Select character on game server
            case 0x06:
            {
                if(character_loaded)
                {
                    parseNetworkReadError(QStringLiteral("charaters is logged, deny charaters add/select/delete, parseQuery(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    return;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, subCodeType: %2, data: %3").arg(mainCodeType).arg(subCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                selectCharacter(queryNumber,rawData);
            }
            break;
            #endif
            //Send datapack file list
            case 0x0C:
            {
                if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.isEmpty())
                {
                    errorOutput("Can't use because mirror is defined");
                    return;
                }
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 number_of_file;
                in >> number_of_file;
                QStringList files;
                QList<quint32> partialHashList;
                QString tempFileName;
                quint32 partialHash;
                quint32 index=0;
                while(index<number_of_file)
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseNetworkReadError(QStringLiteral("error at datapack file list query"));
                        return;
                    }
                    in >> tempFileName;
                    files << tempFileName;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size for id with main ident: %1, subIdent: %2, remaining: %3, lower than: %4")
                            .arg(mainCodeType)
                            .arg(subCodeType)
                            .arg(in.device()->size()-in.device()->pos())
                            .arg((int)sizeof(quint32))
                            );
                        return;
                    }
                    in >> partialHash;
                    partialHashList << partialHash;
                    index++;
                }
                datapackList(queryNumber,files,partialHashList);
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
            case 0x0D:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                            parseNetworkReadError(QStringLiteral("error at datapack file list query"));
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
                    parseNetworkReadError(QStringLiteral("unknown clan action code"));
                    return;
                }
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x10:
        switch(subCodeType)
        {
            //Use seed into dirt
            case 0x06:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint8 &plant_id=*rawData;
                plantSeed(queryNumber,plant_id);
                return;
            }
            break;
            //Collect mature plant
            case 0x07:
                collectPlant(queryNumber);
            break;
            //Usage of recipe
            case 0x08:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &recipe_id=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                useRecipe(queryNumber,recipe_id);
                return;
            }
            break;
            //Use object
            case 0x09:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &objectId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                useObject(queryNumber,objectId);
                return;
            }
            break;
            //Get shop list
            case 0x0A:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                getShopList(queryNumber,shopId);
                return;
            }
            break;
            //Buy object
            case 0x0B:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                buyObject(queryNumber,shopId,objectId,quantity,price);
                return;
            }
            break;
            //Sell object
            case 0x0C:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &shopId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                sellObject(queryNumber,shopId,objectId,quantity,price);
                return;
            }
            break;
            case 0x0D:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                getFactoryList(queryNumber,factoryId);
                return;
            }
            break;
            case 0x0E:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                buyFactoryProduct(queryNumber,factoryId,objectId,quantity,price);
                return;
            }
            break;
            case 0x0F:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint16)*2+sizeof(quint32)*2)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint16 &factoryId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                const quint16 &objectId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+sizeof(quint16))));
                const quint32 &quantity=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2)));
                const quint32 &price=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+sizeof(quint16)*2+sizeof(quint32))));
                sellFactoryResource(queryNumber,factoryId,objectId,quantity,price);
                return;
            }
            break;
            case 0x10:
                getMarketList(queryNumber);
                return;
            break;
            case 0x11:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                        parseNetworkReadError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 marketObjectId;
                    in >> marketObjectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    buyMarketMonster(queryNumber,monsterId);
                }
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
            case 0x12:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                        parseNetworkReadError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint16 objectId;
                    in >> objectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 price;
                    in >> price;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(double))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    putMarketObject(queryNumber,objectId,quantity,price);
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 price;
                    in >> price;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(double))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    putMarketMonster(queryNumber,monsterId,price);
                }
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
            case 0x13:
                recoverMarketCash(queryNumber);
                return;
            break;
            case 0x14:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                        parseNetworkReadError(QStringLiteral("market return type with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                if(queryType==0x01)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 objectId;
                    in >> objectId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
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
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                        return;
                    }
                    quint32 monsterId;
                    in >> monsterId;
                    withdrawMarketMonster(queryNumber,monsterId);
                }
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
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
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void Client::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
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
        parseNetworkReadError(QStringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void Client::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
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
        parseNetworkReadError(QStringLiteral("is not logged, parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
            case 0x01:
                normalOutput("teleportValidatedTo() from protocol");
                teleportValidatedTo(lastTeleportation.first().map,lastTeleportation.first().x,lastTeleportation.first().y,lastTeleportation.first().orientation);
                lastTeleportation.removeFirst();
            break;
            //Event change
            case 0x02:
                removeFirstEventInQueue();
            break;
            default:
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x80:
        switch(subCodeType)
        {
            //Another player request a trade
            case 0x01:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the full reply data main ident: %1, sub ident: %2, data: %3").arg(mainCodeType).arg(subCodeType).arg(QString(QByteArray(data,size).toHex())));
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
                            parseNetworkReadError(QStringLiteral("ident: %1, sub ident: %2, unknown return code: %3").arg(mainCodeType).arg(subCodeType).arg(returnCode));
                        break;
                    }
                    return;
                }
            break;
            default:
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x90:
        switch(subCodeType)
        {
            //Another player request a trade
            case 0x01:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the full reply data main ident: %1, sub ident: %2, data: %3").arg(mainCodeType).arg(subCodeType).arg(QString(QByteArray(data,size).toHex())));
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
                            parseNetworkReadError(QStringLiteral("ident: %1, sub ident: %2, unknown return code: %3").arg(mainCodeType).arg(subCodeType).arg(returnCode));
                        break;
                    }
                    return;
                }
            break;
            default:
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void Client::parseNetworkReadError(const QString &errorString)
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
