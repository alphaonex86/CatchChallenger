#include "EpollClientLoginSlave.hpp"
#include "EpollServerLoginSlave.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <iostream>
#include <string>

using namespace CatchChallenger;

EpollClientLoginSlave::EpollClientLoginSlave(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        ) :
        EpollClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Server
            #endif
            ),
        stat(EpollClientLoginStat::None),
        datapackStatus(DatapackStatus::Base),
        fastForward(false),
        linkToGameServer(NULL),
        socketString(NULL),
        socketStringSize(0)
{
    client_list.push_back(this);
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    vectorremoveOne(client_list,this);
    if(socketString!=NULL)
    {
        delete socketString;
        socketString=NULL;
    }
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }

    if(client_list.size()==0)
        LinkToGameServer::compressionSet=false;
}

bool EpollClientLoginSlave::disconnectClient()
{
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
    EpollClient::close();
    vectorremoveOne(client_list,this);
    /*SIGILLif(stat!=EpollClientLoginStat::None)
        messageParsingLayer("Disconnected client");*/
    return true;
}

//input/ouput layer
void EpollClientLoginSlave::errorParsingLayer(const std::string &error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const std::string &message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

void EpollClientLoginSlave::errorParsingLayer(const char * const error)
{
    if(stat==EpollClientLoginStat::None)
    {
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
        return;
    }
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::EpollObjectType EpollClientLoginSlave::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void EpollClientLoginSlave::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool EpollClientLoginSlave::sendRawBlock(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

bool EpollClientLoginSlave::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}

bool EpollClientLoginSlave::sendDatapackProgression(const uint8_t progression)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x78;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+1);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EpollServerLoginSlave::epollServerLoginSlave->gatewayNumber;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=progression;
    posOutput+=1;

    return sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EpollClientLoginSlave::allowDynamicSize()
{
    flags|=0x08;
}

void EpollClientLoginSlave::breakNeedMoreData()
{
    if(stat==EpollClientLoginStat::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

ssize_t EpollClientLoginSlave::read(char * data, const size_t &size)
{
    return EpollClient::read(data,size);
}

ssize_t EpollClientLoginSlave::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    return EpollClient::write(data,size);
}

void EpollClientLoginSlave::closeSocket()
{
    disconnectClient();
}
