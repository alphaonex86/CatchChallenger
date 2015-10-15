#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

bool LinkToGameServer::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
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
                parseNetworkReadError(std::stringLiteral("wrong size with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                return;
            }
            uint8_t returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
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
                    case 0x07:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Lz4;
                    break;
                    default:
                        parseNetworkReadError(std::stringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                if(size!=(sizeof(uint8_t)))
                {
                    parseNetworkReadError(std::stringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                //send token to game server
                packFullOutcommingQuery(0x02,0x06,queryIdToReconnect/*query number*/,tokenForGameServer,sizeof(tokenForGameServer));
                stat=ProtocolGood;
                return;
            }
            else
            {
                if(client!=NULL)
                    client->postReply(queryIdToReconnect,data,size);
                if(returnCode==0x03)
                    parseNetworkReadError("Server full");
                else
                    parseNetworkReadError(std::stringLiteral("Unknown error %1").arg(returnCode));
                return;
            }
        }
        break;
        default:
            parseNetworkReadError(std::stringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(std::stringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
}

void LinkToGameServer::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType));
        return;
    }
    (void)data;
    (void)size;
    if(client!=NULL)
        client->packOutcommingData(mainCodeType,data,size);
}

void LinkToGameServer::parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char *rawData,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType)+" "+std::to_string(subCodeType));
        return;
    }
    (void)rawData;
    (void)size;
    if(client!=NULL)
        client->packFullOutcommingData(mainCodeType,subCodeType,rawData,size);
}

//have query with reply
void LinkToGameServer::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::Logged)
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    if(client!=NULL)
        client->packOutcommingQuery(mainCodeType,queryNumber,data,size);
}

void LinkToGameServer::parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError(std::stringLiteral("is not logged, parseFullQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    if(client!=NULL)
        client->packFullOutcommingQuery(mainCodeType,subCodeType,queryNumber,rawData,size);
}

//send reply
void LinkToGameServer::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        if(mainCodeType==0x03 && queryNumber==0x01 && stat==Stat::Connected)
        {
            parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
            return;
        }
        else
        {
            parseNetworkReadError(std::stringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
            return;
        }
    }
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here

    if(client!=NULL)
        client->postReply(queryNumber,data,size);
}

void LinkToGameServer::parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::ProtocolGood && mainCodeType==0x02 && subCodeType==0x06)
        {}
        else
        {
            parseNetworkReadError(std::stringLiteral("is not logged, parseFullReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
            return;
        }
    }
    (void)data;
    (void)size;
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
            switch(subCodeType)
            {
                case 0x06:
                    if(stat==Stat::ProtocolGood)
                    {
                        if(size>=1)
                        {
                            if(data[0x00]==0x01)
                            {
                                stat=Stat::Logged;
                                client->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                            }
                            else
                            {
                                parseNetworkReadError(std::string("invalid code %1 main ident: ").arg(data[0x00])+std::to_string(mainCodeType)+std::string(", sub ident: %1 file:%2:%3").arg(subCodeType).arg(__FILE__).arg(__LINE__));
                                return;
                            }
                        }
                        else
                        {
                            parseNetworkReadError("size==0 main ident: "+std::to_string(mainCodeType)+std::string(", sub ident: %1 file:%2:%3").arg(subCodeType).arg(__FILE__).arg(__LINE__));
                            return;
                        }
                    }
                    else
                    {
                        parseNetworkReadError("stat wrong main ident: "+std::to_string(mainCodeType)+std::string(", sub ident: %1 file:%2:%3").arg(subCodeType).arg(__FILE__).arg(__LINE__));
                        return;
                    }
                break;
            }
        default:
            if(client!=NULL)
                client->postReply(queryNumber,data,size);
        return;
    }
}

void LinkToGameServer::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
