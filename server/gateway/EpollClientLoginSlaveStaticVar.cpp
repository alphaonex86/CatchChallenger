#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

std::vector<EpollClientLoginSlave *> EpollClientLoginSlave::client_list;

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;
