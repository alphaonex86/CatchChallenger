#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "../base/BaseServerLogin.h"
#include "../epoll/EpollSocket.h"
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
        int index=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(movePacketKick[index]>movePacketKickTotalCache)
            std::cerr << "movePacketKick[index]>movePacketKickTotalCache" << std::endl;
        else
        #endif
        movePacketKickTotalCache-=movePacketKick[index];
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
            {
                parseNetworkReadError("index out of range <0, movePacketKick");
                return;
            }
            if((index+1)>=CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
            {
                parseNetworkReadError("index out of range >, movePacketKick");
                return;
            }
            if(movePacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("index out of range in array for index "+std::to_string(movePacketKick[index])+", movePacketKick");
                return;
            }
            #endif
            movePacketKick[index]=movePacketKick[index+1];
            index++;
        }
        #else
        memmove(movePacketKick,movePacketKick+1,sizeof(movePacketKick)-1);
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            int index=0;
            int tot=0;
            while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
            {
                tot+=movePacketKick[index];
                index++;
            }
            if(tot!=movePacketKickTotalCache)
                std::cerr << "tot!=movePacketKickTotalCache: cache is wrong" << std::endl;
        }
        #endif
        movePacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=movePacketKickNewValue;
        movePacketKickTotalCache+=movePacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(movePacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
        {
            parseNetworkReadError("bug in DDOS calculation count");
            return;
        }
        #endif
        movePacketKickNewValue=0;
    }
    {
        int index=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKick[index]>chatPacketKickTotalCache)
            std::cerr << "chatPacketKick[index]>chatPacketKickTotalCache" << std::endl;
        else
        #endif
        chatPacketKickTotalCache-=chatPacketKick[index];
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                std::cerr << "index out of range <0, chatPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
                std::cerr << "index out of range >, chatPacketKick" << std::endl;
            if(chatPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
                std::cerr << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            index++;
        }
        #else
        memmove(chatPacketKick,chatPacketKick+1,sizeof(chatPacketKick)-1);
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            int index=0;
            int tot=0;
            while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
            {
                tot+=chatPacketKick[index];
                index++;
            }
            if(tot!=chatPacketKickTotalCache)
                std::cerr << "tot!=chatPacketKickTotalCache: cache is wrong" << std::endl;
        }
        #endif
        chatPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
        #endif
        chatPacketKickNewValue=0;
    }
    {
        int index=0;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKick[index]>otherPacketKickTotalCache)
            std::cerr << "otherPacketKick[index]>otherPacketKickTotalCache" << std::endl;
        else
        #endif
        otherPacketKickTotalCache-=otherPacketKick[index];
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                std::cerr << "index out of range <0, otherPacketKick" << std::endl;
            if((index+1)>=CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
                std::cerr << "index out of range >, otherPacketKick" << std::endl;
            if(otherPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
                std::cerr << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick" << std::endl;
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            index++;
        }
        #else
        memmove(otherPacketKick,otherPacketKick+1,sizeof(otherPacketKick)-1);
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            int index=0;
            int tot=0;
            while(index<(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1))
            {
                tot+=otherPacketKick[index];
                index++;
            }
            if(tot!=otherPacketKickTotalCache)
                std::cerr << "tot!=movePacketKickTotalCache: cache is wrong" << std::endl;
        }
        #endif
        otherPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
            std::cerr << "bug in DDOS calculation count" << std::endl;
        #endif
        otherPacketKickNewValue=0;
    }
}

bool EpollClientLoginSlave::parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    (void)size;
    switch(mainCodeType)
    {
        //Protocol initialization for client
        case 0xA0:
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                stat=EpollClientLoginStat::ProtocolGood;

                stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                /// \todo do the async connect
                /// linkToGameServer->stat=Stat::Connecting;
                const int &socketFd=LinkToGameServer::tryConnect(EpollServerLoginSlave::epollServerLoginSlave->destination_server_ip,EpollServerLoginSlave::epollServerLoginSlave->destination_server_port,5,1);
                if(Q_LIKELY(socketFd>=0))
                {
                    stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                    this->linkToGameServer=linkToGameServer;
                    linkToGameServer->stat=LinkToGameServer::Stat::Connected;
                    linkToGameServer->client=this;
                    linkToGameServer->protocolQueryNumber=queryNumber;
                    //send the protocol
                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                    linkToGameServer->setConnexionSettings();
                    linkToGameServer->parseIncommingData();
                    int s = EpollSocket::make_non_blocking(socketFd);
                    if(s == -1)
                        std::cerr << "unable to make to socket non blocking" << std::endl;
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
            if((movePacketKickTotalCache+movePacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                std::vector<std::string> contentList;
                int index=0;
                while(index<CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
                {
                    contentList.push_back(std::to_string(movePacketKick[index]));
                    index++;
                }
                parseNetworkReadError("Too many move in sort time, check DDOS limit: ("+std::to_string(movePacketKickTotalCache)+"+"+std::to_string(movePacketKickNewValue)+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITMOVE)+", dump: "+stringimplode(contentList,","));
                return false;
            }
            movePacketKickNewValue++;
        break;
        //Chat
        case 0x03:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT)
            {
                std::vector<std::string> contentList;
                int index=0;
                while(index<CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
                {
                    contentList.push_back(std::to_string(chatPacketKick[index]));
                    index++;
                }
                parseNetworkReadError("Too many chat in sort time, check DDOS limit: ("+std::to_string(chatPacketKickTotalCache)+"+"+std::to_string(chatPacketKickNewValue)+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITCHAT)+", dump: "+stringimplode(contentList,","));
                return false;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
            {
                std::vector<std::string> contentList;
                int index=0;
                while(index<CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE)
                {
                    contentList.push_back(std::to_string(otherPacketKick[index]));
                    index++;
                }
                parseNetworkReadError("Too many packet in sort time, check DDOS limit: ("+std::to_string(otherPacketKickTotalCache)+"+"+std::to_string(otherPacketKickNewValue)+")>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT:"+std::to_string(CATCHCHALLENGER_DDOS_KICKLIMITOTHER)+", dump: "+stringimplode(contentList,","));
                return false;
            }
            otherPacketKickNewValue++;
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
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
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
                return true;
            }
            break;
            //Send datapack file list
            case 0xA1:
            {
                uint32_t pos=0;
                #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
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

                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType));
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
                    parseNetworkReadError("wrong size with the main ident: "+std::to_string(mainCodeType));
                    return false;
                }
                const uint32_t &number_of_file=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                std::vector<std::string> files;
                std::vector<uint32_t> partialHashList;
                std::string tempFileName;
                uint32_t index=0;
                while(index<number_of_file)
                {
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseNetworkReadError("wrong utf8 to std::string size in PM for text size");
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
                                                      std::to_string(mainCodeType)+","+
                                                      std::to_string(queryNumber)+
                                                      ")");
                                return false;
                            }
                            tempFileName=std::string(data+pos,textSize);
                            pos+=textSize;
                        }
                    }
                    if((size-pos)<1)
                    {
                        parseNetworkReadError("missing header utf8 datapack file list query");
                        return false;
                    }
                    files.push_back(tempFileName);
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseNetworkReadError("wrong size for id with main ident: "+std::to_string(mainCodeType)+
                                              ", remaining: "+std::to_string(size-pos)+
                                              ", lower than: "+std::to_string((int)sizeof(uint32_t))
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
                    parseNetworkReadError("remaining data: parseQuery("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
                    return false;
                }
                return true;
                #else
                parseNetworkReadError("CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR");
                return false;
                #endif
            }
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
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
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
