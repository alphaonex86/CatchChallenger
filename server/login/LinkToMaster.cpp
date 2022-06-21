#include "LinkToMaster.hpp"
#include "EpollClientLoginSlave.hpp"
#include "../epoll/Epoll.hpp"
#include "../epoll/EpollSocket.hpp"
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
#include "CharactersGroupForLogin.hpp"
#include "EpollServerLoginSlave.hpp"

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
        EpollClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Client
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
    reconnectTime=0;
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
    {
        const size_t &size=strlen(host)+1;
        if(size>255)
            memcpy(LinkToMaster::host,host,256);
        else
            memcpy(LinkToMaster::host,host,size);
        LinkToMaster::port=port;
    }
    LinkToMaster::linkToMasterSocketFd=-1;

    const int &socketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd<0)
    {
        std::cerr << "ERROR opening socket to master server at tryConnect() (abort), errno: " << errno << std::endl;
        abort();
    }
    //resolv again the dns
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(host,std::to_string(port).c_str(), &hints, &result);
    if (s != 0) {
        std::cerr << "ERROR connecting to master server server on: " << host << ":" << port << ": " << gai_strerror(s) << std::endl;
        return -1;
    }

    std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... -1" << std::endl;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ..." << std::endl;
        int connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
        std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 0" << std::endl;
        if(connStatusType<0)
        {
            std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 1" << std::endl;
            unsigned int index=0;
            while((considerDownAfterNumberOfTry==0 || (considerDownAfterNumberOfTry>1 && index<considerDownAfterNumberOfTry)) && connStatusType<0)
            {
                std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 2" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
                auto start = std::chrono::high_resolution_clock::now();
                connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end-start;
                if(considerDownAfterNumberOfTry>0)
                    index++;
                if(elapsed.count()<(uint32_t)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
                {
                    const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                }
                std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 3" << std::endl;
            }
            std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 4" << std::endl;
        }
        std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 5" << std::endl;
        if(connStatusType>=0)
        {
            std::cout << "Connected to master server" << std::endl;
            LinkToMaster::linkToMasterSocketFd=sfd;
            haveTheFirstSslHeader=false;
            freeaddrinfo(result);
            return sfd;
        }
        std::cout << "Try connect to master server host: " << host << ", port: " << std::to_string(port) << " ... 6" << std::endl;

        ::close(sfd);
    }
    if (rp == NULL)               /* No address succeeded */
        std::cerr << "ERROR No address succeeded, connecting to master server server on: " << host << ":" << port << std::endl;
    else
        std::cerr << "ERROR connecting to master server server on: " << host << ":" << port << std::endl;
    freeaddrinfo(result);           /* No longer needed */

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
        memset(&event,0,sizeof(event));
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
        std::cerr << "ERROR opening socket to master server at connectInternal (abort), errno: " << errno << std::endl;
        abort();
    }
    EpollClient::reopen(LinkToMaster::linkToMasterSocketFd);

    //resolv the dns if needed
    int connStatusType=tryConnect(host,port,1,1);
    if(connStatusType<0)
    {
        stat=Stat::Unconnected;
        return;
    }
    haveTheFirstSslHeader=false;
    if(connStatusType>=0)
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
    reconnectTime=0;
    sendProtocolHeader();
}

bool LinkToMaster::disconnectClient()
{
    EpollClient::close();
    reset();
    resetForReconnect();
    messageParsingLayer("LinkToMaster::disconnectClient()");

    queryNumberList.clear();
    queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
    for( const auto& n : selectCharacterClients ) {
        const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=n.second;
        EpollClientLoginSlave * client=static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client);
        if(dataForSelectedCharacterReturn.client!=NULL)
        {
            messageParsingLayer("LinkToMaster::disconnectClient(): connected player: "+std::to_string(client->account_id)+" ("+std::to_string(client->stat)+")");
            if(client->stat!=EpollClientLoginSlave::GameServerConnected)
            {
                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                ->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04/*Server internal problem*/);
                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                ->closeSocket();
            }
        }
        else
            messageParsingLayer("connected player NULL");
    }
    selectCharacterClients.clear();

    return true;
}

//input/ouput layer
void LinkToMaster::errorParsingLayer(const std::string &error)
{
    std::cerr << "LinkToMaster::errorParsingLayer: " << error << std::endl;
    disconnectClient();
}

void LinkToMaster::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void LinkToMaster::errorParsingLayer(const char * const error)
{
    std::cerr << "LinkToMaster::errorParsingLayer: " << error << std::endl;
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
        if(reconnectTime<=0)
            reconnectTime=50;
        std::this_thread::sleep_for(std::chrono::milliseconds(reconnectTime));
        reconnectTime+=500;
        if(reconnectTime>600*1000)
            reconnectTime=600*1000;
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
    //messageParsingLayer("try select (0xBE: 190)");
    DataForSelectedCharacterReturn dataForSelectedCharacterReturn;
    dataForSelectedCharacterReturn.client=client;// EpollClientLoginSlave *
    dataForSelectedCharacterReturn.client_query_id=client_query_id;
    dataForSelectedCharacterReturn.serverUniqueKey=serverUniqueKey;
    dataForSelectedCharacterReturn.charactersGroupIndex=charactersGroupIndex;
    dataForSelectedCharacterReturn.start=time(NULL);
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

ssize_t LinkToMaster::read(char * data, const size_t &size)
{
    return EpollClient::read(data,size);
}

ssize_t LinkToMaster::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    return EpollClient::write(data,size);
}

void LinkToMaster::closeSocket()
{
    disconnectClient();
}

void LinkToMaster::detectTimeout()
{
    uint64_t timetemp=time(NULL);
    for( const auto& n : selectCharacterClients ) {
        const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=n.second;
        if(dataForSelectedCharacterReturn.client==nullptr)
        {
            messageParsingLayer("LinkToMaster::detectTimeout(): connected player NULL");
            const uint8_t queryNumber=n.first;
            queryNumberList.push_back(queryNumber);
            selectCharacterClients.erase(queryNumber);
        }
        else
        {
            EpollClientLoginSlave * client=static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client);
            if(dataForSelectedCharacterReturn.start<(timetemp-60))//if started than more 60s
            {
                if(client->stat!=EpollClientLoginSlave::GameServerConnected)
                {
                    messageParsingLayer("LinkToMaster::detectTimeout(): connected player: account_id: "+std::to_string(client->account_id)+" (stat: "+std::to_string(client->stat)+")");
                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                    ->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04/*Server internal problem*/);
                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                    ->closeSocket();
                }
                else
                    messageParsingLayer("LinkToMaster::detectTimeout(): connected player: account_id: "+std::to_string(client->account_id)+", should be removed from this list");
                const uint8_t queryNumber=n.first;
                queryNumberList.push_back(queryNumber);
                selectCharacterClients.erase(queryNumber);
            }
        }
    }
}
