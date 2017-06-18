#include "LinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../epoll/EpollSocket.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <unistd.h>
#include <string.h>
#include "CharactersGroupForLogin.h"
#include "EpollServerLoginSlave.h"

using namespace CatchChallenger;

LinkToMaster *LinkToMaster::linkToMaster=NULL;
int LinkToMaster::linkToMasterSocketFd=-1;
bool LinkToMaster::haveTheFirstSslHeader=false;
char LinkToMaster::host[];
uint16_t LinkToMaster::port=0;

LinkToMaster::LinkToMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        ) :
        ProtocolParsingInputOutput(
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ,PacketModeTransmission_Client
            #endif
            ),
        stat(Stat::Connected)
{
    flags|=0x08;
    queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
    memset(LinkToMaster::queryNumberToCharacterGroup,0x00,sizeof(LinkToMaster::queryNumberToCharacterGroup));
}

LinkToMaster::~LinkToMaster()
{
    memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
    closeSocket();
}

int LinkToMaster::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }

    //don't cache this
    sockaddr_in serv_addr;
    {
        const size_t &size=strlen(host)+1;
        if(size>255)
            memcpy(LinkToMaster::host,host,256);
        else
            memcpy(LinkToMaster::host,host,size);
        LinkToMaster::port=port;
    }

    LinkToMaster::linkToMasterSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToMaster::linkToMasterSocketFd<0)
    {
        std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
        abort();
    }
    struct hostent *server=gethostbyname(host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to master server (abort)" << std::endl;
        abort();
    }
    //resolv the dns if needed
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    std::cout << "Try connect to master " << host << ":" << port << " ..." << std::endl;
    int connStatusType=::connect(LinkToMaster::linkToMasterSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType<0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
            auto start = std::chrono::high_resolution_clock::now();
            connStatusType=::connect(LinkToMaster::linkToMasterSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            index++;
            if(elapsed.count()<(uint32_t)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        }
        if(connStatusType<0)
        {
            memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));
            std::cerr << "ERROR connecting to master server (abort): " << host << ":" << port << " ..." << std::endl;
            abort();
        }
    }
    std::cout << "Connected to master " << host << ":" << port << std::endl;
    haveTheFirstSslHeader=false;

    return LinkToMaster::linkToMasterSocketFd;
}

void LinkToMaster::setConnexionSettings()
{
    if(LinkToMaster::linkToMasterSocketFd==-1)
    {
        std::cerr << "linkToMaster::setConnexionSettings() linkToMaster::linkToMasterSocketFd==-1 (abort)" << std::endl;
        abort();
    }
    {
        epoll_event event;
        event.data.ptr = LinkToMaster::linkToMaster;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
        int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, LinkToMaster::linkToMasterSocketFd, &event);
        if(s == -1)
        {
            std::cerr << "epoll_ctl on socket (master link) error" << std::endl;
            abort();
        }
    }
    {
        /*const int s = EpollSocket::make_non_blocking(linkToMaster::linkToMasterSocketFd);
        if(s == -1)
        {
            std::cerr << "unable to make to socket non blocking" << std::endl;
            abort();
        }
        else*/
        {
            //if(tcpCork)
            {
                //set cork for CatchChallener because don't have real time part
                int state = 1;
                if(setsockopt(LinkToMaster::linkToMasterSocketFd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                {
                    std::cerr << "Unable to apply tcp cork" << std::endl;
                    abort();
                }
            }
            /*else if(tcpNodelay)
            {
                //set no delay to don't try group the packet and improve the performance
                int state = 1;
                if(setsockopt(linkToMaster::linkToMasterSocketFd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                {
                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                    abort();
                }
            }*/
        }
    }
}

void LinkToMaster::connectInternal()
{
    LinkToMaster::linkToMasterSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToMaster::linkToMasterSocketFd<0)
    {
        std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
        abort();
    }
    epollSocket.reopen(LinkToMaster::linkToMasterSocketFd);

    struct hostent *server=gethostbyname(host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to master server (abort)" << std::endl;
        abort();
    }

    //resolv the dns if needed
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    int connStatusType=::connect(LinkToMaster::linkToMasterSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        stat=Stat::Unconnected;
        return;
    }
    haveTheFirstSslHeader=false;
    if(connStatusType==0)
    {
        stat=Stat::Connected;
        std::cout << "(Re)Connected to master" << std::endl;
    }
    else
    {
        stat=Stat::Connecting;
        std::cout << "(Re)Connecting in progress to master" << std::endl;
    }
    setConnexionSettings();
}

void LinkToMaster::readTheFirstSslHeader()
{
    if(haveTheFirstSslHeader)
        return;
    std::cout << "linkToMaster::readTheFirstSslHeader()" << std::endl;
    char buffer[1];
    if(::read(LinkToMaster::linkToMasterSocketFd,buffer,1)<0)
    {
        std::cerr << "ERROR reading from socket to master server (abort): " << std::to_string(errno) << ", LinkToMaster::linkToMasterSocketFd: " << std::to_string(LinkToMaster::linkToMasterSocketFd) << std::endl;
        //abort();
        return;
    }
    #ifdef SERVERSSL
    if(buffer[0]!=0x01)
    {
        std::cerr << "ERROR server configured in ssl mode but protocol not done" << std::endl;
        abort();
    }
    #else
    if(buffer[0]!=0x00)
    {
        std::cerr << "ERROR server configured in clear mode but protocol not done" << std::endl;
        abort();
    }
    #endif
    haveTheFirstSslHeader=true;
    stat=Stat::Connected;
    EpollSocket::make_non_blocking(LinkToMaster::linkToMasterSocketFd);
    sendProtocolHeader();
}

void LinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LinkToMaster::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToMaster::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void LinkToMaster::errorParsingLayer(const char * const error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToMaster::messageParsingLayer(const char * const message) const
{
    std::cout << message << std::endl;
}

BaseClassSwitch::EpollObjectType LinkToMaster::getType() const
{
    return BaseClassSwitch::EpollObjectType::MasterLink;
}

void LinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void LinkToMaster::tryReconnect()
{
    {
        //abort all pending request asked to master server
        //as selectCharacter():
        /// \todo abort selectCharacter() pending
    }
    stat=Stat::Unconnected;
    EpollClientLoginSlave::maxAccountIdList.clear();
    {
        unsigned int index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin * const group=CharactersGroupForLogin::list.at(index);
            group->maxCharacterId.clear();
            group->maxMonsterId.clear();
            group->maxCharacterIdRequested=false;
            group->maxMonsterIdRequested=false;
            index++;
        }
    }
    //same than base contructor
    {
        resetForReconnect();
    }
    //same as contructor
    {
        flags|=0x08;
        queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
        unsigned int index=0;
        while(index<queryNumberList.size())
        {
            queryNumberList[index]=index;
            index++;
        }
        memset(LinkToMaster::queryNumberToCharacterGroup,0x00,sizeof(LinkToMaster::queryNumberToCharacterGroup));
    }
    EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.clear();

    if(stat!=Stat::Unconnected)
        return;
    else
    {
        std::cout << "Try reconnect to master..." << std::endl;
        do
        {
            stat=Stat::Connecting;
            //start to connect
            auto start = std::chrono::high_resolution_clock::now();
            connectInternal();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<5000 && stat!=Stat::Connected)
            {
                const unsigned int ms=5000-elapsed.count();
                if(ms>5000)
                    std::this_thread::sleep_for(std::chrono::seconds(ms));
            }
        } while(stat!=Stat::Connected);
        readTheFirstSslHeader();
    }
}

bool LinkToMaster::trySelectCharacter(void * const client,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId)
{
    //todo: cache the user cache to locally double lock check to minimize the master
    if(queryNumberList.empty())
    {
        errorParsingLayer("EpollClientLoginSlave::trySelectCharacter() out of query to request the master server: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        std::cerr << listTheRunningQuery() << std::endl;
        return false;
    }
    DataForSelectedCharacterReturn dataForSelectedCharacterReturn;
    dataForSelectedCharacterReturn.client=client;
    dataForSelectedCharacterReturn.client_query_id=client_query_id;
    dataForSelectedCharacterReturn.serverUniqueKey=serverUniqueKey;
    dataForSelectedCharacterReturn.charactersGroupIndex=charactersGroupIndex;
    selectCharacterClients[queryNumberList.back()]=dataForSelectedCharacterReturn;
    //register it
    registerOutputQuery(queryNumberList.back(),0xBE);
    //the data
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xBE;
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumberList.back();
    ProtocolParsingBase::tempBigBufferForOutput[0x02]=charactersGroupIndex;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+0x03)=serverUniqueKey;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+0x07)=htole32(characterId);
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+0x0B)=htole32(static_cast<EpollClientLoginSlave *>(client)->account_id);

    queryNumberList.pop_back();
    return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,(/*header*/1+1)+1+4+4+4);
}

void LinkToMaster::sendProtocolHeader()
{
    if(queryNumberList.empty())
    {
        std::cerr << listTheRunningQuery() << std::endl;
        return;
    }

    //register it
    registerOutputQuery(queryNumberList.back(),0xB8);

    LinkToMaster::header_magic_number[0x01]=queryNumberList.back();
    internalSendRawSmallPacket(reinterpret_cast<char *>(LinkToMaster::header_magic_number),sizeof(LinkToMaster::header_magic_number));
    queryNumberList.pop_back();
}

bool LinkToMaster::sendRawBlock(const char * const data,const unsigned int &size)
{
    return internalSendRawSmallPacket(data,size);
}

std::string LinkToMaster::listTheRunningQuery() const
{
    std::string returnVar;
    uint8_t index=0;
    while(index<sizeof(outputQueryNumberToPacketCode))
    {
        if(outputQueryNumberToPacketCode[index]!=0x00)
        {
            if(!returnVar.empty())
                returnVar+=",";
            returnVar+="["+std::to_string(index)+"]="+std::to_string(outputQueryNumberToPacketCode[index]);
        }
        index++;
    }
    if(!returnVar.empty())
        returnVar="running query: "+returnVar;
    else
        returnVar="no running query";
    return returnVar;
}
