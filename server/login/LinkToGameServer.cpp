#include "LinkToGameServer.hpp"
#include "EventLoopClientLoginSlave.hpp"
#include "../cli/EventLoop.hpp"
#include "EventLoopServerLoginSlave.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include "../../general/base/cpp11addition.hpp"
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <unistd.h>
#include <sys/time.h>

using namespace CatchChallenger;

std::vector<void *> LinkToGameServer::gameLinkToDelete[16];
size_t LinkToGameServer::gameLinkToDeleteSize=0;
uint8_t LinkToGameServer::gameLinkToDeleteIndex=0;
std::unordered_set<void *> LinkToGameServer::detectDuplicateGameLinkToDelete;

LinkToGameServer::LinkToGameServer(
            const int &infd
        ) :
        EventLoopClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Client
            #endif
            ),
        stat(Stat::Connected),
        client(NULL),
        queryIdToReconnect(0),
        socketFd(infd)
{
    memset(&tokenForGameServer,0,sizeof(tokenForGameServer));
    flags|=0x08;
    lastActivity=LinkToGameServer::msFrom1970();
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

uint64_t LinkToGameServer::get_lastActivity() const
{
    return lastActivity;
}

uint64_t LinkToGameServer::msFrom1970() //ms from 1970
{
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec*1000LL + te.tv_usec/1000;
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
                std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
                connStatusType=::connect(sfd, rp->ai_addr, rp->ai_addrlen);
                std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
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
        int s = EventLoop::loop.ctl(EPOLL_CTL_ADD, socketFd, &event);
        if(s == -1)
        {
            std::cerr << "epoll_ctl on socket (master link) error" << std::endl;
            abort();
        }
    }
    /*const int s = SocketUtil::make_non_blocking(socketFd);
    if(s == -1)
    {
        std::cerr << "unable to make to socket non blocking" << std::endl;
        abort();
    }*/
}


bool LinkToGameServer::disconnectClient()
{
    if(detectDuplicateGameLinkToDelete.find(this)==detectDuplicateGameLinkToDelete.cend())
    {
        gameLinkToDelete[gameLinkToDeleteIndex].push_back(this);
        gameLinkToDeleteSize++;
        detectDuplicateGameLinkToDelete.insert(this);
    }
    if(client!=NULL)
    {
        //linkToGameServer=NULL before closeSocket, else segfault
        client->linkToGameServer=NULL;
        client->closeSocket();
        //break the link
        client=NULL;
    }
    EventLoopClient::close();
    messageParsingLayer("Disconnected client");
    return true;
}

//input/ouput layer
void LinkToGameServer::errorParsingLayer(const std::string &error)
{
    std::cerr << sanitizeUtf8String(error) << std::endl;
    disconnectClient();
}

void LinkToGameServer::messageParsingLayer(const std::string &message) const
{
    std::cout << sanitizeUtf8String(message) << std::endl;
}

void LinkToGameServer::errorParsingLayer(const char * const error)
{
    std::cerr << sanitizeUtf8String(std::string(error)) << std::endl;
    disconnectClient();
}

void LinkToGameServer::messageParsingLayer(const char * const message) const
{
    std::cout << sanitizeUtf8String(std::string(message)) << std::endl;
}

BaseClassSwitch::EventLoopObjectType LinkToGameServer::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::GameLink;
}

void LinkToGameServer::parseIncommingData()
{
    lastActivity=LinkToGameServer::msFrom1970();
    // The 1-byte SSL/cleartext preamble that this used to wait on was
    // removed; sendProtocolHeader() runs as soon as the connection is
    // established (see EventLoopClientLoginSlaveProtocolParsing.cpp), so
    // every byte read here belongs to the protocol-input stream.
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
    lastActivity=LinkToGameServer::msFrom1970();
    return internalSendRawSmallPacket(data,size);
}

bool LinkToGameServer::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}

ssize_t LinkToGameServer::readFromSocket(char * data, const size_t &size)
{
    lastActivity=LinkToGameServer::msFrom1970();
    return EventLoopClient::read(data,size);
}

ssize_t LinkToGameServer::writeToSocket(const char * const data, const size_t &size)
{
    lastActivity=LinkToGameServer::msFrom1970();
    //do some basic check on low level protocol (message split, ...)
    return EventLoopClient::write(data,size);
}

void LinkToGameServer::closeSocket()
{
    EventLoopClient::closeSocket();
}
