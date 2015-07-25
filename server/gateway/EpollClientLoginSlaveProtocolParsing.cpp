#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
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

                stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                /// \todo do the async connect
                /// linkToGameServer->stat=Stat::Connecting;
                const int &socketFd=LinkToGameServer::tryConnect(EpollServerLoginSlave::epollServerLoginSlave->destination_server_ip,EpollServerLoginSlave::epollServerLoginSlave->destination_server_port,5,1);
                if(Q_LIKELY(socketFd>=0))
                {
                    stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                    linkToGameServer=linkToGameServer;
                    linkToGameServer->stat=LinkToGameServer::Stat::Connected;
                    linkToGameServer->client=this;
                    linkToGameServer->protocolQueryNumber=queryNumber;
                    //send the protocol
                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                    linkToGameServer->setConnexionSettings();
                    linkToGameServer->parseIncommingData();
                }
                else
                {
                    parseNetworkReadError(QStringLiteral("not able to connect on the game server as gateway, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
                    return;
                }

                stat=EpollClientLoginStat::ProtocolGood;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                parseNetworkReadError("Wrong protocol");
                return;
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
    if(stat!=Logged)
    {
        parseNetworkReadError("client in wrong state main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType)+", for parseFullQuery");
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
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
