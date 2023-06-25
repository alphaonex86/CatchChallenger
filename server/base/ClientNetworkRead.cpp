#include "Client.hpp"
#include "GlobalServerData.hpp"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.hpp"
#endif
#include <cstring>

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_DDOS_FILTER
void Client::doDDOSCompute()
{
    movePacketKick.flush();
    chatPacketKick.flush();
    otherPacketKick.flush();
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
        ProtocolParsingBase::tempBigBufferForOutput[1+1+4+0]=static_cast<uint8_t>(map->id);
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
    if(otherPacketKick.total()>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
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
                            token->value[index]=static_cast<uint8_t>(rand()%256);
                            index++;
                        }
                    }
                    else
                    {
                        const int32_t readedByte=static_cast<int32_t>(fread(token->value,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,BaseServerLogin::fpRandomFile));
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
                switch(CompressionProtocol::compressionTypeServer)
                {
                    case CompressionProtocol::CompressionType::None:
                        *(Client::protocolReplyCompressionNone+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompressionNone+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompressionNone),sizeof(Client::protocolReplyCompressionNone));
                    break;
                    case CompressionProtocol::CompressionType::Zstandard:
                        *(Client::protocolReplyCompresssionZstandard+1)=queryNumber;
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        memcpy(Client::protocolReplyCompresssionZstandard+7,token->value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #endif
                        internalSendRawSmallPacket(reinterpret_cast<char *>(Client::protocolReplyCompresssionZstandard),sizeof(Client::protocolReplyCompresssionZstandard));
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
    if(otherPacketKick.total()>=GlobalServerData::serverSettings.ddos.kickLimitOther)
    {
        errorOutput("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
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
        //Ping for map
        case 0xE3:
        if(pingInProgress>0)
            pingInProgress--;
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
