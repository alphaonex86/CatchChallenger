#include "EventLoopClientLoginSlave.hpp"
#include <iostream>

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */

bool EventLoopClientLoginSlave::sendRawBlock(const char * const data,const unsigned int &size)
{
    return ProtocolParsingBase::internalSendRawSmallPacket(data,size);
}
