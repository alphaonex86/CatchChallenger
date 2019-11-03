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
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Client
            #endif
            ),
        EpollClient(infd),
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
        //linkToGameServer=NULL before closeSocket, else segfault
        client->linkToGameServer=NULL;
        client->closeSocket();
        //break the link
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

    //gethostbyname( deprecated

    const int &socketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd<0)
    {
        std::cerr << "ERROR opening socket to game server server (abort)" << std::endl;
        return -1;
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
        std::cerr << "ERROR connecting to game server server on: " << host << ":" << port << ": " << gai_strerror(s) << std::endl;
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ..." << std::endl;
        int connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
        std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ... 0" << std::endl;
        if(connStatusType<0)
        {
            std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ... 1" << std::endl;
            unsigned int index=0;
            while(index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ... 2" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
                auto start = std::chrono::high_resolution_clock::now();
                connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end-start;
                index++;
                if(elapsed.count()<(uint32_t)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
                {
                    const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                }
                std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ... 3" << std::endl;
            }
            std::cout << "Try connect to game server host: " << host << ", port: " << std::to_string(port) << " ... 4" << std::endl;
        }
        if(connStatusType>=0)
        {
            std::cout << "Connected to game server" << std::endl;
            freeaddrinfo(result);
            return sfd;
        }

        ::close(sfd);
    }
    if (rp == NULL)               /* No address succeeded */
        std::cerr << "ERROR No address succeeded, connecting to game server server on: " << host << ":" << port << std::endl;
    else
        std::cerr << "ERROR connecting to game server server on: " << host << ":" << port << std::endl;
    freeaddrinfo(result);           /* No longer needed */
    return -1;
}

void LinkToGameServer::setConnexionSettings()
{
    if(socketFd==-1)
    {
        std::cerr << "LinkToGameServer::setConnexionSettings() LinkToGameServer::linkToMasterSocketFd==-1 (abort)" << std::endl;
        abort();
    }
    {
        epoll_event event;
        memset(&event,0,sizeof(event));
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

bool LinkToGameServer::disconnectClient()
{
    if(client!=NULL)
    {
        //linkToGameServer=NULL before closeSocket, else segfault
        client->linkToGameServer=NULL;
        client->closeSocket();
        //break the link
        client=NULL;
    }
    EpollClient::close();
    messageParsingLayer("Disconnected client");
    return true;
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
    return BaseClassSwitch::EpollObjectType::GameLink;
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

ssize_t LinkToGameServer::read(char * data, const size_t &size)
{
    return EpollClient::read(data,size);
}

ssize_t LinkToGameServer::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    return EpollClient::write(data,size);
}

void LinkToGameServer::closeSocket()
{
    EpollClient::closeSocket();
}
