#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "../../general/base/CommonSettingsCommon.h"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.h"
#endif

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_DDOS_FILTER
void Client::doDDOSCompute()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue<0 || GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue>CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        std::cerr << "GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue out of range:" << GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue << std::endl;
        return;
    }
    #endif
    {
        movePacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
            {
                errorOutput("index out of range <0, movePacketKick");
                return;
            }
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
            {
                errorOutput("index out of range >, movePacketKick");
                return;
            }
            if(movePacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitMove*2)
            {
                errorOutput("index out of range in array for index "+std::to_string(movePacketKick[index])+", movePacketKick");
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
        if(movePacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitMove*2)
        {
            errorOutput("bug in DDOS calculation count");
            return;
        }
        #endif
        movePacketKickNewValue=0;
    }
    {
        chatPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                std::cerr << "index out of range <0, chatPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                std::cerr << "index out of range >, chatPacketKick" << std::endl;
            if(chatPacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitChat*2)
                std::cerr << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitChat*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
        #endif
        chatPacketKickNewValue=0;
    }
    {
        otherPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                std::cerr << "index out of range <0, otherPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                std::cerr << "index out of range >, otherPacketKick" << std::endl;
            if(otherPacketKick[index]>GlobalServerData::serverSettings.ddos.kickLimitOther*2)
                std::cerr << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>GlobalServerData::serverSettings.ddos.kickLimitOther*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
        #endif
        otherPacketKickNewValue=0;
    }
}
#endif

void Client::sendNewEvent(char * const data, const uint32_t &size)
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of sendNewEvent");
        return;
    }

    //send the network reply
    data[0x01]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xE2);

    sendRawBlock(data,size);

    queryNumberList.pop_back();
}

void Client::teleportTo(CommonMap *map,const /*COORD_TYPE*/uint8_t &x,const /*COORD_TYPE*/uint8_t &y,const Orientation &orientation)
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of teleportation");
        return;
    }
    PlayerOnMap teleportationPoint;
    teleportationPoint.map=map;
    teleportationPoint.x=x;
    teleportationPoint.y=y;
    teleportationPoint.orientation=orientation;
    lastTeleportation.push(teleportationPoint);

    //send the network reply
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xE1;
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xE1);
    queryNumberList.pop_back();

    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(/*map:*/1+1+1+1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+0]=map->id;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+1]=x;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+2]=y;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+3]=(uint8_t)orientation;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+4+/*map:*/1+1+1+1);
    }
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(/*map:*/2+1+1+1);//set the dynamic size
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4)=htole16(map->id);
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+2]=x;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+3]=y;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+4]=(uint8_t)orientation;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+4+/*map:*/2+1+1+1);
    }
    else
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(/*map:*/4+1+1+1);//set the dynamic size
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4)=htole32(map->id);
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+4]=x;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+5]=y;
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+6]=(uint8_t)orientation;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+4+/*map:*/4+1+1+1);
    }
}

void Client::sendTradeRequest(char * const data,const uint32_t &size)
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of trade");
        return;
    }
    data[1]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xE0);
    sendRawBlock(data,size);
    queryNumberList.pop_back();
}

void Client::sendBattleRequest(char * const data, const uint32_t &size)
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of trade");
        return;
    }
    data[1]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xDF);
    sendRawBlock(data,size);
    queryNumberList.pop_back();
}

