#include "LinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include "../epoll/Epoll.h"
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

LinkToMaster *LinkToMaster::linkToMaster=NULL;
int LinkToMaster::linkToMasterSocketFd=-1;
bool LinkToMaster::haveTheFirstSslHeader=false;
sockaddr_in LinkToMaster::serv_addr;

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
    queryNumberList.resize(30);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
}

LinkToMaster::~LinkToMaster()
{
    closeSocket();
}

int LinkToMaster::tryConnect(const char * const host, const quint16 &port,const quint8 &tryInterval,const quint8 &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }

    struct hostent *server;

    LinkToMaster::linkToMasterSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToMaster::linkToMasterSocketFd<0)
    {
        std::cerr << "ERROR opening socket to master server (abort)" << std::endl;
        abort();
    }
    server=gethostbyname(host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to master server (abort)" << std::endl;
        abort();
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    std::cout << "Try connect to master..." << std::endl;
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
            if(elapsed.count()<(quint32)tryInterval*1000 && connStatusType<0)
            {
                const unsigned int ms=(quint32)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            index++;
        }
        if(connStatusType<0)
        {
            std::cerr << "ERROR connecting to master server (abort)" << std::endl;
            abort();
        }
    }
    std::cout << "Connected to master" << std::endl;
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
        std::cerr << "ERROR reading from socket to master server (abort)" << std::endl;
        abort();
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

void LinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LinkToMaster::errorParsingLayer(const QString &error)
{
    std::cerr << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void LinkToMaster::messageParsingLayer(const QString &message) const
{
    std::cout << message.toLocal8Bit().constData() << std::endl;
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

BaseClassSwitch::Type LinkToMaster::getType() const
{
    return BaseClassSwitch::Type::MasterLink;
}

void LinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void LinkToMaster::tryReconnect()
{
    stat=Stat::Unconnected;
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
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        } while(stat!=Stat::Connected);
        readTheFirstSslHeader();
    }
}

bool LinkToMaster::trySelectCharacter(void * const client,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId)
{
    if(queryNumberList.empty())
        return false;
    DataForSelectedCharacterReturn dataForSelectedCharacterReturn;
    dataForSelectedCharacterReturn.client=client;
    dataForSelectedCharacterReturn.client_query_id=client_query_id;
    dataForSelectedCharacterReturn.serverUniqueKey=serverUniqueKey;
    dataForSelectedCharacterReturn.charactersGroupIndex=charactersGroupIndex;
    selectCharacterClients[queryNumberList.back()]=dataForSelectedCharacterReturn;
    //register it
    newFullOutputQuery(0x02,0x07,queryNumberList.back());
    //the data
    EpollClientLoginSlave::selectCharaterRequestOnMaster[0x02]=queryNumberList.back();
    EpollClientLoginSlave::selectCharaterRequestOnMaster[0x03]=charactersGroupIndex;
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::selectCharaterRequestOnMaster+0x04)=serverUniqueKey;
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::selectCharaterRequestOnMaster+0x08)=htole32(characterId);
    *reinterpret_cast<quint32 *>(EpollClientLoginSlave::selectCharaterRequestOnMaster+0x0C)=htole32(static_cast<EpollClientLoginSlave *>(client)->account_id);

    queryNumberList.pop_back();
    return internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::selectCharaterRequestOnMaster),sizeof(EpollClientLoginSlave::selectCharaterRequestOnMaster));
}

void LinkToMaster::sendProtocolHeader()
{
    packOutcommingQuery(0x01,
                        queryNumberList.back(),
                        reinterpret_cast<char *>(LinkToMaster::header_magic_number_and_private_token),
                        sizeof(LinkToMaster::header_magic_number_and_private_token)
                        );
    queryNumberList.pop_back();
}
