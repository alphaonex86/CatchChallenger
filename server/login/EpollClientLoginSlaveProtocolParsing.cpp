#include "EpollClientLoginSlave.hpp"
#include "LinkToGameServer.hpp"
#include "../base/BaseServerLogin.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/cpp11addition.hpp"

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
    movePacketKick.flush();
    chatPacketKick.flush();
    otherPacketKick.flush();
}

bool EpollClientLoginSlave::parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)size;
    std::cout << " EpollClientLoginSlave::parseInputBeforeLogin(" << std::to_string(mainCodeType) << "," << std::to_string(queryNumber) << "," << binarytoHexa(data,size) << ")" << std::endl;
    if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
    switch(mainCodeType)
    {
        case 0xA0:
            std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                removeFromQueryReceived(queryNumber);
                if(stat!=EpollClientLoginStat::None)
                {
                    parseNetworkReadError("stat!=EpollClientLoginStat::None for case 0xA0");
                    return false;
                }
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                //if lot of un logged connection, remove the first
                if(BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
                {
                    EpollClientLoginSlave *client=static_cast<EpollClientLoginSlave *>(BaseServerLogin::tokenForAuth[0].client);
                    std::cerr << "BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION: " << client << std::endl;
                    client->disconnectClient();
                    BaseServerLogin::tokenForAuthSize--;
                    if(BaseServerLogin::tokenForAuthSize>0)
                    {
                        uint32_t index=0;
                        while(index<BaseServerLogin::tokenForAuthSize)
                        {
                            BaseServerLogin::tokenForAuth[index]=BaseServerLogin::tokenForAuth[index+1];
                            index++;
                        }
                        //don't work:memmove(BaseServerLogin::tokenForAuth,BaseServerLogin::tokenForAuth+sizeof(TokenLink),BaseServerLogin::tokenForAuthSize*sizeof(TokenLink));
                        //don't set the last wrong entry to improve performance againts DDOS
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(BaseServerLogin::tokenForAuth[0].client==NULL)
                            abort();
                        #endif
                    }
                }
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                BaseServerLogin::TokenLink *token=&BaseServerLogin::tokenForAuth[BaseServerLogin::tokenForAuthSize];
                {
                    token->client=this;
                    if(BaseServerLogin::fpRandomFile==NULL)
                    {
                        //insecure implementation
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
                            parseNetworkReadError("Not correct number of byte to generate the token: size!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT: "+std::to_string(size)+"!="+std::to_string(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT));
                            return false;
                        }
                        //messageParsingLayer("auth token send: "+binarytoHexa(token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT));
                    }
                }
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(CompressionProtocol::compressionTypeServer)
                {
                    case CompressionProtocol::CompressionType::None:
                        *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                    break;
                    case CompressionProtocol::CompressionType::Zstandard:
                        *(EpollClientLoginSlave::protocolReplyCompresssionZstandard+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompresssionZstandard+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompresssionZstandard),sizeof(EpollClientLoginSlave::protocolReplyCompresssionZstandard));
                    break;
                    default:
                        parseNetworkReadError("Compression selected wrong");
                    return false;
                }
                #else
                *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+7,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                #endif
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                BaseServerLogin::tokenForAuthSize++;
                stat=EpollClientLoginStat::ProtocolGood;
            }
            else
            {
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                /*don't send packet to prevent DDOS
                *(EpollClientLoginSlave::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginSlave::protocolReplyProtocolNotSupported));*/
                parseNetworkReadError("Wrong protocol");
                return false;
            }
        break;
        //login
        case 0xA8:
            std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                removeFromQueryReceived(queryNumber);
                *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                parseNetworkReadError("Loggin already in progress");
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                parseNetworkReadError("send login before the protocol");
                return false;
            }
            else
            {
                std::cout << " EpollClientLoginSlave::parseInputBeforeLogin() " << __FILE__ << ":" << __LINE__ << std::endl;
                stat=EpollClientLoginStat::LoginInProgress;
                askLogin(queryNumber,data);
                return true;
            }
        break;
        //create account
        case 0xA9:
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                parseNetworkReadError("Loggin already in progress");
                return false;
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                parseNetworkReadError("send login before the protocol");
                return false;
            }
            else
            {
                if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
                {
                    stat=EpollClientLoginStat::LoginInProgress;
                    createAccount(queryNumber,data);
                    return true;
                }
                else
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                    #endif
                    //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                    //not reply to prevent DDOS attack
                    parseNetworkReadError("Account creation not premited");
                    return false;
                }
            }
        break;
        //Stat client
        case 0xAD:
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                parseNetworkReadError("Loggin already in progress");
                return false;
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                parseNetworkReadError("send login before the protocol");
                return false;
            }
            else
            {
                askStatClient(queryNumber,data);
                return true;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType)+", stat: "+std::to_string(stat));
        break;
    }
    return true;
}

