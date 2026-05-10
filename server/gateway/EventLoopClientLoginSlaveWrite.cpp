#include "EventLoopClientLoginSlave.hpp"
#include <iostream>

using namespace CatchChallenger;

bool EventLoopClientLoginSlave::sendRawBlock(const char * const data,const unsigned int &size)
{
    return ProtocolParsingBase::internalSendRawSmallPacket(data,size);
}
