#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../../general/base/CommonSettingsCommon.h"

using namespace CatchChallenger;

bool LinkToGameServer::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    std::cout << "LinkToGameServer::parseInputBeforeLogin mainCodeType" << std::to_string(mainCodeType) << std::endl;
    switch(mainCodeType)
    {
        case 0xA0:
        {
            //Protocol initialization
            if(size<1)
            {
                parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+" and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                return false;
            }
            uint8_t returnCode=data[0x00];
            std::cout << "LinkToGameServer::parseInputBeforeLogin returnCode" << std::to_string(returnCode) << std::endl;
            if(returnCode==0x04 || returnCode<=0x08)
            {
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x08:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zstandard;
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
                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x93;
                ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryIdToReconnect;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,tokenForGameServer,sizeof(tokenForGameServer));

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);

                stat=ProtocolGood;
                return true;
            }
            else
            {
                if(client!=NULL)
                {
                    //send the network reply
                    //client->removeFromQueryReceived(queryNumber);
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
    {
        parseNetworkReadError("parseFullMessage() no client attached: "+std::to_string(mainCodeType));
        return false;
    }
}

//have query with reply
bool LinkToGameServer::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
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

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
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
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
            posOutput+=size;

            return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        return true;
    }
    else
    {
        parseNetworkReadError("parseQuery() no client attached: "+std::to_string(mainCodeType));
        return false;
    }
}

//send reply
bool LinkToGameServer::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    std::cout << "LinkToGameServer::parseReplyData()" << std::endl;
    if(stat!=Stat::Logged)
    {
        if(mainCodeType==0xA0 && queryNumber==0x01 && stat==Stat::Connected)
            return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        else if(mainCodeType!=0x93)
        {
            parseNetworkReadError("is not logged, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+"): "+std::string(__FILE__)+", "+std::to_string(__LINE__));
            return false;
        }
    }
    //do the work here
    switch(mainCodeType)
    {
        //Select character on game server
        case 0x93:
            if(stat==Stat::ProtocolGood)
            {
                if(size>=1)
                {
                    if(data[0x00]==0x01)
                    {
                        stat=Stat::Logged;
                        client->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                        /// \note need reply to AC, convert directly by SIMD the reply, adapt the query id
                        // don't use directly parseReplyData() due to cross reply: this->removeFromQueryReceived(query_id); not client->removeFromQueryReceived(query_id);

                        //send the network reply
                        client->removeFromQueryReceived(queryNumber);
                        ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                        ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumber;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);

                        client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+4+size);

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
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1;

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
                    posOutput+=size;

                    return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                else
                {
                    //dynamic size
                    //send the network message
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1+4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
                    posOutput+=size;

                    return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                return true;
            }
            else
            {
                parseNetworkReadError("parseQuery() no client attached: "+std::to_string(mainCodeType));
                return false;
            }
    }
    return true;
}

void LinkToGameServer::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