bool EpollClientLoginSlave::parseMessage(const uint8_t &mainCodeType, const char * const data, const unsigned int &size)
{
    std::cout << " EpollClientLoginSlave::parseMessage(" << std::to_string(mainCodeType) << "," << binarytoHexa(data,size) << ")" << std::endl;
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    switch(mainCodeType)
    {
        case 0x02:
            if(movePacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many move in sort time, check DDOS limit: ("+std::to_string(movePacketKick.total())+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITMOVE));
                return false;
            }
            movePacketKick.incrementLastValue();
        break;
        case 0x03:
            if(chatPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many chat in sort time, check DDOS limit: ("+std::to_string(chatPacketKick.total())+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITCHAT));
                return false;
            }
            chatPacketKick.incrementLastValue();
        break;
        default:
            if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
            {
                parseNetworkReadError("Too many packet in sort time, check DDOS limit: ("+std::to_string(otherPacketKick.total())+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITOTHER));
                return false;
            }
            otherPacketKick.incrementLastValue();
        break;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
            if(fixedSize!=0xFE)
            {
                //fixed size
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
                posOutput+=1;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
                posOutput+=size;

                return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

                return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+std::to_string(mainCodeType));
            return false;
        }
    }
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
}

//have query with reply
bool EpollClientLoginSlave::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    std::cout << " EpollClientLoginSlave::parseQuery(" << std::to_string(mainCodeType) << "," << std::to_string(queryNumber) << "," << binarytoHexa(data,size) << ")" << std::endl;
    if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
    if(stat!=EpollClientLoginStat::Logged)
    {
        if(stat==EpollClientLoginStat::GameServerConnecting)
        {
            parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
            return false;
        }
        if(stat==EpollClientLoginStat::GameServerConnected)
        {
            if(Q_LIKELY(linkToGameServer))
            {
                linkToGameServer->registerOutputQuery(queryNumber,mainCodeType);
                const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
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

                    return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

                    return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
            }
            else
            {
                parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+std::to_string(mainCodeType));
                return false;
            }
        }
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    }
    //do the work here
    switch(mainCodeType)
    {
        //Add character
        case 0xAA:
        {
            int cursor=0;
            uint8_t charactersGroupIndex;
            uint8_t profileIndex;
            std::string pseudo;
            uint8_t monsterGroupId;
            uint8_t skinId;
            uint8_t pseudoSize;
            {
                if((size-cursor)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                charactersGroupIndex=data[cursor];
                cursor+=1;
            }
            {
                if((size-cursor)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                profileIndex=data[cursor];
                cursor+=1;
            }
            {
                if((size-cursor)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                pseudoSize=data[cursor];
                cursor+=1;
                if((size-cursor)<pseudoSize)
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                pseudo=std::string(data+cursor,pseudoSize);
                cursor+=pseudoSize;
            }
            {
                if((size-cursor)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("error to get skin with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                monsterGroupId=data[cursor];
                cursor+=1;
            }
            {
                if((size-cursor)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("error to get skin with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                skinId=data[cursor];
                cursor+=1;
            }
            addCharacter(queryNumber,charactersGroupIndex,profileIndex,pseudo,monsterGroupId,skinId);
            if((size-cursor)!=0)
            {
                parseNetworkReadError("remaining data: parseQuery("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
                return false;
            }
            return true;
        }
        break;
        //Remove character
        case 0xAB:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint32_t)+(int)sizeof(uint8_t)))
            {
                parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint8_t &charactersGroupIndex=data[0];
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            removeCharacterLater(queryNumber,charactersGroupIndex,characterId);
            return true;
        }
        break;
        //Select character
        case 0xAC:
        {
            const uint8_t &charactersGroupIndex=data[0];
            const uint32_t &serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+5)));
            if(characterId==0)
                std::cerr << "EpollClientLoginMaster::selectCharacter() call with characterId=0" << std::endl;
            selectCharacter(queryNumber,serverUniqueKey,charactersGroupIndex,characterId);
            return true;
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
bool EpollClientLoginSlave::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    std::cout << " EpollClientLoginSlave::parseReplyData(" << std::to_string(mainCodeType) << "," << std::to_string(queryNumber) << "," << binarytoHexa(data,size) << ")" << std::endl;
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            linkToGameServer->removeFromQueryReceived(queryNumber);
            const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType+128];
            if(fixedSize!=0xFE)
            {
                //fixed size
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
                posOutput+=size;

                return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
            else
            {
                //dynamic size
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
                posOutput+=size;

                return linkToGameServer->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+std::to_string(mainCodeType));
            return false;
        }
    }
    //queryNumberList << queryNumber;
    parseNetworkReadError("The server for now not ask anything: "+std::to_string(mainCodeType)+", "+std::to_string(queryNumber));
    return false;
}

void EpollClientLoginSlave::parseNetworkReadError(const std::string &errorString)
{
    /*not parseNetworkReadError to do soft error, else crash on removeFromQueryReceived();
     * used to prevent abort in case of: ERROR, dns resolution failed on: cc-server-bot.portable-datacenter.first-world.info, h_errno: 2 */
    errorParsingLayer(errorString);
}

void EpollClientLoginSlave::parseNetworkReadMessage(const std::string &errorString)
{
    /*not parseNetworkReadError to do soft error, else crash on removeFromQueryReceived();
     * used to prevent abort in case of: ERROR, dns resolution failed on: cc-server-bot.portable-datacenter.first-world.info, h_errno: 2 */
    messageParsingLayer(errorString);
}
