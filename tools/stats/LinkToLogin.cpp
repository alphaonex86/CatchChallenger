#include "LinkToLogin.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/cpp11addition.h"
#include "../../server/epoll/Epoll.h"
#include "../../server/epoll/EpollSocket.h"
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

#ifndef STATSODROIDSHOW2
LinkToLogin *LinkToLogin::linkToLogin=NULL;
#endif
int LinkToLogin::linkToLoginSocketFd=-1;
bool LinkToLogin::haveTheFirstSslHeader=false;
char LinkToLogin::host[]="localhost";
uint16_t LinkToLogin::port=22222;

LinkToLogin::LinkToLogin(
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
        considerDownAfterNumberOfTry(3)
{
    flags|=0x08;
    rng.seed(time(0));
    queryNumberList.resize(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    unsigned int index=0;
    while(index<queryNumberList.size())
    {
        queryNumberList[index]=index;
        index++;
    }
    warningLogicalGroupOutOfBound=false;
}

LinkToLogin::~LinkToLogin()
{
    LinkToLogin::linkToLoginSocketFd=-1;
    closeSocket();
    //critical bug, need be reopened, never closed
    abort();
}

void LinkToLogin::displayErrorAndQuit(const char * errorString)
{
    #ifndef STATSODROIDSHOW2
    removeJsonFile();
    #else
    LinkToLogin::writeData("\ec\e[2s\e[1r\e[31m");
    LinkToLogin::writeData(errorString);
    #endif
    std::cerr << errorString << std::endl;
    abort();
}

const std::string &LinkToLogin::getJsonFileContent() const
{
    return jsonFileContent;
}

int LinkToLogin::tryConnect(const char * const host, const uint16_t &port,const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(port==0)
    {
        std::cerr << "ERROR port is 0 (abort)" << std::endl;
        abort();
    }

    strcpy(LinkToLogin::host,host);
    LinkToLogin::port=port;

    LinkToLogin::linkToLoginSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToLogin::linkToLoginSocketFd<0)
    {
        std::cerr << "ERROR opening socket to login server (abort)" << std::endl;
        abort();
    }
    struct hostent *server;
    server=gethostbyname(host);
    if(server==NULL)
        displayErrorAndQuit("ERROR, no such host to login server (abort)");
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    std::cout << "Try connect to login " << host << ":" << port << " ..." << std::endl;
    int connStatusType=::connect(LinkToLogin::linkToLoginSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType<0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(tryInterval));
            auto start = std::chrono::high_resolution_clock::now();
            connStatusType=::connect(LinkToLogin::linkToLoginSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
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
            displayErrorAndQuit("ERROR connecting to login server (abort)");
    }
    std::cout << "Connected to login " << host << ":" << port << std::endl;
    haveTheFirstSslHeader=false;

    return LinkToLogin::linkToLoginSocketFd;
}

void LinkToLogin::setConnexionSettings(const uint8_t &tryInterval,const uint8_t &considerDownAfterNumberOfTry)
{
    if(tryInterval>0 && tryInterval<60)
        this->tryInterval=tryInterval;
    if(considerDownAfterNumberOfTry>0 && considerDownAfterNumberOfTry<60)
        this->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    if(LinkToLogin::linkToLoginSocketFd==-1)
    {
        std::cerr << "LoginLinkToLogin::setConnexionSettings() LoginLinkToLogin::linkToLoginSocketFd==-1 (abort)" << std::endl;
        abort();
    }
    {
        epoll_event event;
        event.data.ptr = this;
        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
        int s = Epoll::epoll.ctl(EPOLL_CTL_ADD, LinkToLogin::linkToLoginSocketFd, &event);
        if(s == -1)
        {
            std::cerr << "epoll_ctl on socket (login link) error" << std::endl;
            abort();
        }
    }
    {
        /*const int s = EpollSocket::make_non_blocking(LoginLinkToLogin::linkToLoginSocketFd);
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
                if(setsockopt(LinkToLogin::linkToLoginSocketFd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                {
                    std::cerr << "Unable to apply tcp cork" << std::endl;
                    abort();
                }
            }
            /*else if(tcpNodelay)
            {
                //set no delay to don't try group the packet and improve the performance
                int state = 1;
                if(setsockopt(LoginLinkToLogin::linkToLoginSocketFd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                {
                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                    abort();
                }
            }*/
        }
    }
}

