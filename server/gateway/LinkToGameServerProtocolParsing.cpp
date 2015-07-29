#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

void LinkToGameServer::parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size)
{
    Q_UNUSED(queryNumber);
    Q_UNUSED(size);
    Q_UNUSED(data);
    switch(mainCodeType)
    {
        case 0x03:
        {
            //Protocol initialization
            if(size<1)
            {
                parseNetworkReadError(QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                return;
            }
            quint8 returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                rewrite to send the ouput compression
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x05:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zlib;
                    break;
                    case 0x06:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Xz;
                    break;
                    default:
                        parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                if(size!=(sizeof(quint8)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    parseNetworkReadError(QStringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                //send token to game server
                if(client!=NULL)
                    client->postReply(queryNumber,data,size);
                stat=ProtocolGood;
                return;
            }
            else
            {
                if(returnCode==0x02)
                    parseNetworkReadError("Protocol not supported");
                else if(returnCode==0x03)
                    parseNetworkReadError("Server full");
                else
                    parseNetworkReadError(QStringLiteral("Unknown error %1").arg(returnCode));
                return;
            }
        }
        break;
        default:
            parseNetworkReadError(QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
}

void LinkToGameServer::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::ProtocolGood)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType));
        return;
    }
    (void)data;
    (void)size;
    if(client!=NULL)
        client->packOutcommingData(mainCodeType,data,size);
}

void LinkToGameServer::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const rawData,const unsigned int &size)
{
    if(stat!=Stat::ProtocolGood)
    {
        if(mainCodeType==0xC2 && subCodeType==0x0F)//send Logical group
        {}
        else if(mainCodeType==0xC2 && subCodeType==0x0E)//send Send server list to real player
        {}
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
            return;
        }
    }
    (void)rawData;
    (void)size;
    if(client!=NULL)
        client->packFullOutcommingData(mainCodeType,subCodeType,rawData,size);
}

//have query with reply
void LinkToGameServer::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::ProtocolGood)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    if(client!=NULL)
        client->packOutcommingQuery(mainCodeType,queryNumber,data,size);
}

void LinkToGameServer::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    //do the work here
    if(client!=NULL)
        client->packFullOutcommingQuery(mainCodeType,subCodeType,queryNumber,rawData,size);
}

//send reply
void LinkToGameServer::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(mainCodeType==0x03 && stat==Stat::Connected)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here

    /* intercept part here */
    if(mainCodeType==0x04)
    {
        if(size>14 && data[0x00]==0x01)//all is good, change the reply
        {
            unsigned int pos=14;
            if(reply04inWait!=NULL)
            {
                delete reply04inWait;
                reply04inWait=NULL;
                parseNetworkReadError("another reply04inWait in suspend");
                return;
            }
            unsigned int remainingSize=size-14-1-data[pos];
            pos+=data[pos];
            unsigned int reply04inWaitSize=14+LinkToGameServer::httpDatapackMirrorRewriteBase.size()+remainingSize;
            reply04inWait=new char[reply04inWaitSize];
            memcpy(reply04inWait,data,14);
            memcpy(reply04inWait,LinkToGameServer::httpDatapackMirrorRewriteBase.constData(),LinkToGameServer::httpDatapackMirrorRewriteBase.size());
            memcpy(reply04inWait,data+pos,remainingSize);

            if()//checksum never done
            {
            }
            else if()//need download the datapack content
            {
            }
            else
                client->postReply(queryNumber,reply04inWait,reply04inWaitSize);
            return;
        }
    }

    if(client!=NULL)
        client->postReply(queryNumber,data,size);
}

void LinkToGameServer::parseFullReplyData(const quint8 &mainCodeType, const quint8 &subCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size)
{
    (void)mainCodeType;
    (void)subCodeType;
    (void)data;
    (void)size;
    //do the work here
    if(client!=NULL)
        client->postReply(queryNumber,data,size);
}

void LinkToGameServer::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
