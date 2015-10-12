#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

bool EpollClientLoginSlave::sendRawBlock(const char * const data,const unsigned int &size)
{
    return ProtocolParsingBase::internalSendRawSmallPacket(data,size);
}