bool Client::parseInputBeforeLogin(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    if(stopIt)
        return false;
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("parseInputBeforeLogin("+std::to_string(packetCode)+","+std::to_string(queryNumber)+","+binarytoHexa(data,size)+")");
    #endif
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    (void)size;
    #endif
    switch(packetCode)
    {
        //protocol header
        case 0xA0:
            if(memcmp(data,Client::protocolHeaderToMatch,sizeof(Client::protocolHeaderToMatch))==0)
            {
                if(stat!=ClientStat::None)
                {
                    errorOutput("stat!=EpollClientLoginStat::None for case 0xA0");
                    return false;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                inputQueryNumberToPacketCode[queryNumber]=0;
                if(GlobalServerData::serverPrivateVariables.connected_players>=GlobalServerData::serverSettings.max_players)
                {
                    *(Client::protocolReplyServerFull+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyServerFull),sizeof(Client::protocolReplyServerFull));
                    disconnectClient();
                    //errorOutput(Client::text_server_full);DDOS -> full the log
                    return false;
                }
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                //if lot of un logged connection, remove the first
                if(BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
                {
                    //remove the first
                    Client *client=static_cast<Client *>(BaseServerLogin::tokenForAuth[0].client);
                    client->disconnectClient();
                    BaseServerLogin::tokenForAuthSize--;
                    //move the last
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
                #endif
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                BaseServerLogin::TokenLink *token=&BaseServerLogin::tokenForAuth[BaseServerLogin::tokenForAuthSize];
                {
                    token->client=this;
                    #ifdef __linux__
                    if(BaseServerLogin::fpRandomFile==NULL)
                    {
                        //failback
                        /* allow poor quality number:
                         * 1) more easy to run, allow start include if RANDOMFILEDEVICE can't be read
                         * 2) it's for very small server (Lan) or internal communication */
                        #if ! defined(CATCHCHALLENGER_CLIENT) && ! defined(CATCHCHALLENGER_SOLO)
                        /// \warning total insecure implementation
                        abort();
                        #endif
                        int index=0;
                        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            token->value[index]=rand()%256;
                            index++;
                        }
                    }
                    else
                    {
                        const int32_t readedByte=fread(token->value,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,BaseServerLogin::fpRandomFile);
                        if(readedByte!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            /// \todo check why client not disconnected if pass here
                            errorOutput(
                                        "Not correct number of byte to generate the token readedByte!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT: "+
                                        std::to_string(readedByte)+
                                        "!="+
                                        std::to_string(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)+
                                        ", errno: "+
                                        std::to_string(errno)+"");
                            return false;
                        }
                    }
                    #else
                    /// \warning total insecure implementation
                    // not abort(); because under not linux will not work
                    int index=0;
                    while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                    {
                        token->value[index]=rand()%256;
                        index++;
                    }
                    #endif
                }
                #endif
                //normalOutput(std::string("Protocol reply send with token: ")+binarytoHexa(token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT));
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(ProtocolParsing::compressionTypeServer)
                {
                    case CompressionType_None:
                        *(Client::protocolReplyCompressionNone+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompressionNone+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                    break;
                    case CompressionType_Zlib:
                        *(Client::protocolReplyCompresssionZlib+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompresssionZlib+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompresssionZlib),sizeof(Client::protocolReplyCompresssionZlib));
                    break;
                    case CompressionType_Xz:
                        *(Client::protocolReplyCompressionXz+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompressionXz+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionXz),sizeof(Client::protocolReplyCompressionXz));
                    break;
                    default:
                        errorOutput("Compression selected wrong");
                    return false;
                }
                #else
                *(Client::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(Client::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                #endif
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                BaseServerLogin::tokenForAuthSize++;
                #endif
                stat=ClientStat::ProtocolGood;
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(Client::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyProtocolNotSupported),sizeof(Client::protocolReplyProtocolNotSupported));*/
                errorOutput("Wrong protocol");
                return false;
            }
        break;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        //Get first data and send the login
        case 0xA8:
            if(stat!=ClientStat::ProtocolGood)
            {
                errorOutput("send login before the protocol");
                return false;
            }
            if(stat==ClientStat::LoginInProgress)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(queryNumber);
                #endif
                //replyOutputSize.remove(queryNumber);
                errorOutput("Loggin already in progress");
                return false;
            }
            else
            {
                stat=ClientStat::LoginInProgress;
                return askLogin(queryNumber,data);
            }
        break;
        //create account
        case 0xA9:
            if(stat!=ClientStat::ProtocolGood)
            {
                errorOutput("send login before the protocol");
                return false;
            }
            if(stat==ClientStat::LoginInProgress)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                #endif
                //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                errorOutput("Loggin already in progress at create account");
                return false;
            }
            else
            {
                if(GlobalServerData::serverSettings.automatic_account_creation)
                {
                    stat=ClientStat::LoginInProgress;
                    return createAccount(queryNumber,data);
                }
                else
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                    #endif
                    //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                    //not reply to prevent DDOS attack
                    errorOutput("Account creation not premited");
                    return false;
                }
            }
        break;
        //stat client
        case 0xAD:
            if(stat!=ClientStat::ProtocolGood)
            {
                errorOutput("send login before the protocol");
                return false;
            }
            if(stat==ClientStat::LoginInProgress)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(queryNumber);//all list dropped at client destruction
                #endif
                //replyOutputSize.remove(queryNumber);//all list dropped at client destruction
                //not reply to prevent DDOS attack
                errorOutput("Loggin already in progress at create account");
                return false;
            }
            else
            {
                askStatClient(queryNumber,data);
                return true;
            }
        break;
        #else
        //Select character on game server
        case 0x93:
        {
            if(stat!=ClientStat::ProtocolGood)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            selectCharacter(queryNumber,data);
        }
        break;
        #endif
        default:
            errorOutput("wrong data before login with mainIdent: "+std::to_string(packetCode));
            return false;
        break;
    }
    return true;
}

