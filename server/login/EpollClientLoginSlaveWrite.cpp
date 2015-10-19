#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */

bool EpollClientLoginSlave::sendRawBlock(const char * const data,const unsigned int &size)
{
    return ProtocolParsingBase::internalSendRawSmallPacket(data,size);
}
