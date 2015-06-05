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
#include <time.h>

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
        stat(Stat::None)
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

bool LoginLinkToMaster::setSettings(QSettings * const settings)
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
    memcpy(LoginLinkToMaster::header_magic_number_and_private_token+9,QByteArray::fromHex(token.toLatin1()).constData(),TOKEN_SIZE_FOR_MASTERAUTH);
    settings->endGroup();

    return true;
}

void LoginLinkToMaster::generateToken()
{
    FILE *fpRandomFile = fopen("/dev/urandom","rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open /dev/urandom to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LoginLinkToMaster::header_magic_number_and_private_token+9,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from /dev/urandom" << std::endl;
        abort();
    }
    settings->setValue(QStringLiteral("token"),QString(
                          QByteArray(
                              reinterpret_cast<char *>(LoginLinkToMaster::header_magic_number_and_private_token)
                              +9,TOKEN_SIZE_FOR_MASTERAUTH)
                          .toHex()));
    fclose(fpRandomFile);
    settings->sync();
}

bool LoginLinkToMaster::registerGameServer(const QString &exportedXml)
{
    if(queryNumberList.empty())
        return false;

    int pos=0;
    int newSizeCharactersGroup;
    char tempBuffer[65536*4+1024];

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

void LoginLinkToMaster::sendProtocolHeader()
{
    packOutcommingQuery(0x01,
                        queryNumberList.back(),
                        reinterpret_cast<char *>(LoginLinkToMaster::header_magic_number_and_private_token),
                        sizeof(LoginLinkToMaster::header_magic_number_and_private_token)
                        );
    queryNumberList.pop_back();
}
