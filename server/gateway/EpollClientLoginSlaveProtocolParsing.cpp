#include "EpollClientLoginSlave.hpp"
#include "EpollServerLoginSlave.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <iostream>
#include <cstring>

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
    if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
    (void)size;
    switch(mainCodeType)
    {
        //Protocol initialization for client
        case 0xA0:
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                /// \todo do the async connect
                /// linkToGameServer->stat=Stat::Connecting;
                int socketFd=-1;
                if(strlen(EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_ip)<=0)
                    socketFd=LinkToGameServer::tryConnect(EpollServerLoginSlave::epollServerLoginSlave->destination_server_ip.c_str(),EpollServerLoginSlave::epollServerLoginSlave->destination_server_port,5,1);
                else
                    socketFd=LinkToGameServer::tryConnect(EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_ip,EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_port,5,1);
                if(socketFd>=0)
                {
                    stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                    this->linkToGameServer=linkToGameServer;
                    if(strlen(EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_ip)<=0)
                        linkToGameServer->stat=LinkToGameServer::Stat::WaitingFirstSslHeader;
                    else
                        linkToGameServer->stat=LinkToGameServer::Stat::WaitingProxy;
                    linkToGameServer->client=this;
                    linkToGameServer->protocolQueryNumber=queryNumber;
                    //send the protocol
                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                    linkToGameServer->setConnexionSettings();
                    if(strlen(EpollServerLoginSlave::epollServerLoginSlave->destination_proxy_ip)<=0)
                        linkToGameServer->parseIncommingData();
                    else
                        linkToGameServer->sendProxyRequest(EpollServerLoginSlave::epollServerLoginSlave->destination_server_ip,EpollServerLoginSlave::epollServerLoginSlave->destination_server_port);
                    /*int s = EpollSocket::make_non_blocking(socketFd);
                    if(s == -1)
                        std::cerr << "unable to make to socket non blocking" << std::endl;*/
                }
                else
                {
                    //parseNetworkReadError("not able to connect on the game server as gateway, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
                    //Message done by LinkToGameServer::tryConnect()
                    disconnectClient();
                    return false;
                }

                return true;
            }
            else
            {
                parseNetworkReadError("Wrong protocol");
                return false;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType)+", stat: "+std::to_string(stat));
            return false;
        break;
    }
    return true;
}

