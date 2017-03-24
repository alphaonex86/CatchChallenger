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
#include <cstring>
#include <openssl/sha.h>

using namespace CatchChallenger;

uint16_t LinkToMaster::purgeLockPeriod=3*60;
uint16_t LinkToMaster::maxLockAge=10*60;
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
        stat(Stat::Unconnected),
        tryInterval(5),
        considerDownAfterNumberOfTry(3),
        askMoreMaxClanIdInProgress(false),
        askMoreMaxMonsterIdInProgress(false)
{
    rng.seed(time(0));

    flags|=0x08;
    queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
}

LinkToMaster::~LinkToMaster()
{
    memset(LinkToMaster::private_token,0x00,sizeof(LinkToMaster::private_token));
    LinkToMaster::linkToMasterSocketFd=-1;
    closeSocket();
    //critical bug, need be reopened, never closed
    abort();
}

int LinkToMaster::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
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

    std::cout << "Try connect to master " << host << ":" << port << " ..." << std::endl;
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
            if(elapsed.count()<(uint32_t)tryInterval*1000 && index<considerDownAfterNumberOfTry && connStatusType<0)
            {
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        }
        if(connStatusType<0)
        {
            memset(LinkToMaster::private_token,0x00,sizeof(LinkToMaster::private_token));
            std::cerr << "ERROR connecting to master server (abort)" << std::endl;
            abort();
        }
    }
    std::cout << "Connected to master " << host << ":" << port << std::endl;
    haveTheFirstSslHeader=false;

    return LinkToMaster::linkToMasterSocketFd;
}

void LinkToMaster::setConnexionSettings(const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(tryInterval>0 && tryInterval<60)
        this->tryInterval=tryInterval;
    if(considerDownAfterNumberOfTry>0 && considerDownAfterNumberOfTry<60)
        this->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
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
    setConnexionSettings(this->tryInterval,this->considerDownAfterNumberOfTry);
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
        //abort();//normal for disconnect
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
    /*const int s = EpollSocket::make_non_blocking(LinkToMaster::linkToMasterSocketFd);
    if(s == -1)
    {
        std::cerr << "unable to make to socket non blocking" << std::endl;
        abort();
    }*/
    sendProtocolHeader();
}

void LinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected master link... try connect in loop");
}

//input/ouput layer
void LinkToMaster::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    //critical error, prefer restart from 0
    abort();

    disconnectClient();
}

void LinkToMaster::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
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

bool LinkToMaster::setSettings(TinyXMLSettings * const settings)
{
    //token
    settings->beginGroup("master");
    if(!settings->contains("token"))
        generateToken(settings);
    std::string token=settings->value("token");
    if(token.size()!=TOKEN_SIZE_FOR_MASTERAUTH*2/*String Hexa, not binary*/)
        generateToken(settings);
    token=settings->value("token");
    std::vector<char> binarytoken=hexatoBinary(token);
    if(binarytoken.empty())
    {
        std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
        abort();
    }
    memcpy(LinkToMaster::private_token,binarytoken.data(),binarytoken.size());
    settings->endGroup();

    server_ip=settings->value("server-ip");
    server_port=settings->value("server-port");

    settings->beginGroup("master");
    charactersGroup=settings->value("charactersGroup");

    {
        if(!settings->contains("external-server-ip"))
            settings->setValue("external-server-ip",server_ip);
        externalServerIp=settings->value("external-server-ip");
        if(externalServerIp.empty())
        {
            settings->setValue("external-server-ip",server_ip);
            externalServerIp=settings->value("external-server-ip");
            if(externalServerIp.empty())
            {
                externalServerIp="localhost";
                settings->setValue("external-server-ip",externalServerIp);
            }
        }
    }
    {
        if(!settings->contains("external-server-port"))
            settings->setValue("external-server-port",server_port);
        bool ok;
        externalServerPort=stringtouint16(settings->value("external-server-port"),&ok);
        if(!ok)
            settings->setValue("external-server-port",server_port);
        externalServerPort=stringtouint16(settings->value("external-server-port"),&ok);
        if(!ok)
        {
            externalServerPort = rng()%(65535-8192)+8192;
            settings->setValue("external-server-port",externalServerPort);
        }
    }

    if(!settings->contains("uniqueKey"))
    {
        uniqueKey = rng();
        settings->setValue("uniqueKey",std::to_string(uniqueKey));
    }
    else
    {
        bool ok;
        uniqueKey=stringtouint32(settings->value("uniqueKey"),&ok);
        if(!ok)
        {
            uniqueKey = rng();
            settings->setValue("uniqueKey",std::to_string(uniqueKey));
        }
    }
    logicalGroup=settings->value("logicalGroup");
    settings->endGroup();

    settings->sync();

    return true;
}

void LinkToMaster::generateToken(TinyXMLSettings * const settings)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const int &returnedSize=fread(LinkToMaster::private_token,1,TOKEN_SIZE_FOR_MASTERAUTH,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_MASTERAUTH)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token" << std::endl;
        abort();
    }
    settings->setValue("token",binarytoHexa(reinterpret_cast<char *>(LinkToMaster::private_token),TOKEN_SIZE_FOR_MASTERAUTH));
    fclose(fpRandomFile);
    settings->sync();
}