void LinkToLogin::connectInternal()
{
    LinkToLogin::linkToLoginSocketFd=socket(AF_INET, SOCK_STREAM, 0);
    if(LinkToLogin::linkToLoginSocketFd<0)
    {
        std::cerr << "ERROR opening socket to login server (abort)" << std::endl;
        abort();
    }
    epollSocket.reopen(LinkToLogin::linkToLoginSocketFd);

    struct hostent *server;
    server=gethostbyname(LinkToLogin::host);
    if(server==NULL)
    {
        std::cerr << "ERROR, no such host to login server (abort)" << std::endl;
        abort();
    }
    sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(LinkToLogin::port);
    int connStatusType=::connect(LinkToLogin::linkToLoginSocketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(connStatusType<0)
    {
        stat=Stat::Unconnected;
        return;
    }
    haveTheFirstSslHeader=false;
    if(connStatusType==0)
    {
        stat=Stat::Connected;
        //std::cout << "(Re)Connected to login" << std::endl;
    }
    else
    {
        stat=Stat::Connecting;
        //std::cout << "(Re)Connecting in progress to login" << std::endl;
    }
    setConnexionSettings(this->tryInterval,this->considerDownAfterNumberOfTry);
}

void LinkToLogin::readTheFirstSslHeader()
{
    if(haveTheFirstSslHeader)
        return;
    //std::cout << "LoginLinkToLogin::readTheFirstSslHeader()" << std::endl;
    char buffer[1];
    if(::read(LinkToLogin::linkToLoginSocketFd,buffer,1)<0)
    {
        //reconnect, need maybe more time
        EpollSocket::make_non_blocking(LinkToLogin::linkToLoginSocketFd);
        /*then disable:
        jsonFileContent.clear();
        displayErrorAndQuit("ERROR reading from socket to login server (abort)");*/
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
    EpollSocket::make_non_blocking(LinkToLogin::linkToLoginSocketFd);
    sendProtocolHeader();
}

bool LinkToLogin::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected login link... try connect in loop");
    return true;
}

//input/ouput layer
void LinkToLogin::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    //critical error, prefer restart from 0
    abort();

    disconnectClient();
}

void LinkToLogin::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void LinkToLogin::errorParsingLayer(const char * const error)
{
    std::cerr << error << std::endl;
    disconnectClient();
}

void LinkToLogin::messageParsingLayer(const char * const message) const
{
    std::cout << message << std::endl;
}

BaseClassSwitch::EpollObjectType LinkToLogin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void LinkToLogin::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool LinkToLogin::registerStatsClient(const char * const dynamicToken)
{
    if(queryNumberList.empty())
        return false;

    //send the network query
    registerOutputQuery(queryNumberList.back(),0xAD);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xAD;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    {
        SHA256_CTX hashFile;
        if(SHA224_Init(&hashFile)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }

        SHA224_Update(&hashFile,reinterpret_cast<const char *>(LinkToLogin::private_token_statclient),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        SHA224_Update(&hashFile,dynamicToken,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput),&hashFile);
        /*#ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Hash this to reply: " << binarytoHexa(reinterpret_cast<const char *>(LinkToLogin::private_token_statclient),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                  << " + "
                  << binarytoHexa(dynamicToken,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                  << " = "
                  << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CATCHCHALLENGER_SHA224HASH_SIZE)
                  << std::endl;
        #endif*/
        posOutput+=CATCHCHALLENGER_SHA224HASH_SIZE;
        //memset(LinkToLogin::private_token,0x00,sizeof(LinkToLogin::private_token));->to reconnect after be disconnected
    }

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    return true;
}

