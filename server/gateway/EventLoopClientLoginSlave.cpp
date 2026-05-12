#include "EventLoopClientLoginSlave.hpp"
#include <cstring>
#include "EventLoopServerLoginSlave.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <iostream>
#include <string>

using namespace CatchChallenger;

EventLoopClientLoginSlave::EventLoopClientLoginSlave(
            const int &infd
        ) :
        EventLoopClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Server
            #endif
            ),
        stat(EventLoopClientLoginStat::None),
        datapackStatus(DatapackStatus::Base),
        fastForward(false),
        linkToGameServer(NULL),
        socketString(NULL),
        socketStringSize(0)
{
    //Allow dynamic-size packets from the start. The client legitimately
    //sends 0xA1 (datapack-file-list query, packetFixedSize==0xFE) as
    //soon as the gateway has forwarded the backend's 0xA0 reply, well
    //before any backend-reply path can call allowDynamicSize() for us.
    //Without this the parser rejects 0xA1 with "dynamic size blocked
    //(header) for packet code: 161" and the client→map test fails.
    //LinkToGameServer's ctor already does the same thing for its own
    //direction; mirror it on the client-facing slave.
    flags|=0x08;
    client_list.push_back(this);
}

EventLoopClientLoginSlave::~EventLoopClientLoginSlave()
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

#ifdef CATCHCHALLENGER_IO_URING
void EventLoopClientLoginSlave::onAsyncRecv(const char *buf,size_t len)
{
    parseIncommingDataAsync(buf,len);
}
#endif

bool EventLoopClientLoginSlave::disconnectClient()
{
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
    EventLoopClient::close();
    vectorremoveOne(client_list,this);
    /*SIGILLif(stat!=EventLoopClientLoginStat::None)
        messageParsingLayer("Disconnected client");*/
    return true;
}

//input/ouput layer
void EventLoopClientLoginSlave::errorParsingLayer(const std::string &error)
{
    std::cerr << socketString << ": " << sanitizeUtf8String(error) << std::endl;
    disconnectClient();
}

void EventLoopClientLoginSlave::messageParsingLayer(const std::string &message) const
{
    std::cout << socketString << ": " << sanitizeUtf8String(message) << std::endl;
}

void EventLoopClientLoginSlave::errorParsingLayer(const char * const error)
{
    if(stat==EventLoopClientLoginStat::None)
    {
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
        return;
    }
    std::cerr << socketString << ": " << sanitizeUtf8String(std::string(error)) << std::endl;
    disconnectClient();
}

void EventLoopClientLoginSlave::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << sanitizeUtf8String(std::string(message)) << std::endl;
}

BaseClassSwitch::EventLoopObjectType EventLoopClientLoginSlave::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Client;
}

void EventLoopClientLoginSlave::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

bool EventLoopClientLoginSlave::sendRawBlock(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

bool EventLoopClientLoginSlave::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}

bool EventLoopClientLoginSlave::sendDatapackProgression(const uint8_t progression)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x78;
    posOutput+=1+4;
    {const uint32_t _tmp_le=(htole32(1+1));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EventLoopServerLoginSlave::unixServerLoginSlave->gatewayNumber;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=progression;
    posOutput+=1;

    return sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EventLoopClientLoginSlave::allowDynamicSize()
{
    flags|=0x08;
}

void EventLoopClientLoginSlave::breakNeedMoreData()
{
    if(stat==EventLoopClientLoginStat::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_HARDENED
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

ssize_t EventLoopClientLoginSlave::readFromSocket(char * data, const size_t &size)
{
    return EventLoopClient::read(data,size);
}

ssize_t EventLoopClientLoginSlave::writeToSocket(const char * const data, const size_t &size)
{
    return EventLoopClient::write(data,size);
}

void EventLoopClientLoginSlave::closeSocket()
{
    disconnectClient();
}
