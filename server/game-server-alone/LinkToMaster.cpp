#include "LinkToMaster.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/GlobalServerData.h"
#include "../base/Client.h"
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
#include <time.h>
#include <QCryptographicHash>

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
        stat(Stat::Unconnected)
{
    rng.seed(time(0));
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
    LinkToMaster::linkToMasterSocketFd=-1;
    closeSocket();
    //critical bug, need be reopened, never closed
    abort();
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

    std::cout << "Try first connect to master..." << std::endl;
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
            if(elapsed.count()<(quint32)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                const unsigned int ms=(quint32)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
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
        std::cerr << "LoginLinkToMaster::setConnexionSettings() LoginLinkToMaster::linkToMasterSocketFd==-1 (abort)" << std::endl;
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
        /*const int s = EpollSocket::make_non_blocking(LoginLinkToMaster::linkToMasterSocketFd);
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
                if(setsockopt(LoginLinkToMaster::linkToMasterSocketFd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
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
    std::cout << "LoginLinkToMaster::readTheFirstSslHeader()" << std::endl;
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
    messageParsingLayer("Disconnected master link... try connect in loop");
}

//input/ouput layer
void LinkToMaster::errorParsingLayer(const QString &error)
{
    std::cerr << error.toLocal8Bit().constData() << std::endl;
    //critical error, prefer restart from 0
    abort();

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

BaseClassSwitch::EpollObjectType LinkToMaster::getType() const
{
    return BaseClassSwitch::EpollObjectType::MasterLink;
}

void LinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool LinkToMaster::setSettings(QSettings * const settings)
{
    this->settings=settings;

    //token
    settings->beginGroup(QStringLiteral("master"));
    if(!settings->contains(QStringLiteral("token")))
        generateToken();
    QString token=settings->value(QStringLiteral("token")).toString();
    if(token.size()!=TOKEN_SIZE_FOR_MASTERAUTH*2/*String Hexa, not binary*/)
        generateToken();
    token=settings->value(QStringLiteral("token")).toString();
    memcpy(LinkToMaster::private_token,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE_FOR_MASTERAUTH);
    settings->endGroup();

    return true;
}

void LinkToMaster::generateToken()
{
    FILE *fpRandomFile = fopen("/dev/urandom","rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open /dev/urandom to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::private_token,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings->setValue(QStringLiteral("token"),QString(
                          QByteArray(
                              reinterpret_cast<char *>(LinkToMaster::private_token)
                              ,TOKEN_SIZE_FOR_MASTERAUTH)
                          .toHex()));
    fclose(fpRandomFile);
    settings->sync();
}

