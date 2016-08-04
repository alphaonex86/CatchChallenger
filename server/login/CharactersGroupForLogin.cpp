#include "CharactersGroupForLogin.h"
#include <iostream>

using namespace CatchChallenger;

std::unordered_map<std::string,CharactersGroupForLogin *> CharactersGroupForLogin::hash;
std::vector<CharactersGroupForLogin *> CharactersGroupForLogin::list;
char CharactersGroupForLogin::tempBuffer[4096];

CharactersGroupForLogin::CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const uint8_t &considerDownAfterNumberOfTry,const uint8_t &tryInterval) :
    maxCharacterIdRequested(false),
    maxMonsterIdRequested(false),
    databaseBaseCommon(new EpollPostgresql())
{
    databaseBaseCommon->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    databaseBaseCommon->tryInterval=tryInterval;
    if(!databaseBaseCommon->syncConnect(host,db,login,pass))
    {
        std::cerr << "Connect to common database failed:" << databaseBaseCommon->errorMessage() << ", host: " << host << ", db: " << db << ", login: " << login << std::endl;
        abort();
    }
}

CharactersGroupForLogin::~CharactersGroupForLogin()
{
    if(databaseBaseCommon!=NULL)
    {
        delete databaseBaseCommon;
        databaseBaseCommon=NULL;
    }
}

void CharactersGroupForLogin::clearServerPair()
{
    servers.clear();
}

void CharactersGroupForLogin::setServerUniqueKey(const uint8_t &indexOnFlatList,const uint32_t &serverUniqueKey,const char * const hostData,const uint8_t &hostDataSize,const uint16_t &port)
{
    InternalGameServer tempServer;
    tempServer.host=std::string(hostData,hostDataSize);
    tempServer.port=port;
    tempServer.indexOnFlatList=indexOnFlatList,
    servers[serverUniqueKey]=tempServer;
}

bool CharactersGroupForLogin::containsServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return servers.find(serverUniqueKey)!=servers.cend();
}

CharactersGroupForLogin::InternalGameServer CharactersGroupForLogin::getServerInformation(const uint32_t &serverUniqueKey) const
{
    return servers.at(serverUniqueKey);
}

BaseClassSwitch::EpollObjectType CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

DatabaseBase::DatabaseType CharactersGroupForLogin::databaseType() const
{
    return databaseBaseCommon->databaseType();
}