void Client::moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction)
{
    moveThePlayer(previousMovedUnit,static_cast<Direction>(direction));
}

bool Client::parseMessage(const uint8_t &packetCode,const char * const data,const int unsigned &size)
{
    if(stopIt)
        return false;
    if(stat==ClientStat::None)
    {
        disconnectClient();
        return false;
    }
    if(stat!=ClientStat::CharacterSelected)
    {
        //wrong protocol
        disconnectClient();
        //parseError(std::stringLiteral("is not logged, parsenormalOutput(%1)").arg(packetCode));
        return false;
    }
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    switch(packetCode)
    {
        case 0x02:
            if((movePacketKickTotalCache+movePacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitMove)
            {
                errorOutput("Too many move in sort time, check DDOS limit");
                return false;
            }
            movePacketKickNewValue++;
        break;
        case 0x03:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitChat)
            {
                errorOutput("Too many chat in sort time, check DDOS limit");
                return false;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
            {
                errorOutput("Too many packet in sort time, check DDOS limit");
                return false;
            }
            otherPacketKickNewValue++;
        break;
    }
    #endif
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("parsenormalOutput("+std::to_string(packetCode)+","+binarytoHexa(data,size)+")");
    #endif
    switch(packetCode)
    {
        case 0x02:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)*2)
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            #endif
            const uint8_t &direction=*(data+sizeof(uint8_t));
            if(direction<1 || direction>8)
            {
                errorOutput("Bad direction number: "+std::to_string(direction));
                return false;
            }
            moveThePlayer(static_cast<uint8_t>(*data),static_cast<Direction>(direction));
            return true;
        }
        break;
        //Chat
        case 0x03:
        {
            uint32_t pos=0;
            if(size<((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for chat");
                return false;
            }
            const uint8_t &chatType=data[pos];
            pos+=1;
            if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
            {
                errorOutput("chat type error: "+std::to_string(chatType));
                return false;
            }
            if(chatType==Chat_type_pm)
            {
                if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
                {
                    std::string text;
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            errorOutput("wrong utf8 to std::string size in PM for text size");
                            return false;
                        }
                        const uint8_t &textSize=data[pos];
                        pos+=1;
                        if(textSize>0)
                        {
                            if((size-pos)<textSize)
                            {
                                errorOutput("wrong utf8 to std::string size in PM for text");
                                return false;
                            }
                            text=std::string(data+pos,textSize);
                            pos+=textSize;
                        }
                    }
                    std::string pseudo;
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            errorOutput("wrong utf8 to std::string size in PM for pseudo");
                            return false;
                        }
                        const uint8_t &pseudoSize=data[pos];
                        pos+=1;
                        if(pseudoSize>0)
                        {
                            if((size-pos)<pseudoSize)
                            {
                                errorOutput("wrong utf8 to std::string size in PM for pseudo");
                                return false;
                            }
                            pseudo=std::string(data+pos,pseudoSize);
                            pos+=pseudoSize;
                        }
                    }

                    normalOutput(Client::text_slashpmspace+pseudo+Client::text_space+text);
                    sendPM(text,pseudo);
                }
                else
                {
                    errorOutput("can't send pm because is disabled: "+std::to_string(chatType));
                    return false;
                }
            }
            else
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    errorOutput("wrong utf8 to std::string header size");
                    return false;
                }
                std::string text;
                const uint8_t &textSize=data[pos];
                pos+=1;
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        errorOutput("wrong utf8 to std::string size");
                        return false;
                    }
                    text=std::string(data+pos,textSize);
                    pos+=textSize;
                }

                if(!stringStartWith(text,'/'))
                {
                    if(chatType==Chat_type_local)
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
                            sendLocalChatText(text);
                        else
                        {
                            errorOutput("can't send chat local because is disabled: "+std::to_string(chatType));
                            return false;
                        }
                    }
                    else
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_clan || CommonSettingsServer::commonSettingsServer.chat_allow_all)
                            sendChatText((Chat_type)chatType,text);
                        else
                        {
                            errorOutput("can't send chat other because is disabled: "+std::to_string(chatType));
                            return false;
                        }
                    }
                }
                else
                {
                    std::string command;
                    /// \warning don't use regex here, slow and do DDOS risk
                    std::size_t found=text.find(Client::text_space);
                    if(found!=std::string::npos)
                    {
                        command=text.substr(1,found-1);
                        text=text.substr(found+1,text.size()-found-1);
                    }
                    else
                        command=text.substr(1,text.size()-1);

                    //the normal player command
                    {
                        if(command==Client::text_playernumber)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                        else if(command==Client::text_playerlist)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+" "+text);
                            return true;
                        }
                        else if(command==Client::text_trade)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                        else if(command==Client::text_battle)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                    }
                    //the admin command
                    if(public_and_private_informations.public_informations.type==Player_type_gm || public_and_private_informations.public_informations.type==Player_type_dev || GlobalServerData::serverSettings.everyBodyIsRoot)
                    {
                        if(command==Client::text_give)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_setevent)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_take)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_tp)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_kick)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_chat)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_setrights)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_stop || command==Client::text_restart)
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVER
                            BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                            #endif
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else
                        {
                            normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                            receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                        }
                    }
                    else
                    {
                        normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                        receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                    }
                }
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            return true;
        }
        break;
        //Clan invite accept
        case 0x04:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for clan invite");
                return false;
            }
            #endif
            const uint8_t &returnCode=*(data+sizeof(uint8_t));
            switch(returnCode)
            {
                case 0x01:
                    clanInvite(true);
                break;
                case 0x02:
                    clanInvite(false);
                break;
                default:
                    errorOutput("wrong return code for clan invite ident: "+std::to_string(packetCode));
                return false;
            }
            return true;
        }
        break;
        case 0x05:
        {
        }
        break;
        case 0x06:
        {
        }
        break;
        //Try escape
        case 0x07:
            tryEscape();
        break;
        case 0x08:
        {
        }
        break;
        //Learn skill
        case 0x09:
        {
            if(size!=((int)sizeof(uint8_t)+sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for learn skill");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            const uint16_t &skill=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint8_t))));
            return learnSkill(monsterPosition,skill);
        }
        break;
        case 0x0A:
        {
        }
        break;
        //Heal all the monster
        case 0x0B:
            heal();
        break;
        //Request bot fight
        case 0x0C:
        {
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for request bot fight");
                return false;
            }
            const uint16_t &fightId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            requestFight(fightId);
            return true;
        }
        break;
        //move the monster
        case 0x0D:
        {
            if(size!=((int)sizeof(uint8_t)*2))
            {
                errorOutput("wrong remaining size for move monster");
                return false;
            }
            const uint8_t &moveWay=*data;
            bool moveUp;
            switch(moveWay)
            {
                case 0x01:
                    moveUp=true;
                break;
                case 0x02:
                    moveUp=false;
                break;
                default:
                    errorOutput("wrong move up value");
                return false;
            }
            const uint8_t &position=*(data+sizeof(uint8_t));
            moveMonster(moveUp,position);
            return true;
        }
        break;
        //change monster in fight, monster id in db
        case 0x0E:
        {
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for monster position in fight");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            return changeOfMonsterInFight(monsterPosition);
        }
        break;
        /// \todo check double validation
        //Monster evolution validated
        case 0x0F:
        {
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for monster evolution validated");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            confirmEvolution(monsterPosition);
            return true;
        }
        break;
        //Use object on monster
        case 0x10:
        {
            if(size<((int)sizeof(uint16_t)+(int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for use object on monster");
                return false;
            }
            const uint16_t &item=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint8_t &monsterPosition=data[sizeof(uint16_t)];
            return useObjectOnMonsterByPosition(item,monsterPosition);
        }
        break;
        //use skill
        case 0x11:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            #endif
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            useSkill(le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data))));
            return true;
        }
        break;
        case 0x12:
        {
        }
        break;
        //Destroy an object
        case 0x13:
        {
            if(size!=((int)sizeof(uint16_t)+sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for destroy item id");
                return false;
            }
            const uint16_t &itemId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            destroyObject(itemId,quantity);
            return true;
        }
        break;
        //Put object into a trade
        case 0x14:
        {
            uint32_t pos=0;
            if(size<((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for trade add type");
                return false;
            }
            const uint8_t &type=data[pos];
            pos+=1;
            switch(type)
            {
                //cash
                case 0x01:
                {
                    if((size-pos)<((int)sizeof(uint64_t)))
                    {
                        errorOutput("wrong remaining size for trade add cash");
                        return false;
                    }
                    uint64_t tempVar;
                    memcpy(&tempVar,data+pos,sizeof(uint64_t));
                    const uint64_t &cash=le64toh(tempVar);
                    pos+=sizeof(uint64_t);
                    tradeAddTradeCash(cash);
                }
                break;
                //item
                case 0x02:
                {
                    if((size-pos)<((int)sizeof(uint16_t)))
                    {
                        errorOutput("wrong remaining size for trade add item id");
                        return false;
                    }
                    const uint16_t &item=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        errorOutput("wrong remaining size for trade add item quantity");
                        return false;
                    }
                    const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint32_t);
                    tradeAddTradeObject(item,quantity);
                }
                break;
                //monster
                case 0x03:
                {
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        errorOutput("wrong remaining size for trade add monster");
                        return false;
                    }
                    const uint32_t &monsterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint32_t);
                    tradeAddTradeMonster(monsterId);
                }
                break;
                default:
                    errorOutput("wrong type for trade add");
                    return false;
                break;
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parsenormalOutput("+
                                      std::to_string(packetCode)+
                                      "): "+
                                      binarytoHexa(data,pos)+
                                      " "+
                                      binarytoHexa(data+pos,size-pos)
                                      );
                return false;
            }
            return true;
        }
        break;
        //trade finished after the accept
        case 0x15:
            tradeFinished();
        break;
        //trade canceled after the accept
        case 0x16:
            tradeCanceled();
        break;
        //deposite/withdraw to the warehouse
        case 0x17:
        {
            uint32_t pos=0;
            std::vector<std::pair<uint16_t, int32_t> > items;
            std::vector<uint32_t> withdrawMonsters;
            std::vector<uint32_t> depositeMonsters;
            if((size-pos)<((int)sizeof(int64_t)))
            {
                errorOutput("wrong remaining size for trade add monster");
                return false;
            }
            uint64_t tempVar;
            memcpy(&tempVar,data+pos,sizeof(uint64_t));
            const uint64_t &cash=le64toh(tempVar);
            pos+=sizeof(uint64_t);

            uint16_t size16;
            if((size-pos)<((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for trade add monster");
                return false;
            }
            size16=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint16_t);

            {
                uint16_t id;
                uint32_t index=0;
                while(index<size16)
                {
                    if((size-pos)<((int)sizeof(uint16_t)))
                    {
                        errorOutput("wrong remaining size for trade add monster");
                        return false;
                    }
                    id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        errorOutput("wrong remaining size for trade add monster");
                        return false;
                    }
                    const int32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(int32_t);
                    items.push_back(std::pair<uint16_t, int32_t>(id,quantity));
                    index++;
                }
            }
            if((size-pos)<((int)sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for trade add monster");
                return false;
            }
            uint32_t size;
            size=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            uint32_t index=0;
            while(index<size)
            {
                if((size-pos)<((int)sizeof(uint32_t)))
                {
                    errorOutput("wrong remaining size for trade add monster");
                    return false;
                }
                const uint32_t &id=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                withdrawMonsters.push_back(id);
                index++;
            }
            if((size-pos)<((int)sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for trade add monster");
                return false;
            }
            size=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<size)
            {
                if((size-pos)<((int)sizeof(uint32_t)))
                {
                    errorOutput("wrong remaining size for trade add monster");
                    return false;
                }
                const uint32_t &id=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                depositeMonsters.push_back(id);
                index++;
            }
            wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parsenormalOutput("+
                            std::to_string(packetCode)+
                            "): "+
                            binarytoHexa(data,pos)+
                            " "+
                            binarytoHexa(data+pos,size-pos)
                            );
                return false;
            }
            return true;
        }
        break;
        case 0x18:
            takeAnObjectOnMap();
        break;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        //Use seed into dirt
        case 0x19:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint8_t &plant_id=*data;
            plantSeed(plant_id);
            return true;
        }
        break;
        //Collect mature plant
        case 0x1A:
            collectPlant();
        break;
        #else
        //continous switch to improve the performance
        case 0x19:
        case 0x1A:
            errorOutput("unknown main ident: "+std::to_string(packetCode)+" CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER is not def");
            return false;
        break;
        #endif
        //Quest start
        case 0x1B:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest start");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Start,questId);
            return true;
        }
        break;
        //Quest finish
        case 0x1C:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest finish");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Finish,questId);
            return true;
        }
        break;
        //Quest cancel
        case 0x1D:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest cancel");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Cancel,questId);
            return true;
        }
        break;
        //Quest next step
        case 0x1E:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest next step");
                return false;
            }
            #endif
            const uint32_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_NextStep,questId);
            return true;
        }
        break;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //Waiting for city capture
        case 0x1F:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for city capture");
                return false;
            }
            #endif
            const uint8_t &cancel=*data;
            if(cancel==0x00)
                waitingForCityCaputre(false);
            else
                waitingForCityCaputre(true);
            return true;
        }
        break;
        #endif
        default:
            errorOutput("unknown main ident: "+std::to_string(packetCode));
            return false;
        break;
    }
    return true;
}