bool LinkToMaster::registerGameServer(const std::string &exportedXml, const char * const dynamicToken)
{
    if(queryNumberList.empty())
        return false;

    int newSizeCharactersGroup;
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xB2);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xB2;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1+4;

    {
        SHA256_CTX hashToken;
        if(SHA224_Init(&hashToken)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }

        SHA224_Update(&hashToken,reinterpret_cast<const char *>(LinkToMaster::private_token),TOKEN_SIZE_FOR_MASTERAUTH);
        SHA224_Update(&hashToken,dynamicToken,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput),&hashToken);
        posOutput+=CATCHCHALLENGER_SHA224HASH_SIZE;
        //memset(LinkToMaster::private_token,0x00,sizeof(LinkToMaster::private_token));->to reconnect after be disconnected
    }

    //group to find the catchchallenger_common database
    {
        if(!charactersGroup.empty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8WithHeader(charactersGroup,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
        else
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
            newSizeCharactersGroup=1;
        }
        posOutput+=newSizeCharactersGroup;
    }

    //the unique key to save the info by server
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(uniqueKey);
        posOutput+=4;
    }

    //the external information to connect the client or the login server as proxy
    {
        unsigned int newSizeText=FacilityLibGeneral::toUTF8WithHeader(externalServerIp,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
        posOutput+=newSizeText;
    }
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(externalServerPort);
        posOutput+=2;
    }

    //the xml with name and description
    {
        if(!exportedXml.empty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8With16BitsHeader(exportedXml,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
        else
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput+1]=0x00;
            newSizeCharactersGroup=2;
        }
        posOutput+=newSizeCharactersGroup;
    }

    //logical group
    {
        if(!logicalGroup.empty())
            newSizeCharactersGroup=FacilityLibGeneral::toUTF8WithHeader(logicalGroup,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
        else
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
            newSizeCharactersGroup=1;
        }
        posOutput+=newSizeCharactersGroup;
    }

    //current player number and max player
    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(GlobalServerData::serverPrivateVariables.connected_players);
        posOutput+=2;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(GlobalServerData::serverSettings.max_players);
        posOutput+=2;
    }
    else
    {
        if(GlobalServerData::serverPrivateVariables.connected_players<=255)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(0);
        else
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(0);
        posOutput+=2;
        if(GlobalServerData::serverSettings.max_players<=255)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65534);
        else
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65535);
        posOutput+=2;
    }

    //send the connected player
    {
        uint32_t character_count=0;
        unsigned int sizePos=posOutput;
        posOutput+=2;
        unsigned short int index=0;
        while(index<Client::clientBroadCastList.size())
        {
            const uint32_t &character_id=Client::clientBroadCastList.at(index)->getPlayerId();
            if(character_id!=0)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(character_id);
                posOutput+=4;
                character_count++;
            }
            if(index==65535)
                break;
            index++;
        }
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+sizePos)=htole16(character_count);
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    return true;
}

void LinkToMaster::characterDisconnected(const uint32_t &characterId)
{
    /*#ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "unlock the char " << std::to_string(characterId) << std::endl;
    #endif*/
    *reinterpret_cast<uint32_t *>(LinkToMaster::sendDisconnectedPlayer+0x01)=htole32(characterId);
    internalSendRawSmallPacket(LinkToMaster::sendDisconnectedPlayer,sizeof(LinkToMaster::sendDisconnectedPlayer));
}

void LinkToMaster::currentPlayerChange(const uint16_t &currentPlayer)
{
    *reinterpret_cast<uint16_t *>(LinkToMaster::sendCurrentPlayer+0x01)=htole16(currentPlayer);
    internalSendRawSmallPacket(LinkToMaster::sendCurrentPlayer,sizeof(LinkToMaster::sendCurrentPlayer));
}

bool LinkToMaster::askMoreMaxMonsterId()
{
    if(askMoreMaxMonsterIdInProgress)
        return false;
    askMoreMaxMonsterIdInProgress=true;
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xB0);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xB0;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    return true;
}

bool LinkToMaster::askMoreMaxClanId()
{
    if(askMoreMaxClanIdInProgress)
        return false;
    askMoreMaxClanIdInProgress=true;
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xB1);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xB1;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    return true;
}

void LinkToMaster::tryReconnect()
{
    stat=Stat::Unconnected;
    GlobalServerData::serverPrivateVariables.maxMonsterId.clear();
    GlobalServerData::serverPrivateVariables.maxClanId.clear();
    askMoreMaxClanIdInProgress=false;
    askMoreMaxMonsterIdInProgress=false;
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
    }

    if(stat!=Stat::Unconnected)
        return;
    else
    {
        std::cout << "Try reconnect to master..." << std::endl;
        if(tryInterval<=0 || tryInterval>=60)
            this->tryInterval=5;
        if(considerDownAfterNumberOfTry<=0 && considerDownAfterNumberOfTry>=60)
            this->considerDownAfterNumberOfTry=3;
        do
        {
            stat=Stat::Connecting;
            //start to connect
            auto start = std::chrono::high_resolution_clock::now();
            connectInternal();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<(uint32_t)tryInterval*1000 && stat!=Stat::Connected)
            {
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        } while(stat!=Stat::Connected);
        readTheFirstSslHeader();
    }
}

void LinkToMaster::sendProtocolHeader()
{
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xB8);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xB8;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,reinterpret_cast<char *>(LinkToMaster::header_magic_number),sizeof(LinkToMaster::header_magic_number));
    posOutput+=sizeof(LinkToMaster::header_magic_number);

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void LinkToMaster::moveClientFastPath(const uint8_t &,const uint8_t &)
{
}
