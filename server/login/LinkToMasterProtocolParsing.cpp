#include "LinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../epoll/EpollSocket.h"
#include "VariableLoginServer.h"

#include <iostream>
#include <openssl/sha.h>

using namespace CatchChallenger;

bool LinkToMaster::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &, const char * const , const unsigned int &)
{
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        return false;
    }
    return false;
}

//have query with reply
bool LinkToMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
}

void LinkToMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
