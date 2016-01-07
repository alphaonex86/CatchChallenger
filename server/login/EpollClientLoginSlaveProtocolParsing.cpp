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
        std::cerr << "CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE out of range:" << CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE << std::endl;
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
                parseNetworkReadError("index out of range in array for index "+std::to_string(movePacketKick[index])+", movePacketKick");
                return;
            }
            #endif
            movePacketKick[index]=movePacketKick[index+1];
            movePacketKickTotalCache+=movePacketKick[index];
            index++;
        }
        movePacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=movePacketKickNewValue;
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
                std::cerr << "index out of range <0, chatPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                std::cerr << "index out of range >, chatPacketKick" << std::endl;
            if(chatPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
                std::cerr << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
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
                std::cerr << "index out of range <0, otherPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                std::cerr << "index out of range >, otherPacketKick" << std::endl;
            if(otherPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
                std::cerr << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
        #endif
        otherPacketKickNewValue=0;
    }
}

bool EpollClientLoginSlave::parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    switch(mainCodeType)
    {
        case 0xA0:
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                removeFromQueryReceived(queryNumber);
                //if lot of un logged connection, remove the first
                if(BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
                {
                    EpollClientLoginSlave *client=static_cast<EpollClientLoginSlave *>(BaseServerLogin::tokenForAuth[0].client);
                    client->disconnectClient();
                    delete client;
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
                    }
                }
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(ProtocolParsing::compressionTypeServer)
                {
                    case ProtocolParsing::CompressionType::None:
                        *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                    break;
                    case ProtocolParsing::CompressionType::Zlib:
                        *(EpollClientLoginSlave::protocolReplyCompresssionZlib+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompresssionZlib+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompresssionZlib),sizeof(EpollClientLoginSlave::protocolReplyCompresssionZlib));
                    break;
                    case ProtocolParsing::CompressionType::Xz:
                        *(EpollClientLoginSlave::protocolReplyCompressionXz+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionXz+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionXz),sizeof(EpollClientLoginSlave::protocolReplyCompressionXz));
                    break;
                    case ProtocolParsing::CompressionType::Lz4:
                        *(EpollClientLoginSlave::protocolReplyCompressionLz4+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionLz4+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionLz4),sizeof(EpollClientLoginSlave::protocolReplyCompressionLz4));
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
                BaseServerLogin::tokenForAuthSize++;
                stat=EpollClientLoginStat::ProtocolGood;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                messageParsingLayer("Protocol sended and replied");
                #endif
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(EpollClientLoginSlave::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginSlave::protocolReplyProtocolNotSupported));*/
                parseNetworkReadError("Wrong protocol");
                return false;
            }
        break;
        //login
        case 0xA8:
            if(stat==EpollClientLoginStat::LoginInProgress)
            {
                removeFromQueryReceived(queryNumber);
                *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                parseNetworkReadError("Loggin already in progress");
            }
            else if(stat!=EpollClientLoginStat::ProtocolGood)
            {
                parseNetworkReadError("send login before the protocol");
                return false;
            }
            else
            {
                stat=EpollClientLoginStat::LoginInProgress;
                askLogin(queryNumber,data);
                return false;
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
                return false;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        break;
    }
    return true;
}

bool EpollClientLoginSlave::parseMessage(const uint8_t &mainCodeType, const char * const data, const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    switch(mainCodeType)
    {
        case 0x02:
            if((movePacketKickTotalCache+movePacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many move in sort time, check DDOS limit");
                return false;
            }
            movePacketKickNewValue++;
        break;
        case 0x03:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many chat in sort time, check DDOS limit");
                return false;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
            {
                parseNetworkReadError("Too many packet in sort time, check DDOS limit");
                return false;
            }
            otherPacketKickNewValue++;
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
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
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
                skinId=data[cursor];
                cursor+=1;
            }
            addCharacter(queryNumber,charactersGroupIndex,profileIndex,pseudo,skinId);
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
            removeCharacter(queryNumber,charactersGroupIndex,characterId);
            return true;
        }
        break;
        //Select character
        case 0xAC:
        {
            const uint8_t &charactersGroupIndex=data[0];
            const uint32_t &serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+5)));
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
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
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
    errorParsingLayer(errorString);
}
