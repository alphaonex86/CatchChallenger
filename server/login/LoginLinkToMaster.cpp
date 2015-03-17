#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

LoginLinkToMaster::LoginLinkToMaster()
{
}

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
        have_send_protocol(false),
        is_logging_in_progess(false)
{
}

void LoginLinkToMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void LoginLinkToMaster::errorParsingLayer(const QString &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const QString &message) const
{
    std::cout << socketString << ": " << message.toLocal8Bit().constData() << std::endl;
}

void LoginLinkToMaster::errorParsingLayer(const char * const error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void LoginLinkToMaster::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::Type LoginLinkToMaster::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void LoginLinkToMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}
