#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"

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
        ProtocolParsingInputOutput(
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ,PacketModeTransmission_Server
            #endif
            ),
        stat(EpollClientLoginStat::None),
        datapackStatus(DatapackStatus::Base),
        fastForward(false),
        linkToGameServer(NULL),
        socketString(NULL),
        socketStringSize(0),
        movePacketKickSize(0),
        movePacketKickTotalCache(0),
        movePacketKickNewValue(0),
        chatPacketKickSize(0),
        chatPacketKickTotalCache(0),
        chatPacketKickNewValue(0),
        otherPacketKickSize(0),
        otherPacketKickTotalCache(0),
        otherPacketKickNewValue(0)
{
    {
        memset(movePacketKick,
               0x00,
               sizeof(movePacketKick));
        memset(chatPacketKick,
               0x00,
               sizeof(chatPacketKick));
        memset(otherPacketKick,
               0x00,
               sizeof(otherPacketKick));
    }
    client_list.push_back(this);
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    {
        unsigned int index=0;
        while(index<client_list.size())
        {
            const EpollClientLoginSlave * const client=client_list.at(index);
            if(this==client)
            {
                client_list.erase(client_list.begin()+index);
                break;
            }
            index++;
        }
    }

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

void EpollClientLoginSlave::disconnectClient()
{
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
    epollSocket.close();
    //messageParsingLayer("Disconnected client");
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
