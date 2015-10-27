#include "Client.h"

using namespace CatchChallenger;

void Client::sendRawBlock(const char * const data,const unsigned int &size)
{
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("sendRawSmallPacket("+binarytoHexa(data,size)+")");
    #endif
    if(!ProtocolParsingBase::internalSendRawSmallPacket(data,size))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}
