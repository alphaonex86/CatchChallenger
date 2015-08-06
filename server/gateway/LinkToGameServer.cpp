#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../epoll/EpollSocket.h"
#include "EpollServerLoginSlave.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"
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

const unsigned char protocolHeaderToMatchLogin[] = PROTOCOL_HEADER_LOGIN;

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
        protocolQueryNumber(0),
        socketFd(infd),
        reply04inWait(NULL),
        reply04inWaitSize(0),
        reply04inWaitQueryNumber(0),
        reply0205inWait(NULL),
        reply0205inWaitSize(0),
        reply0205inWaitQueryNumber(0)
{
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
    if(reply04inWait!=NULL)
    {
        unsigned int index=0;
        while(index<DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.size())
        {
            if(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.at(index)==this)
            {
                DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend[index]=NULL;
                break;
            }
            index++;
        }
        delete reply04inWait;
        reply04inWait=NULL;
    }
    if(reply0205inWait!=NULL)
    {
        //todo
        delete reply0205inWait;
        reply0205inWait=NULL;
    }
    if(DatapackDownloaderBase::datapackDownloaderBase==NULL)
    {
        delete DatapackDownloaderBase::datapackDownloaderBase;
        DatapackDownloaderBase::datapackDownloaderBase=NULL;
    }
}

int LinkToGameServer::tryConnect(const char * const host, const quint16 &port,const quint8 &tryInterval,const quint8 &considerDownAfterNumberOfTry)
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
            if(elapsed.count()<(quint32)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                const unsigned int ms=(quint32)tryInterval*1000-elapsed.count();
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

quint8 LinkToGameServer::freeQueryNumberToServer()
{
    quint8 index=0;
    while(waitedReply_mainCodeType.contains(index))
        index++;
    return index;
}

//input/ouput layer
void LinkToGameServer::errorParsingLayer(const QString &error)
{
    std::cerr << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void LinkToGameServer::messageParsingLayer(const QString &message) const
{
    std::cout << message.toLocal8Bit().constData() << std::endl;
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
    packOutcommingQuery(0x03,protocolQueryNumber/*queryNumber()*/,reinterpret_cast<const char *>(protocolHeaderToMatchLogin),sizeof(protocolHeaderToMatchLogin));
}

void LinkToGameServer::sendDiffered04Reply()
{
    if(client!=NULL)
    {
        parseNetworkReadError("client not connected 04");
        return;
    }
    if(reply04inWait==NULL)
    {
        parseNetworkReadError("LinkToGameServer::sendDiffered04Reply() reply04inWait==NULL");
        return;
    }
    client->postReply(reply04inWaitQueryNumber,reply04inWait,reply04inWaitSize);
    delete reply04inWait;
    reply04inWait=NULL;
    reply04inWaitSize=0;
    reply04inWaitQueryNumber=0;
}

void LinkToGameServer::sendDiffered0205Reply()
{
    if(client!=NULL)
    {
        parseNetworkReadError("client not connected 0205");
        return;
    }
    if(reply0205inWait==NULL)
    {
        parseNetworkReadError("LinkToGameServer::sendDiffered0205Reply() reply0205inWait==NULL");
        return;
    }
    client->postReply(reply0205inWaitQueryNumber,reply0205inWait,reply0205inWaitSize);
    delete reply0205inWait;
    reply0205inWait=NULL;
    reply0205inWaitSize=0;
    reply0205inWaitQueryNumber=0;
}
