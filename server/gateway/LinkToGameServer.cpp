#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include "../epoll/Epoll.h"
#include "../epoll/EpollSocket.h"
#include "EpollServerLoginSlave.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"
#include "../../general/base/ProtocolVersion.h"
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

unsigned char protocolHeaderToMatchLogin[] = {0xA0,0x00,0x9c,0xd6,0x49,0x8d,PROTOCOL_HEADER_VERSION};
unsigned char protocolHeaderToMatchGameServer[] = {0xA0,0x00,0x60,0x0c,0xd9,0xbb,PROTOCOL_HEADER_VERSION};

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
        gameServerMode(GameServerMode::None),
        client(NULL),
        haveTheFirstSslHeader(false),
        protocolQueryNumber(0),
        socketFd(infd),
        replySelectListInWait(NULL),
        replySelectListInWaitSize(0),
        replySelectListInWaitQueryNumber(0),
        replySelectCharInWait(NULL),
        replySelectCharInWaitSize(0),
        replySelectCharInWaitQueryNumber(0),
        queryIdToReconnect(0)
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
    if(replySelectListInWait!=NULL)
    {
        vectorremoveOne(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend,this);
        delete replySelectListInWait;
        replySelectListInWait=NULL;
    }
    if(replySelectCharInWait!=NULL)
    {
        if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
        {}
        else if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
        {}
        else
        {
            DatapackDownloaderMainSub * const downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub);
            vectorremoveOne(downloader->clientInSuspend,this);
        }
        delete replySelectCharInWait;
        replySelectCharInWait=NULL;
    }
    if(DatapackDownloaderBase::datapackDownloaderBase==NULL)
    {
        delete DatapackDownloaderBase::datapackDownloaderBase;
        DatapackDownloaderBase::datapackDownloaderBase=NULL;
    }
}

int LinkToGameServer::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }
    if(host==NULL)
    {
        std::cerr << "ERROR LinkToGameServer::tryConnect() host==NULL (abort)" << std::endl;
        //abort();
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
        std::cerr << "ERROR, dns resolution failed on: " << host << ", h_errno: " << std::to_string(h_errno) << " (abort)" << std::endl;
        return -1;
    }
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    //std::cout << "Try connect to game server..." << std::endl;
    int connStatusType=::connect(socketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        {
            const std::string &newConnectErrorMessage=std::string("Unable to connect on game server, retry: ")+std::string(host)+std::string(":")+std::to_string(port)+std::string(", errno: ")+std::to_string(errno);
            if(lastconnectErrorMessage!=newConnectErrorMessage)
            {
                lastconnectErrorMessage=newConnectErrorMessage;
                std::cerr << lastconnectErrorMessage << std::endl;
            }
        }
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
            //std::cerr << "ERROR connecting to game server server (abort) on: " << host << ":" << port << std::endl;
            return -1;
        }
    }
    //std::cout << "Connected to game server" << std::endl;

    return socketFd;
}

void LinkToGameServer::setConnexionSettings()
{
    if(socketFd==-1)
    {
        std::cerr << "linkToGameServer::setConnexionSettings() linkToGameServer::linkToGameServerSocketFd==-1 (abort)" << std::endl;
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
    if(stat!=Stat::Reconnecting)
    {
        stat=Stat::Connected;
        sendProtocolHeader();
    }
    else
        sendProtocolHeaderGameServer();
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
    if(replySelectListInWait!=NULL)
    {
        delete replySelectListInWait;
        replySelectListInWait=NULL;
    }
    if(replySelectCharInWait!=NULL)
    {
        delete replySelectCharInWait;
        replySelectCharInWait=NULL;
    }
    epollSocket.close();
    messageParsingLayer("Disconnected game server");
}

uint8_t LinkToGameServer::freeQueryNumberToServer()
{
    uint8_t index=0;
    while(outputQueryNumberToPacketCode[index]!=0x00)
        index++;
    return index;
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
    {
        /*if(client!=NULL && client->fastForward)
            forwardTo(client);
        else*/
            ProtocolParsingInputOutput::parseIncommingData();
    }
}

void LinkToGameServer::sendProtocolHeader()
{
    registerOutputQuery(protocolQueryNumber,0xA0);
    protocolHeaderToMatchLogin[0x01]=protocolQueryNumber;
    sendRawSmallPacket(reinterpret_cast<const char *>(protocolHeaderToMatchLogin),sizeof(protocolHeaderToMatchLogin));
}

void LinkToGameServer::sendProtocolHeaderGameServer()
{
    registerOutputQuery(protocolQueryNumber,0xA0);
    protocolHeaderToMatchLogin[0x01]=protocolQueryNumber;
    sendRawSmallPacket(reinterpret_cast<const char *>(protocolHeaderToMatchGameServer),sizeof(protocolHeaderToMatchGameServer));
}

void LinkToGameServer::sendDiffered04Reply()
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected 04");
        return;
    }
    if(replySelectListInWait==NULL)
    {
        parseNetworkReadError("LinkToGameServer::sendDiffered04Reply() reply04inWait==NULL");
        return;
    }
    //send the network reply
    client->removeFromQueryReceived(replySelectListInWaitQueryNumber);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=replySelectListInWaitQueryNumber;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(replySelectListInWaitSize);//set the dynamic size

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,replySelectListInWait,replySelectListInWaitSize);
    posOutput+=replySelectListInWaitSize;

    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    delete replySelectListInWait;
    replySelectListInWait=NULL;
    replySelectListInWaitSize=0;
    replySelectListInWaitQueryNumber=0;
}

void LinkToGameServer::sendDiffered0205Reply()
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected 0205");
        return;
    }
    if(replySelectCharInWait==NULL)
    {
        parseNetworkReadError("LinkToGameServer::sendDiffered0205Reply() reply0205inWait==NULL");
        return;
    }

    //send the network reply
    client->removeFromQueryReceived(replySelectCharInWaitQueryNumber);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=replySelectCharInWaitQueryNumber;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(replySelectCharInWaitSize);//set the dynamic size

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,replySelectCharInWait,replySelectCharInWaitSize);
    posOutput+=replySelectCharInWaitSize;

    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    delete replySelectCharInWait;
    replySelectCharInWait=NULL;
    replySelectCharInWaitSize=0;
    replySelectCharInWaitQueryNumber=0;
}

bool LinkToGameServer::sendRawSmallPacket(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

bool LinkToGameServer::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}
