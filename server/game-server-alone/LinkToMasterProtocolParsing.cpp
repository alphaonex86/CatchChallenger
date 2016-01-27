#include "LinkToMaster.h"
#include "../base/Client.h"
#include "../base/GlobalServerData.h"
#include "../../general/base/CommonSettingsCommon.h"
#include <iostream>

using namespace CatchChallenger;

bool LinkToMaster::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &, const char * const , const unsigned int &)
{
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        return false;
    }
}

bool LinkToMaster::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType));
        return false;
    }
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        case 0x4D:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=4)
            {
                parseNetworkReadError("size wrong ident: "+std::to_string(mainCodeType));
                return false;
            }
            #endif
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data)));
            Client::disconnectClientById(characterId);
        }
        return true;
        /*case 0xF1:
            switch(subCodeType)
            {
                default:
                    parseNetworkReadError("unknown sub ident: "+std::to_string(mainCodeType));
                    return;
                break;
            }
        break;*/
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
}

//have query with reply
bool LinkToMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    }
    //do the work here
    switch(mainCodeType)
    {
        case 0xF8:
            if(Q_UNLIKELY(size!=(4+4)))
            {
                parseNetworkReadError("unknown sub ident: "+std::to_string(mainCodeType));
                return false;
            }
            else
            {
                //send the network reply
                removeFromQueryReceived(queryNumber);

                const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data)));
                if(Q_LIKELY(!Client::characterConnected(characterId)))
                {
                    const uint32_t &accountId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+4)));
                    const char * const token=Client::addAuthGetToken(characterId,accountId);
                    if(token!=NULL)
                    {
                        LinkToMaster::protocolReplyGetToken[0x01]=queryNumber;
                        memcpy(LinkToMaster::protocolReplyGetToken+1+1+4,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
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
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    return true;
}

//send reply
bool LinkToMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    //do the work here
    switch(mainCodeType)
    {
        //Protocol initialization and auth for master
        case 0xB8:
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
                    return false;
                }
                stat=Stat::ProtocolGood;
                registerGameServer(CommonSettingsServer::commonSettingsServer.exportedXml,data+1);
                return true;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return false;
            }
        }
        return true;
        //Register game server
        case 0xB2:
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
                    settings->beginGroup("master");
                    settings->setValue("uniqueKey",std::to_string(uniqueKey));
                    std::cerr << "unique key conflict, renew it by: " << uniqueKey << std::endl;
                    settings->endGroup();
                    settings->sync();
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
                return true;
                case 0x03:
                    std::cerr << "charactersGroup not found" << settings->value("charactersGroup") << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
        return true;
        //get maxMonsterId block
        case 0xB0:
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4)
        {
            std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4: mainCodeType: " << mainCodeType << ", queryNumber: " << queryNumber << std::endl;
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
        if(vectorHaveDuplicatesForSmallList(GlobalServerData::serverPrivateVariables.maxMonsterId))
        {
            std::cerr << "reply to 08: duplicate maxMonsterId in " << __FILE__ << ":" <<__LINE__ << std::endl;
            abort();
        }
        return true;
        //get maxClanId block
        case 0xB1:
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4)
        {
            std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4: mainCodeType: " << mainCodeType << ", queryNumber: " << queryNumber << std::endl;
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
        if(vectorHaveDuplicatesForSmallList(GlobalServerData::serverPrivateVariables.maxClanId))
        {
            std::cerr << "reply to 08: duplicate maxClanId in " << __FILE__ << ":" <<__LINE__ << std::endl;
            abort();
        }
        return true;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    parseNetworkReadError("The server for now not ask anything: "+std::to_string(mainCodeType)+" "+std::to_string(queryNumber));
    return false;
}

void LinkToMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
