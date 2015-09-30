#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

bool EpollClientLoginSlave::sendRawSmallPacket(const char * const data,const unsigned int &size)
{
    return ProtocolParsingBase::internalSendRawSmallPacket(data,size);
}