//have query with reply
bool Client::parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stopIt)
        return false;
    const bool goodQueryBeforeLoginLoaded=
            //before be logged, accept:
            packetCode==0xA0 || //protocol header
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            packetCode==0xA8 || //login by login/pass
            packetCode==0xA9 //Create account
            #else
            packetCode==0x93 //login by token + Select character (Get first data and send the login)
            #endif
            ;
    if(stat==ClientStat::None || goodQueryBeforeLoginLoaded)
        return parseInputBeforeLogin(packetCode,queryNumber,data,size);
    const bool goodQueryBeforeCharacterLoaded=
            //before character selected (but after login), accept:
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            packetCode==0xAA || //Add character
            packetCode==0xAB || //Remove character
            packetCode==0xAC || //Select character
            #endif
            packetCode==0xA1;
    if(stat!=ClientStat::CharacterSelected && !goodQueryBeforeCharacterLoaded)
    {
        errorOutput("charaters is not logged, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+")");
        return false;
    }
    switch(packetCode)
    {
        #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        //Use seed into dirt
        case 0x83:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return;
            }
            #endif
            const uint8_t &plant_id=*data;
            plantSeed(queryNumber,plant_id);
            return;
        }
        break;
        //Collect mature plant
        case 0x84:
            collectPlant(queryNumber);
        break;
        #endif
        //Usage of recipe
        case 0x85:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &recipe_id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            useRecipe(queryNumber,recipe_id);
            return true;
        }
        break;
        //Use object
        case 0x86:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            useObject(queryNumber,objectId);
            return true;
        }
        break;
        //Get shop list
        case 0x87:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            getShopList(queryNumber,shopId);
            return true;
        }
        break;
        //Buy object
        case 0x88:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            buyObject(queryNumber,shopId,objectId,quantity,price);
            return true;
        }
        break;
        //Sell object
        case 0x89:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            sellObject(queryNumber,shopId,objectId,quantity,price);
            return true;
        }
        break;
        //Get factory list
        case 0x8A:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            getFactoryList(queryNumber,factoryId);
            return true;
        }
        break;
        //Buy factory object
        case 0x8B:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            buyFactoryProduct(queryNumber,factoryId,objectId,quantity,price);
            return true;
        }
        break;
        //Sell factory object
        case 0x8C:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            sellFactoryResource(queryNumber,factoryId,objectId,quantity,price);
            return true;
        }
        break;
        //Get market list
        case 0x8D:
            getMarketList(queryNumber);
            return true;
        break;
        //Buy into the market
        case 0x8E:
        {
            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &queryType=data[pos];
            pos+=1;
            switch(queryType)
            {
                case 0x01:
                case 0x02:
                break;
                default:
                    errorOutput("market return type with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            if(queryType==0x01)
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &marketObjectId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                buyMarketObject(queryNumber,marketObjectId,quantity);
            }
            else
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &monsterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                buyMarketMonster(queryNumber,monsterId);
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Put object into the market
        case 0x8F:
        {
            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &queryType=data[pos];
            pos+=1;
            switch(queryType)
            {
                case 0x01:
                case 0x02:
                break;
                default:
                    errorOutput("market return type with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            if(queryType==0x01)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint16_t &objectId=le32toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(double))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                putMarketObject(queryNumber,objectId,quantity,price);
            }
            else
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &monsterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(double))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                putMarketMonster(queryNumber,monsterId,price);
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                                    std::to_string(packetCode)+
                                    ","+
                                    std::to_string(queryNumber)+
                                    "): "+
                                    binarytoHexa(data,pos)+
                                    " "+
                                    binarytoHexa(data+pos,size-pos)
                                    );
                return false;
            }
            return true;
        }
        break;
        //Withdraw cash
        case 0x90:
            withdrawMarketCash(queryNumber);
            return true;
        break;
        //Withdraw object
        case 0x91:
        {
            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &queryType=data[pos];
            pos+=1;
            switch(queryType)
            {
                case 0x01:
                case 0x02:
                break;
                default:
                    errorOutput("market return type with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            if(queryType==0x01)
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &objectId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                withdrawMarketObject(queryNumber,objectId,quantity);
            }
            else
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                    return false;
                }
                const uint32_t &monsterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                withdrawMarketMonster(queryNumber,monsterId);
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Clan action
        case 0x92:
        {
            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &clanActionId=data[pos];
            pos+=1;
            switch(clanActionId)
            {
                case 0x01:
                case 0x04:
                case 0x05:
                {
                    std::string tempString;
                    const uint8_t &textSize=data[pos];
                    pos+=1;
                    if(textSize>0)
                    {
                        if((size-pos)<textSize)
                        {
                            errorOutput("wrong utf8 to std::string size in clan action for text");
                            return false;
                        }
                        tempString=std::string(data+pos,textSize);
                        pos+=textSize;
                    }
                    clanAction(queryNumber,clanActionId,tempString);
                }
                break;
                case 0x02:
                case 0x03:
                    clanAction(queryNumber,clanActionId,std::string());
                break;
                default:
                    errorOutput("unknown clan action code");
                return false;
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Send datapack file list
        case 0xA1:
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        {
            switch(datapackStatus)
            {
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                case DatapackStatus::Base:
                    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
                    {
                        if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
                        {
                            errorOutput("Can't use because mirror is defined");
                            return false;
                        }
                        else
                            datapackStatus=DatapackStatus::Main;
                    }
                break;
                #endif
                case DatapackStatus::Sub:
                    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
                    {
                        errorOutput("CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty()");
                        return false;
                    }
                case DatapackStatus::Main:
                    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
                    {
                        errorOutput("Can't use because mirror is defined");
                        return false;
                    }
                break;
                default:
                    errorOutput("Double datapack send list");
                return false;
            }

            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &stepToSkip=data[pos];
            pos+=1;
            switch(stepToSkip)
            {
                case 0x01:
                    if(datapackStatus==DatapackStatus::Base)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                case 0x02:
                    if(datapackStatus==DatapackStatus::Base)
                        datapackStatus=DatapackStatus::Main;
                    else if(datapackStatus==DatapackStatus::Main)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                case 0x03:
                    if(datapackStatus==DatapackStatus::Base)
                        datapackStatus=DatapackStatus::Sub;
                    else if(datapackStatus==DatapackStatus::Main)
                        datapackStatus=DatapackStatus::Sub;
                    else if(datapackStatus==DatapackStatus::Sub)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                default:
                    errorOutput("step out of range to get datapack: "+std::to_string(stepToSkip));
                return false;
            }

            if((size-pos)<(int)sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint32_t &number_of_file=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            std::vector<std::string> files;
            files.reserve(number_of_file);
            std::vector<uint32_t> partialHashList;
            partialHashList.reserve(number_of_file);
            std::string tempFileName;
            uint32_t index=0;
            while(index<number_of_file)
            {
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        errorOutput("wrong utf8 to std::string size in PM for text size");
                        return false;
                    }
                    const uint8_t &textSize=data[pos];
                    pos+=1;
                    //control the regex file into Client::datapackList()
                    if(textSize>0)
                    {
                        if((size-pos)<textSize)
                        {
                            errorOutput("wrong utf8 to std::string size for file name: parseQuery("+
                                std::to_string(packetCode)+
                                ","+
                                std::to_string(queryNumber)+
                                "): "+
                                binarytoHexa(data,pos)+
                                " "+
                                binarytoHexa(data+pos,size-pos)
                                );
                            return false;
                        }
                        tempFileName=std::string(data+pos,textSize);
                        pos+=textSize;
                    }
                }
                files.push_back(tempFileName);
                index++;
            }
            index=0;
            while(index<number_of_file)
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size for id with main ident: "+
                                          std::to_string(packetCode)+
                                          ", remaining: "+
                                          std::to_string(size-pos)+
                                          ", lower than: "+
                                          std::to_string((int)sizeof(uint32_t))
                                          );
                    return false;
                }
                const uint32_t &partialHash=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                partialHashList.push_back(partialHash);
                index++;
            }
            datapackList(queryNumber,files,partialHashList);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        #else
        errorOutput("CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR");
        return false;
        #endif
        break;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        //Add character
        case 0xAA:
        {
            uint32_t pos=0;
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }

            std::string pseudo;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size)+"");
                return false;
            }
            //const uint8_t &charactersGroupIndex=data[pos];
            pos+=1;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size)+"");
                return false;
            }
            const uint8_t &profileIndex=data[pos];
            pos+=1;
            //pseudo
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    errorOutput("wrong utf8 to std::string size in PM for text size");
                    return false;
                }
                const uint8_t &textSize=data[pos];
                pos+=1;
                if(textSize>0)
                {
                    if(textSize>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
                    {
                        errorOutput("pseudo size is too big: "+std::to_string(pseudo.size())+" because is greater than "+std::to_string(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
                        return false;
                    }
                    if((size-pos)<textSize)
                    {
                        errorOutput("wrong utf8 to std::string size in PM for text");
                        return false;
                    }
                    pseudo=std::string(data+pos,textSize);
                    pos+=textSize;
                }
            }
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("error to get skin with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &monsterGroupId=data[pos];
            pos+=1;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("error to get skin with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &skinId=data[pos];
            pos+=1;
            addCharacter(queryNumber,profileIndex,pseudo,monsterGroupId,skinId);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Remove character
        case 0xAB:
        {
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)+sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            //skip charactersGroupIndex with data+1
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            removeCharacterLater(queryNumber,characterId);
        }
        break;
        //Select character
        case 0xAC:
        {
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            if(stat!=ClientStat::Logged)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)+sizeof(uint32_t)+sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            //skip charactersGroupIndex with data+4+1
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1+4)));
            selectCharacter(queryNumber,characterId);
            return true;
        }
        break;
        #endif
        default:
        errorOutput("no query with only the main code for now, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+")");
        return false;
    }
    return true;
}

