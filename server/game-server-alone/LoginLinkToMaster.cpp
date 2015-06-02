#include "LoginLinkToMaster.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/GlobalServerData.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <unistd.h>

using namespace CatchChallenger;

LoginLinkToMaster *LoginLinkToMaster::loginLinkToMaster;

LoginLinkToMaster::LoginLinkToMaster(
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
        stat(Stat::None),
        have_send_protocol_and_registred(false)
{
    queryNumberList.resize(30);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
}

LoginLinkToMaster::~LoginLinkToMaster()
{
    closeSocket();
}

int LoginLinkToMaster::tryConnect(const char * const host, const quint16 &port,const quint8 &tryInterval,const quint8 &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }
    int sockfd,n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0)
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
    int connStatusType=::connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType<0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
            connStatusType=::connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
            index++;
        }
        if(connStatusType<0)
        {
            std::cerr << "ERROR connecting to master server (abort)" << std::endl;
            abort();
        }
    }
    std::cout << "Connected to master" << std::endl;
    char buffer[1];
    n=::read(sockfd,buffer,1);
    if(n<0)
    {
        std::cerr << "ERROR reading from socket to master server (abort)" << std::endl;
        abort();
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
    return sockfd;
}

void LoginLinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LoginLinkToMaster::errorParsingLayer(const QString &error)
{
    std::cerr << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const QString &message) const
{
    std::cout << message.toLocal8Bit().constData() << std::endl;
}

void LoginLinkToMaster::errorParsingLayer(const char * const error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const char * const message) const
{
    std::cout << message << std::endl;
}

BaseClassSwitch::Type LoginLinkToMaster::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void LoginLinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool LoginLinkToMaster::registerGameServer(QSettings * const settings,const QString &exportedXml)
{
    if(queryNumberList.empty())
        return false;

    this->settings=settings;

    int pos=0;
    int newSizeCharactersGroup;
    char tempBuffer[65536*4+1024];
    std::default_random_engine generator;

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
            std::uniform_int_distribution<unsigned int> distribution(0,4000000000);
            uniqueKey = distribution(generator);
            settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
        }
        else
        {
            bool ok;
            uniqueKey=settings->value(QLatin1Literal("uniqueKey")).toUInt(&ok);
            if(!ok)
            {
                std::uniform_int_distribution<unsigned int> distribution(0,4000000000);
                uniqueKey = distribution(generator);
                settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
            }
        }
        *reinterpret_cast<quint32 *>(tempBuffer+pos)=htole32(uniqueKey);
        pos+=4;
    }

    //the external information to connect the client or the login server as proxy
    {
        if(!settings->contains(QLatin1Literal("external-server-ip")))
            settings->setValue(QLatin1Literal("external-server-ip"),settings->value(QLatin1Literal("server-ip")).toString());
        QString externalServerIp=settings->value(QLatin1Literal("external-server-ip")).toString();
        if(externalServerIp.isEmpty())
        {
            externalServerIp=QStringLiteral("localhost");
            settings->setValue(QLatin1Literal("external-server-ip"),externalServerIp);
        }

        newSizeCharactersGroup=FacilityLibGeneral::toUTF8WithHeader(externalServerIp,tempBuffer+pos);
        pos+=newSizeCharactersGroup;
    }
    {
        unsigned short int externalServerPort;
        if(!settings->contains(QLatin1Literal("external-server-port")))
            settings->setValue(QLatin1Literal("external-server-port"),settings->value(QLatin1Literal("server-port")).toString());
        bool ok;
        externalServerPort=settings->value(QLatin1Literal("external-server-port")).toUInt(&ok);
        if(!ok)
        {
            std::uniform_int_distribution<unsigned short int> distribution(8192,65535);
            externalServerPort = distribution(generator);
            settings->setValue(QLatin1Literal("external-server-port"),externalServerPort);
        }
        *reinterpret_cast<quint32 *>(tempBuffer+pos)=htole16(externalServerPort);
        pos+=2;
    }

    //the xml with name and description
    {
        if(!exportedXml.isEmpty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8With16BitsHeader(exportedXml,tempBuffer+pos);
        else
        {
            tempBuffer[pos]=0x00;
            newSizeCharactersGroup=1;
        }
        pos+=newSizeCharactersGroup;
    }

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
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(255);
        else
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(65535);
        pos+=2;
        if(GlobalServerData::serverSettings.max_players<=255)
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(255);
        else
            *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(65535);
        pos+=2;
    }

    //send the disconnected player
    {
        *reinterpret_cast<quint16 *>(tempBuffer+pos)=htole16(disconnectedClientsDatabaseId.size());
        pos+=2;
        unsigned short int index=0;
        while(index<disconnectedClientsDatabaseId.size())
        {
            *reinterpret_cast<quint32 *>(tempBuffer+pos)=htole32(disconnectedClientsDatabaseId.at(index));
            pos+=4;
            if(index==65535)
                break;
            index++;
        }
        disconnectedClientsDatabaseId.clear();
    }

    packOutcommingQuery(0x07,queryNumberList.back(),tempBuffer,pos);
    queryNumberList.pop_back();
    return true;
}

void LoginLinkToMaster::characterDisconnected(const quint32 &characterId)
{
    *reinterpret_cast<quint32 *>(LoginLinkToMaster::sendDisconnectedPlayer+0x02)=htole32(characterId);
    internalSendRawSmallPacket(LoginLinkToMaster::sendDisconnectedPlayer,sizeof(LoginLinkToMaster::sendDisconnectedPlayer));
}

void LoginLinkToMaster::askMoreMaxMonsterId()
{
    newFullOutputQuery(0x11,0x07,queryNumberList.back());
    queryNumberList.pop_back();
}

void LoginLinkToMaster::askMoreMaxClanId()
{
    newFullOutputQuery(0x11,0x08,queryNumberList.back());
    queryNumberList.pop_back();
}