void LinkToLogin::tryReconnect()
{
    stat=Stat::Unconnected;
    if(stat!=Stat::Unconnected)
        return;
    else
    {
        //std::cout << "Try reconnect to login..." << std::endl;
        resetForReconnect();
        jsonFileContent.clear();
        #ifndef STATSODROIDSHOW2
        removeJsonFile();
        #endif
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

void LinkToLogin::sendProtocolHeader()
{
    if(stat!=Stat::Connected)
    {
        std::cerr << "ERROR LinkToLogin::sendProtocolHeader() already send (abort)" << std::endl;
        abort();
    }
    //send the network query
    registerOutputQuery(queryNumberList.back(),0xA0);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xA0;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumberList.back();
    queryNumberList.pop_back();
    posOutput+=1;

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,reinterpret_cast<char *>(LinkToLogin::header_magic_number),sizeof(LinkToLogin::header_magic_number));
    posOutput+=sizeof(LinkToLogin::header_magic_number);

    internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    stat=ProtocolSent;
}

void LinkToLogin::moveClientFastPath(const uint8_t &,const uint8_t &)
{
}

void LinkToLogin::updateJsonFile(const bool &withIndentation)
{
    #ifndef STATSODROIDSHOW2
    //std::cout << "Update the json file..." << std::endl;

    if(pFile==NULL && !LinkToLogin::linkToLogin->pFilePath.empty())
    {
        LinkToLogin::linkToLogin->pFile = fopen(LinkToLogin::linkToLogin->pFilePath.c_str(),"wb");
        if(LinkToLogin::linkToLogin->pFile==NULL)
        {
            std::cerr << "Unable to open the output file: " << LinkToLogin::linkToLogin->pFilePath << std::endl;
            abort();
        }
    }

    std::string content;

    std::unordered_map<uint8_t/*group index*/,std::string > serverByGroup;
    unsigned int index=0;
    while(index<serverList.size())
    {
        const ServerFromPoolForDisplay &server=serverList.at(index);
        if(server.logicalGroupIndex<logicalGroups.size())
        {
            std::string serverString;
            if(withIndentation)
                serverString+="    ";
            //serverString+="\""+std::to_string(server.groupIndex)+"-"+std::to_string(server.uniqueKey)+"\":";
            serverString+="{";
            std::string tempXml=server.xml;
            stringreplaceAll(tempXml,"/","\\/");
            stringreplaceAll(tempXml,"\"","\\\"");
            serverString+="\"xml\":\""+tempXml+"\",";
            serverString+="\"connectedPlayer\":"+std::to_string(server.currentPlayer)+",";
            if(server.maxPlayer<65534)
                serverString+="\"maxPlayer\":"+std::to_string(server.maxPlayer)+",";
            serverString+="\"charactersGroup\":"+std::to_string(server.groupIndex)+",";
            serverString+="\"uniqueKey\":"+std::to_string(server.uniqueKey)+"";
            serverString+="}";
            if(serverByGroup.find(server.logicalGroupIndex)==serverByGroup.cend())
                serverByGroup[server.logicalGroupIndex]=serverString;
            else
            {
                if(withIndentation)
                    serverByGroup[server.logicalGroupIndex]+=",\n"+serverString;
                else
                    serverByGroup[server.logicalGroupIndex]+=","+serverString;
            }
        }
        else
        {
            if(!logicalGroups.empty())
                if(!warningLogicalGroupOutOfBound)
                {
                    warningLogicalGroupOutOfBound=true;
                    std::cerr << "Server " << server.uniqueKey << "-" << server.groupIndex << " have logicalGroupIndex of of bound:"
                              << server.logicalGroupIndex << ">=" << logicalGroups.size() << std::endl;
                }
        }
        index++;
    }

    {
        unsigned int indexLogicalGroups=0;
        while(indexLogicalGroups<logicalGroups.size())
        {
            const LogicalGroup &logicalGroup=logicalGroups.at(indexLogicalGroups);
            if(serverByGroup.find(indexLogicalGroups)!=serverByGroup.end())
            {
                if(!content.empty())
                {
                    content+=",";
                    if(withIndentation)
                        content+="\n";
                }
                if(withIndentation)
                    content+="  ";
                content+="\""+logicalGroup.path+"\":{";
                if(withIndentation)
                    content+="\n";
                std::string tempXml=logicalGroup.xml;
                stringreplaceAll(tempXml,"/","\\/");
                stringreplaceAll(tempXml,"\"","\\\"");
                if(withIndentation)
                    content+="  ";
                content+="\"xml\":\""+tempXml+"\",";
                if(withIndentation)
                    content+="\n  ";
                content+="\"servers\":[";
                if(withIndentation)
                    content+="\n";
                content+=serverByGroup.at(indexLogicalGroups);
                if(withIndentation)
                    content+="\n  ";
                content+="]";
                if(withIndentation)
                    content+="\n  ";
                content+="}";
            }
            indexLogicalGroups++;
        }
    }
    if(withIndentation)
        content="\n"+content;
    content="{"+content;
    if(withIndentation)
        content=content+"\n";
    content=content+"}";
    jsonFileContent=content;

    if(pFile!=NULL)
    {
        if(fseek(pFile,0,SEEK_SET)!=0)
        {
            std::cerr << "unable to seek the output file: " << errno << std::endl;
            abort();
        }
        if(fwrite(content.data(),1,content.size(),pFile)!=content.size())
        {
            std::cerr << "unable to write the output file: " << errno << std::endl;
            abort();
        }
        if(fflush(pFile)!=0)
        {
            std::cerr << "unable to flush the output file: " << errno << std::endl;
            abort();
        }
        if(ftruncate(fileno(pFile),content.size())!=0)
        {
            std::cerr << "unable to resize the output file: " << errno << std::endl;
            abort();
        }
    }
    #endif
}

#ifndef STATSODROIDSHOW2
void LinkToLogin::removeJsonFile()
{
    if(pFile!=NULL)
    {
        fclose(pFile);
        pFile=NULL;
    }
    if(remove(pFilePath.c_str())!=0)
    {
        if(errno!=2)
        {
            std::cerr << "Error deleting file: " << pFilePath << ", errno: " << errno << std::endl;
            abort();
        }
    }
}
#endif

#ifdef STATSODROIDSHOW2
void LinkToLogin::writeData(const char * const str)
{
    const int fd=usbdev;
    ::write(fd, "\006", 1);
    int length = strlen(str) + 48;
    ::write(fd, &length, 1);
    int isbusy=0;
    while (isbusy != '6') {
        ::read(fd, &isbusy, 1);
        usleep(10000);
    }
    ::write(fd, str, length - 48);
    isbusy = 0;
}

void LinkToLogin::writeData(const std::string &str)
{
    writeData(str.c_str());
}
#endif
