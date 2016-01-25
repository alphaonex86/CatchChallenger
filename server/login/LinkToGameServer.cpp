#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../epoll/EpollSocket.h"
#include "EpollServerLoginSlave.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <unistd.h>

using namespace CatchChallenger;

LinkToGameServer::LinkToGameServer(
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
        stat(Stat::Connected),
        client(NULL),
        haveTheFirstSslHeader(false),
        queryIdToReconnect(0),
        socketFd(infd)
{
    flags|=0x08;
}

LinkToGameServer::~LinkToGameServer()
{
    if(client!=NULL)
    {
        client->closeSocket();
        //break the link
        client->linkToGameServer=NULL;
        client=NULL;
    }
}

int LinkToGameServer::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }

    struct hostent *server;

    const int &socketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd<0)
    {
        std::cerr << "ERROR opening socket to game server server (abort)" << std::endl;
        abort();
    }
    //resolv again the dns
    server=gethostbyname(host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to game server server (abort)" << std::endl;
        abort();
    }
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    std::cout << "Try connect to game server..." << std::endl;
    int connStatusType=::connect(socketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType<0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
            auto start = std::chrono::high_resolution_clock::now();
            connStatusType=::connect(socketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
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
            std::cerr << "ERROR connecting to game server server (abort)" << std::endl;
            abort();
        }
    }
    std::cout << "Connected to game server" << std::endl;

    return socketFd;
}

void LinkToGameServer::setConnexionSettings()
{
    if(socketFd==-1)
    {
        std::cerr << "linkToMaster::setConnexionSettings() linkToMaster::linkToMasterSocketFd==-1 (abort)" << std::endl;
        abort();
    }
    {
        epoll_event event;
        event.data.ptr = this;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
        int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, socketFd, &event);
        if(s == -1)
        {
            std::cerr << "epoll_ctl on socket (master link) error" << std::endl;
            abort();
        }
    }
    {
        if(EpollServerLoginSlave::epollServerLoginSlave->tcpCork)
        {
            //set cork for CatchChallener because don't have real time part
            int state = 1;
            if(setsockopt(socketFd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
            {
                std::cerr << "Unable to apply tcp cork" << std::endl;
                abort();
            }
        }
        else if(EpollServerLoginSlave::epollServerLoginSlave->tcpNodelay)
        {
            //set no delay to don't try group the packet and improve the performance
            int state = 1;
            if(setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
            {
                std::cerr << "Unable to apply tcp no delay" << std::endl;
                abort();
            }
        }
    }
    /*const int s = EpollSocket::make_non_blocking(socketFd);
    if(s == -1)
    {
        std::cerr << "unable to make to socket non blocking" << std::endl;
        abort();
    }*/
}

void LinkToGameServer::readTheFirstSslHeader()
{
    if(haveTheFirstSslHeader)
        return;
    std::cout << "linkToMaster::readTheFirstSslHeader()" << std::endl;
    char buffer[1];
    const ssize_t &size=::read(socketFd,buffer,1);
    if(size<0)
    {
        std::cerr << "ERROR reading from socket to game server server, errno " << errno << std::endl;
        if(errno!=EAGAIN)
            closeSocket();
        return;
    }
    if(size<1)
    {
        std::cerr << "ERROR reading from socket to game server server, wait more data" << std::endl;
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
    sendProtocolHeader();
}

void LinkToGameServer::disconnectClient()
{
    if(client!=NULL)
    {
        client->closeSocket();
        //break the link
        client->linkToGameServer=NULL;
        client=NULL;
    }
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LinkToGameServer::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToGameServer::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void LinkToGameServer::errorParsingLayer(const char * const error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToGameServer::messageParsingLayer(const char * const message) const
{
    std::cout << message << std::endl;
}

BaseClassSwitch::EpollObjectType LinkToGameServer::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void LinkToGameServer::parseIncommingData()
{
    if(!haveTheFirstSslHeader)
        readTheFirstSslHeader();
    if(haveTheFirstSslHeader)
        ProtocolParsingInputOutput::parseIncommingData();
}

void LinkToGameServer::sendProtocolHeader()
{
    //send the network query
    registerOutputQuery(0x01,0xA0);
    sendRawBlock(reinterpret_cast<char *>(protocolHeaderToMatchGameServer),sizeof(protocolHeaderToMatchGameServer));
}

bool LinkToGameServer::sendRawBlock(const char * const data,const unsigned int &size)
{
    return internalSendRawSmallPacket(data,size);
}

bool LinkToGameServer::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}