bool LinkToMaster::registerGameServer(const QString &exportedXml, const char * const dynamicToken)
{
    if(queryNumberList.empty())
        return false;

    int pos=0;
    int newSizeCharactersGroup;
    char tempBuffer[65536*4+1024];

    {
        QCryptographicHash hash(QCryptographicHash::Sha224);
        hash.addData(reinterpret_cast<const char *>(LinkToMaster::private_token),TOKEN_SIZE_FOR_MASTERAUTH);
        hash.addData(dynamicToken,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        const QByteArray &hashedToken=hash.result();
        memcpy(tempBuffer,hashedToken.constData(),hashedToken.size());
        pos+=hashedToken.size();
        memset(LinkToMaster::private_token,0x00,sizeof(LinkToMaster::private_token));
    }

    QString server_ip=settings->value(QLatin1Literal("server-ip")).toString();
    QString server_port=settings->value(QLatin1Literal("server-port")).toString();

    settings->beginGroup(QStringLiteral("master"));
    //group to find the catchchallenger_common database
    {
        if(!settings->value(QLatin1Literal("charactersGroup")).toString().isEmpty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8WithHeader(settings->value(QLatin1Literal("charactersGroup")).toString(),tempBuffer+pos);
        else
        {
            tempBuffer[pos]=0x00;
            newSizeCharactersGroup=1;
        }
        pos+=newSizeCharactersGroup;
    }

    //the unique key to save the info by server
    {
        unsigned int uniqueKey;
        if(!settings->contains(QLatin1Literal("uniqueKey")))
        {
            uniqueKey = rng();
            settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
        }
        else
        {
            bool ok;
            uniqueKey=settings->value(QLatin1Literal("uniqueKey")).toUInt(&ok);
            if(!ok)
            {
                uniqueKey = rng();
                settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
            }
        }
        *reinterpret_cast<quint32 *>(tempBuffer+pos)=htole32(uniqueKey);
        pos+=4;
    }

    //the external information to connect the client or the login server as proxy
    {
        if(!settings->contains(QLatin1Literal("external-server-ip")))
            settings->setValue(QLatin1Literal("external-server-ip"),server_ip);
        QString externalServerIp=settings->value(QLatin1Literal("external-server-ip")).toString();
        if(externalServerIp.isEmpty())
        {
            externalServerIp=QStringLiteral("localhost");
            settings->setValue(QLatin1Literal("external-server-ip"),externalServerIp);
        }

        unsigned int newSizeText=FacilityLibGeneral::toUTF8WithHeader(externalServerIp,tempBuffer+pos);
        pos+=newSizeText;
    }
    {
        unsigned short int externalServerPort;
        if(!settings->contains(QLatin1Literal("external-server-port")))
            settings->setValue(QLatin1Literal("external-server-port"),server_port);
        bool ok;
        externalServerPort=settings->value(QLatin1Literal("external-server-port")).toUInt(&ok);
        if(!ok)
            settings->setValue(QLatin1Literal("external-server-port"),server_port);
        externalServerPort=settings->value(QLatin1Literal("external-server-port")).toUInt(&ok);
        if(!ok)
        {
            externalServerPort = rng()%(65535-8192)+8192;
            settings->setValue(QLatin1Literal("external-server-port"),externalServerPort);
        }
        *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(externalServerPort);
        pos+=2;
    }
    settings->endGroup();

    //the xml with name and description
    {
        if(!exportedXml.isEmpty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8With16BitsHeader(exportedXml,tempBuffer+pos);
        else
        {
            tempBuffer[pos]=0x00;
            newSizeCharactersGroup=2;
        }
        pos+=newSizeCharactersGroup;
    }

    settings->beginGroup(QStringLiteral("master"));
    //logical group
    {
        if(!settings->value(QLatin1Literal("logicalGroup")).toString().isEmpty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8WithHeader(settings->value(QLatin1Literal("logicalGroup")).toString(),tempBuffer+pos);
        else
        {
            tempBuffer[pos]=0x00;
            newSizeCharactersGroup=1;
        }
        pos+=newSizeCharactersGroup;
    }
    settings->endGroup();

    settings->sync();

    //current player number and max player
    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(GlobalServerData::serverPrivateVariables.connected_players);
        pos+=2;
        *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(GlobalServerData::serverSettings.max_players);
        pos+=2;
    }
    else
    {
        if(GlobalServerData::serverPrivateVariables.connected_players<=255)
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(255/2);
        else
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(65535/2);
        pos+=2;
        if(GlobalServerData::serverSettings.max_players<=255)
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(255);
        else
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(65535);
        pos+=2;
    }

    //send the connected player
    {
        quint32 character_count=0;
        unsigned int sizePos=pos;
        pos+=2;
        unsigned short int index=0;
        while(index<Client::clientBroadCastList.size())
        {
            const quint32 &character_id=Client::clientBroadCastList.at(index)->getPlayerId();
            if(character_id!=0)
            {
                *reinterpret_cast<quint32 *>(tempBuffer+pos)=htole32(character_id);
                pos+=4;
                character_count++;
            }
            if(index==65535)
                break;
            index++;
        }
        *reinterpret_cast<quint16 *>(tempBuffer+sizePos)=htole16(character_count);
    }

    packOutcommingQuery(0x07,queryNumberList.back(),tempBuffer,pos);
    queryNumberList.pop_back();
    return true;
}

void LinkToMaster::characterDisconnected(const quint32 &characterId)
{
    *reinterpret_cast<quint32 *>(LinkToMaster::sendDisconnectedPlayer+0x02)=htole32(characterId);
    internalSendRawSmallPacket(LinkToMaster::sendDisconnectedPlayer,sizeof(LinkToMaster::sendDisconnectedPlayer));
}

void LinkToMaster::currentPlayerChange(const quint16 &currentPlayer)
{
    *reinterpret_cast<quint16 *>(LinkToMaster::sendCurrentPlayer+0x02)=htole16(currentPlayer);
    internalSendRawSmallPacket(LinkToMaster::sendCurrentPlayer,sizeof(LinkToMaster::sendCurrentPlayer));
}

void LinkToMaster::askMoreMaxMonsterId()
{
    newFullOutputQuery(0x11,0x07,queryNumberList.back());
    queryNumberList.pop_back();
}

void LinkToMaster::askMoreMaxClanId()
{
    newFullOutputQuery(0x11,0x08,queryNumberList.back());
    queryNumberList.pop_back();
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

void LinkToMaster::sendProtocolHeader()
{
    packOutcommingQuery(0x01,
                        queryNumberList.back(),
                        reinterpret_cast<char *>(LinkToMaster::header_magic_number),
                        sizeof(LinkToMaster::header_magic_number)
                        );
    queryNumberList.pop_back();
}
