#include "LinkToMaster.h"
#include "../base/Client.h"
#include "../base/GlobalServerData.h"
#include "../../general/base/CommonSettingsCommon.h"
#include <iostream>

using namespace CatchChallenger;

void LinkToMaster::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    Q_UNUSED(queryNumber);
    Q_UNUSED(size);
    Q_UNUSED(data);
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::string::number(mainCodeType));
        break;
    }
}

void LinkToMaster::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
}

void LinkToMaster::parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const rawData,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
        return;
    }
    (void)rawData;
    (void)size;
    switch(mainCodeType)
    {
        case 0xE1:
            switch(subCodeType)
            {
                case 0x02:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(size!=4)
                    {
                        parseNetworkReadError("size wrong ident: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
                        return;
                    }
                    #endif
                    const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData)));
                    Client::disconnectClientById(characterId);
                }
                return;
                default:
                    parseNetworkReadError("unknown sub ident: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
                    return;
                break;
            }
        break;
        case 0xF1:
            switch(subCodeType)
            {
                default:
                    parseNetworkReadError("unknown sub ident: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void LinkToMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::Logged)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
}

void LinkToMaster::parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError(std::stringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        case 0x81:
            switch(subCodeType)
            {
                //query because need wait the return (sync/async problem) to send the token for the client connect
                //check if the characterId is linked to the correct account on login server
                case 0x01:
                    if(Q_UNLIKELY(size!=(4+4)))
                    {
                        parseNetworkReadError("unknown sub ident: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
                        return;
                    }
                    else
                    {
                        const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData)));
                        if(Q_LIKELY(!Client::characterConnected(characterId)))
                        {
                            const uint32_t &accountId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+4)));
                            const char * const token=Client::addAuthGetToken(characterId,accountId);
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            queryReceived.remove(queryNumber);
                            #endif
                            if(token!=NULL)
                            {
                                LinkToMaster::protocolReplyGetToken[0x01]=queryNumber;
                                memcpy(LinkToMaster::protocolReplyGetToken+0x03,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                                internalSendRawSmallPacket(LinkToMaster::protocolReplyGetToken,sizeof(LinkToMaster::protocolReplyGetToken));
                            }
                            else
                            {
                                LinkToMaster::protocolReplyNoMoreToken[0x01]=queryNumber;
                                internalSendRawSmallPacket(LinkToMaster::protocolReplyNoMoreToken,sizeof(LinkToMaster::protocolReplyNoMoreToken));
                            }
                        }
                        else
                        {
                            LinkToMaster::protocolReplyAlreadyConnectedToken[0x01]=queryNumber;
                            internalSendRawSmallPacket(LinkToMaster::protocolReplyAlreadyConnectedToken,sizeof(LinkToMaster::protocolReplyAlreadyConnectedToken));
                        }
                    }
                break;
                default:
                    parseNetworkReadError("unknown sub ident: "+std::string::number(mainCodeType)+" "+std::string::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void LinkToMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    switch(mainCodeType)
    {
        case 0x01:
        {
            if(size<1)
            {
                std::cerr << "Need more size for protocol header " << std::endl;
                abort();
            }
            //Protocol initialization
            const uint8_t &returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                if(size!=(1+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "wrong size for protocol header " << returnCode << std::endl;
                    abort();
                }
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
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return;
                }
                stat=Stat::ProtocolGood;
                registerGameServer(CommonSettingsServer::commonSettingsServer.exportedXml,data+1);
                return;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return;
            }
        }
        break;
        case 0x07:
        {
            if(size<1)
            {
                std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            unsigned int pos=1;//skip the first bytes it's the reply code parsed above
            switch(data[0x00])
            {
                case 0x02:
                if((size-pos)<4)
                {
                    std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                {
                    const uint32_t &uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=4;
                    settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
                }
                case 0x01:
                {
                    if((size-pos)<4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        GlobalServerData::serverPrivateVariables.maxMonsterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if((size-pos)<4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                    {
                        std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                    {
                        GlobalServerData::serverPrivateVariables.maxClanId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if((size-pos)<(1+2+1+2))
                    {
                        std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters=data[pos];
                    pos+=1;
                    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems=data[pos];
                    pos+=1;
                    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                    if((size-pos)!=0)
                    {
                        std::cerr << "reply to 07 size remaining != 0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                }
                stat=Stat::Logged;
                return;
                case 0x03:
                    std::cerr << "charactersGroup not found" << settings->value(QLatin1Literal("charactersGroup")).toString().toStdString() << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
                case 0x04:
                    std::cerr << "token wrong (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
                default:
                    std::cerr << "reply return code error (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
            }
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(std::stringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LinkToMaster::parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        std::cerr << "parseFullReplyData() reply to unknown query: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
        abort();
    }
    (void)data;
    (void)size;
    queryNumberList.push_back(queryNumber);
    //do the work here
    switch(mainCodeType)
    {
        case 0x11:
        switch(subCodeType)
        {
            case 0x07:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4)
            {
                std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                abort();
            }
            #endif
            {
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    GlobalServerData::serverPrivateVariables.maxMonsterId.push_back(
                                le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+index*sizeof(unsigned int))))
                                );
                    index++;
                }
            }
            break;
            case 0x08:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4)
            {
                std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                abort();
            }
            #endif
            {
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                {
                    GlobalServerData::serverPrivateVariables.maxClanId.push_back(
                                le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+index*sizeof(unsigned int))))
                                );
                    index++;
                }
            }
            break;
            default:
                std::cerr << "mainCodeType: " << mainCodeType << ", unknown subCodeType: " << subCodeType << " " << __FILE__ << ":" <<__LINE__ << std::endl;
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::string::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(std::stringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LinkToMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