bool EpollClientLoginSlave::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if(linkToGameServer==NULL)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }
    switch(mainCodeType)
    {
        //Send position + direction
        case 0x02:
            if(movePacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many move in sort time, check DDOS limit: ("+std::to_string(otherPacketKick.total())+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITMOVE));
                return false;
            }
            movePacketKick.incrementLastValue();
        break;
        //Chat
        case 0x03:
            if(chatPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT)
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

            return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

            return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
    if(otherPacketKick.total()>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKick.incrementLastValue();
    (void)data;
    if(linkToGameServer==NULL && mainCodeType!=0xA0)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }



    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        switch(mainCodeType)
        {
            //Select the character
            case 0xAC:
            if(linkToGameServer->gameServerMode==LinkToGameServer::GameServerMode::Reconnect)
            {
                if(size!=(1+4+4))
                {
                    parseNetworkReadError("Select character: wrong size");
                    return false;
                }
                const uint8_t &charactersGroupIndex=data[0x00];
                const uint32_t &uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
                if(linkToGameServer->serverReconnectList.find(charactersGroupIndex)==linkToGameServer->serverReconnectList.cend())
                {
                    parseNetworkReadError("Select character: charactersGroupIndex not found");
                    return false;
                }
                if(linkToGameServer->serverReconnectList.at(charactersGroupIndex).find(uniqueKey)==linkToGameServer->serverReconnectList.at(charactersGroupIndex).cend())
                {
                    parseNetworkReadError("Select character: unique key not found");
                    return false;
                }
                linkToGameServer->selectedServer=linkToGameServer->serverReconnectList.at(charactersGroupIndex).at(uniqueKey);
                linkToGameServer->serverReconnectList.clear();
                //return true; -> why?
            }
            break;
            //Send datapack file list
            case 0xA1:
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            {
                switch(datapackStatus)
                {
                    case DatapackStatus::Base:
                        if(LinkToGameServer::httpDatapackMirrorRewriteBase.size()>1)
                        {
                            if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()>1)
                            {
                                parseNetworkReadError("Can't use because mirror is defined");
                                return false;
                            }
                            else
                                datapackStatus=DatapackStatus::Main;
                        }
                    break;
                    case DatapackStatus::Sub:
                        if(linkToGameServer->sub.empty())
                        {
                            parseNetworkReadError("linkToGameServer->sub.empty()");
                            return false;
                        }
                    [[gnu::fallthrough]];
                    case DatapackStatus::Main:
                        if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()>1)
                        {
                            parseNetworkReadError("Can't use because mirror is defined");
                            return false;
                        }
                    break;
                    default:
                        parseNetworkReadError("Double datapack send list");
                    return false;
                }

                uint32_t pos=0;
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
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
                            parseNetworkReadError("step out of range to get datapack base but already in highter level");
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
                            parseNetworkReadError("step out of range to get datapack base but already in highter level");
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
                            parseNetworkReadError("step out of range to get datapack base but already in highter level");
                            return false;
                        }
                    break;
                    default:
                        parseNetworkReadError("step out of range to get datapack: "+std::to_string(stepToSkip));
                    return false;
                }

                if((size-pos)<(int)sizeof(uint32_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType)+", data: "+binarytoHexa(data,size));
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
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);
                #endif
                if(number_of_file>1000000)
                {
                    parseNetworkReadError("number_of_file > million: "+std::to_string(number_of_file)+": parseQuery("+
                        std::to_string(mainCodeType)+
                        ","+
                        std::to_string(queryNumber)+
                        "): "+
                        binarytoHexa(data,pos)+
                        " "+
                        binarytoHexa(data+pos,size-pos)
                        );
                    return false;
                }
                while(index<number_of_file)
                {
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseNetworkReadError("wrong utf8 to std::string size "+std::to_string(size)+" pos "+std::to_string(pos)+": parseQuery("+
                                std::to_string(mainCodeType)+
                                ","+
                                std::to_string(queryNumber)+
                                "): "+
                                binarytoHexa(data,pos)+
                                " "+
                                binarytoHexa(data+pos,size-pos)
                                );
                            return false;
                        }
                        const uint8_t &textSize=data[pos];
                        pos+=1;
                        //control the regex file into Client::datapackList()
                        if(textSize>0)
                        {
                            if((size-pos)<textSize)
                            {
                                parseNetworkReadError("wrong utf8 to std::string size for file name: parseQuery("+
                                    std::to_string(mainCodeType)+
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
                        else
                            tempFileName.clear();
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(!std::regex_match(tempFileName,datapack_rightFileName))
                        {
                            parseNetworkReadError("wrong file name \""+tempFileName+"\": parseQuery("+
                                std::to_string(mainCodeType)+
                                ","+
                                std::to_string(queryNumber)+
                                "): "+
                                binarytoHexa(data,pos)+
                                " "+
                                binarytoHexa(data+pos,size-pos)
                                );
                            return false;
                        }
                        #endif
                    }
                    files.push_back(tempFileName);
                    index++;
                }
                index=0;
                while(index<number_of_file)
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseNetworkReadError("wrong size for id with main ident: "+
                                              std::to_string(mainCodeType)+
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
                    parseNetworkReadError("remaining data: parseQuery("+
                        std::to_string(mainCodeType)+
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
            parseNetworkReadError("CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR");
            return false;
            #endif
            break;
            default:
            break;
        }

        linkToGameServer->registerOutputQuery(queryNumber,mainCodeType);
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

            return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

            return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
    }

    return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
}

//send reply
bool EpollClientLoginSlave::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
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
    if(linkToGameServer==NULL)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }
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

                return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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

                return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+std::to_string(mainCodeType));
            return false;
        }
    }
    //queryNumberList << queryNumber;
    (void)data;
    (void)size;
    parseNetworkReadError("The server for now not ask anything: "+std::to_string(mainCodeType)+", "+std::to_string(queryNumber));
    return false;
}

void EpollClientLoginSlave::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
