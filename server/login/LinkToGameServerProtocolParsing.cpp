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
                parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+" and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                return false;
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
                        parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+" and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                if(size!=(sizeof(uint8_t)))
                {
                    parseNetworkReadError("compression type wrong size (stage 3) with main ident: "+std::to_string(mainCodeType)+" and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                //send token to game server
                registerOutputQuery(queryIdToReconnect,0x93);
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x93;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryIdToReconnect;
                posOutput+=1+4;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,tokenForGameServer,sizeof(tokenForGameServer));
                posOutput+=sizeof(tokenForGameServer);

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                stat=ProtocolGood;
                return true;
            }
            else
            {
                if(client!=NULL)
                {
                    //send the network reply
                    client->removeFromQueryReceived(queryNumber);
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1+4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data,size);
                    posOutput+=size;

                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                if(returnCode==0x03)
                    parseNetworkReadError("Server full");
                else
                    parseNetworkReadError("Unknown error "+std::to_string(returnCode));
                return false;
            }
        }
        break;
        default:
            parseNetworkReadError("unknown sort ident reply code: "+std::to_string(mainCodeType)+", line: "+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
}

bool LinkToGameServer::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType));
        return false;
    }
    (void)data;
    (void)size;
    if(client!=NULL)
    {
        uint8_t fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
        if(fixedSize!=0xFE)
        {
            //fixed size
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
            posOutput+=1;

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
            posOutput+=size;

            return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        else
        {
            //dynamic size
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
            posOutput+=size;

            return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
    }
    else
        return false;
}

//have query with reply
bool LinkToGameServer::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::Logged)
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    if(client!=NULL)
    {
        client->registerOutputQuery(queryNumber,mainCodeType);
        uint8_t fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
        if(fixedSize!=0xFE)
        {
            //fixed size
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1;

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
            posOutput+=size;

            return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        else
        {
            //dynamic size
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
            posOutput+=size;

            return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        return true;
    }
    else
        return false;
}

//send reply
bool LinkToGameServer::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        if(mainCodeType==0xA0 && queryNumber==0x01 && stat==Stat::Connected)
            return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        else
        {
            parseNetworkReadError("is not logged, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
            return false;
        }
    }
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    //do the work here
    switch(mainCodeType)
    {
        case 0x93:
            if(stat==Stat::ProtocolGood)
            {
                if(size>=1)
                {
                    if(data[0x00]==0x01)
                    {
                        stat=Stat::Logged;
                        client->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                        return true;
                    }
                    else
                    {
                        parseNetworkReadError("invalid code "+std::to_string(data[0x00])+" main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                }
                else
                {
                    parseNetworkReadError("size==0 main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
            }
            else
            {
                parseNetworkReadError("stat wrong main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
        break;
        default:
            if(client!=NULL)
            {
                client->removeFromQueryReceived(queryNumber);
                const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType+128];
                if(fixedSize!=0xFE)
                {
                    //fixed size
                    //send the network message
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1;

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
                    posOutput+=size;

                    return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                else
                {
                    //dynamic size
                    //send the network message
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1+4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
                    posOutput+=size;

                    return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                return true;
            }
        return false;
    }
    return true;
}

void LinkToGameServer::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