//send reply
bool Client::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            size
                            #endif
                            )
{
    queryNumberList.push_back(queryNumber);
    if(stopIt)
        return false;
    if(stat==ClientStat::None)
    {
        disconnectClient();
        return false;
    }
    if(stat!=ClientStat::CharacterSelected)
    {
        errorOutput("is not logged, parseReplyData("+std::to_string(packetCode)+","+std::to_string(queryNumber)+")");
        return false;
    }
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    #endif
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+","+binarytoHexa(data,size)+")");
    #endif
    switch(packetCode)
    {
        //Another player request a trade
        case 0xDF:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the full reply data main ident: "+std::to_string(packetCode)+
                                      ", data: "+binarytoHexa(data,size)
                                      );
                return false;
            }
            #endif
            const uint8_t &returnCode=*data;
            switch(returnCode)
            {
                case 0x01:
                    battleAccepted();
                break;
                case 0x02:
                    battleCanceled();
                break;
                default:
                    errorOutput("ident: "+std::to_string(packetCode)+
                                          ", unknown return code: "+std::to_string(returnCode)
                                          );
                return false;
            }
            return true;
        }
        break;
        //teleportation
        case 0xE1:
            normalOutput("teleportValidatedTo() from protocol");
            teleportValidatedTo(lastTeleportation.front().map,lastTeleportation.front().x,lastTeleportation.front().y,lastTeleportation.front().orientation);
            lastTeleportation.pop();
        break;
        //Event change
        case 0xE2:
            removeFirstEventInQueue();
        break;
        //Another player request a trade
        case 0xE0:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the full reply data main ident: "+
                                      std::to_string(packetCode)+
                                      ", data: "+
                                      binarytoHexa(data,size)
                                      );
                return false;
            }
            #endif
            const uint8_t &returnCode=*data;
            switch(returnCode)
            {
                case 0x01:
                    tradeAccepted();
                break;
                case 0x02:
                    tradeCanceled();
                break;
                default:
                    errorOutput("ident: "+std::to_string(packetCode)+
                                          ", unknown return code: "+std::to_string(returnCode)
                                          );
                return false;
            }
            return true;
        }
        break;
        default:
            errorOutput("unknown main ident: "+std::to_string(packetCode));
            return false;
        break;
    }
    return true;
}

/// \warning it need be complete protocol trame
void Client::fake_receive_data(const std::vector<char> &)
{
//	parseInputAfterLogin(data);
}

void Client::purgeReadBuffer()
{
    parseIncommingData();
}
