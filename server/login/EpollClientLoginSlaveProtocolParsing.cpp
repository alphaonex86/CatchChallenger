#include "EpollClientLoginSlave.h"
#include "../base/BaseServerLogin.h"
#include "../../general/base/CommonSettingsCommon.h"

#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginSlave::doDDOSComputeAll()
{
    unsigned int index=0;
    while(index<client_list.size())
    {
        client_list.at(index)->doDDOSCompute();
        index++;
    }
}

void EpollClientLoginSlave::doDDOSCompute()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE<0 || CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE>CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        qDebug() << "CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE out of range:" << CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        return;
    }
    #endif
    {
        movePacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
            {
                parseNetworkReadError("index out of range <0, movePacketKick");
                return;
            }
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
            {
                parseNetworkReadError("index out of range >, movePacketKick");
                return;
            }
            if(movePacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITMOVE*2)
            {
                parseNetworkReadError(QString("index out of range in array for index %1, movePacketKick").arg(movePacketKick[index]));
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
        if(movePacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITMOVE*2)
        {
            parseNetworkReadError("bug in DDOS calculation count");
            return;
        }
        #endif
        movePacketKickNewValue=0;
    }
    {
        chatPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, chatPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, chatPacketKick";
            if(chatPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
                qDebug() << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick";
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
        chatPacketKickNewValue=0;
    }
    {
        otherPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, otherPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, otherPacketKick";
            if(otherPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
                qDebug() << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick";
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
        otherPacketKickNewValue=0;
    }
}

void EpollClientLoginSlave::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x03:
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                replyOutputSize.remove(queryNumber);
                //if lot of un logged connection, remove the first
                if(BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
                {
                    EpollClientLoginSlave *client=static_cast<EpollClientLoginSlave *>(BaseServerLogin::tokenForAuth[0].client);
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
                BaseServerLogin::TokenLink *token=&BaseServerLogin::tokenForAuth[BaseServerLogin::tokenForAuthSize];
                {
                    token->client=this;
                    if(BaseServerLogin::fpRandomFile==NULL)
                    {
                        //insercure implementation
                        abort();
                        int index=0;
                        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            token->value[index]=rand()%256;
                            index++;
                        }
                    }
                    else
                    {
                        const int &size=fread(token->value,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,BaseServerLogin::fpRandomFile);
                        if(size!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            parseNetworkReadError(
                                        QStringLiteral("Not correct number of byte to generate the token: size!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT: %1!=%2")
                                        .arg(size)
                                        .arg(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                                                  );
                            return;
                        }
                    }
                }
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(ProtocolParsing::compressionTypeServer)
                {
                    case ProtocolParsing::CompressionType::None:
                        *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                    break;
                    case ProtocolParsing::CompressionType::Zlib:
                        *(EpollClientLoginSlave::protocolReplyCompresssionZlib+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompresssionZlib+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompresssionZlib),sizeof(EpollClientLoginSlave::protocolReplyCompresssionZlib));
                    break;
                    case ProtocolParsing::CompressionType::Xz:
                        *(EpollClientLoginSlave::protocolReplyCompressionXz+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionXz+4,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionXz),sizeof(EpollClientLoginSlave::protocolReplyCompressionXz));
                    break;
                    default:
                        parseNetworkReadError("Compression selected wrong");
                    return;
                }
                #else
                *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                #endif
                BaseServerLogin::tokenForAuthSize++;
                stat=EpollClientLoginStat::ProtocolGood;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(EpollClientLoginSlave::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginSlave::protocolReplyProtocolNotSupported));*/
                parseNetworkReadError("Wrong protocol");
                return;
            }
        break;
        //login
        case 0x04:
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                replyOutputSize.remove(queryNumber);
                *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                parseNetworkReadError("Loggin already in progress");
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                parseNetworkReadError("send login before the protocol");
                return;
            }
            else
            {
                stat=EpollClientLoginStat::LoginInProgress;
                askLogin(queryNumber,data);
                return;
            }
        break;
        //create account
        case 0x05:
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                #endif
                replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                parseNetworkReadError("Loggin already in progress");
                return;
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                parseNetworkReadError("send login before the protocol");
                return;
            }
            else
            {
                if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
                {
                    stat=EpollClientLoginStat::LoginInProgress;
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
                    parseNetworkReadError("Account creation not premited");
                    return;
                }
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void EpollClientLoginSlave::parseMessage(const quint8 &mainCodeType,const char *data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
        return;
    }
    switch(mainCodeType)
    {
        case 0x40:
            if((movePacketKickTotalCache+movePacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many move in sort time, check DDOS limit");
                return;
            }
            movePacketKickNewValue++;
        break;
        case 0x43:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many chat in sort time, check DDOS limit");
                return;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
            {
                parseNetworkReadError("Too many packet in sort time, check DDOS limit");
                return;
            }
            otherPacketKickNewValue++;
        break;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->packOutcommingData(mainCodeType,data,size);
            return;
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
    }
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginSlave::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char *rawData,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
        return;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->packFullOutcommingData(mainCodeType,subCodeType,rawData,size);
            return;
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
    }
    (void)rawData;
    (void)size;
    (void)subCodeType;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void EpollClientLoginSlave::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    Q_UNUSED(data);
    if(stat!=EpollClientLoginStat::Logged)
    {
        if(stat==EpollClientLoginStat::GameServerConnecting)
        {
            parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
            return;
        }
        if(stat==EpollClientLoginStat::GameServerConnected)
        {
            if(Q_LIKELY(linkToGameServer))
                linkToGameServer->packOutcommingQuery(mainCodeType,queryNumber,data,size);
            else
                parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginSlave::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
        return;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->packFullOutcommingQuery(mainCodeType,subCodeType,queryNumber,rawData,size);
            return;
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
    }
    if(account_id==0)
    {
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    if(stat!=Logged)
    {
        parseNetworkReadError("client in wrong state main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType)+", for parseFullQuery");
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            //Add character
            case 0x03:
            {
                int cursor=0;
                quint8 charactersGroupIndex;
                quint8 profileIndex;
                QString pseudo;
                quint8 skinId;
                quint8 pseudoSize;
                {
                    if((size-cursor)<(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                        return;
                    }
                    charactersGroupIndex=rawData[cursor];
                    cursor+=1;
                }
                {
                    if((size-cursor)<(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                        return;
                    }
                    profileIndex=rawData[cursor];
                    cursor+=1;
                }
                {
                    if((size-cursor)<(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                        return;
                    }
                    pseudoSize=rawData[cursor];
                    cursor+=1;
                    if((size-cursor)<(int)pseudoSize)
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                        return;
                    }
                    pseudo=QString::fromUtf8(rawData+cursor,pseudoSize);
                    cursor+=pseudoSize;
                }
                {
                    if((size-cursor)<(int)sizeof(quint8))
                    {
                        parseNetworkReadError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                        return;
                    }
                    skinId=rawData[cursor];
                    cursor+=1;
                }
                addCharacter(queryNumber,charactersGroupIndex,profileIndex,pseudo,skinId);
                if((size-cursor)!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3)")
                               .arg(mainCodeType)
                               .arg(subCodeType)
                               .arg(queryNumber)
                               );
                    return;
                }
                return;
            }
            break;
            //Remove character
            case 0x04:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=((int)sizeof(quint32)+(int)sizeof(quint8)))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint8 &charactersGroupIndex=rawData[0];
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                removeCharacter(queryNumber,charactersGroupIndex,characterId);
            }
            break;
            //Select character
            case 0x05:
            {
                const quint8 &charactersGroupIndex=rawData[0];
                const quint32 &serverUniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+5)));
                selectCharacter(queryNumber,serverUniqueKey,charactersGroupIndex,characterId);
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
void EpollClientLoginSlave::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
        return;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->postReplyData(queryNumber,data,size);
            return;
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
    }
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void EpollClientLoginSlave::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+QString::number(mainCodeType));
        return;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return;
    }
    otherPacketKickNewValue++;
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->postReplyData(queryNumber,data,size);
            return;
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+QString::number(mainCodeType));
            return;
        }
    }
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void EpollClientLoginSlave::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
