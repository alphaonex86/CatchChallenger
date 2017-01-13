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
    preparedDBQueryCommon.initDatabaseQueryCommonWithoutSP(databaseBaseCommon->databaseType(),databaseBaseCommon);
    preparedDBQueryCommonForLogin.initDatabaseQueryCommonForLogin(databaseBaseCommon->databaseType(),databaseBaseCommon);
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
    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
    std::cout << "setServerUniqueKey(" << std::to_string(indexOnFlatList) << "," << std::to_string(serverUniqueKey) << "," << std::string(hostData,hostDataSize) << "," << std::to_string(hostDataSize) << "," << std::to_string(port) << ") in " << __FILE__ << ":" <<__LINE__ << std::endl;
    #endif
    InternalGameServer tempServer;
    tempServer.host=std::string(hostData,hostDataSize);
    tempServer.port=port;
    tempServer.indexOnFlatList=indexOnFlatList;
    servers[serverUniqueKey]=tempServer;
}

void CharactersGroupForLogin::setIndexServerUniqueKey(const uint8_t &indexOnFlatList,const uint32_t &serverUniqueKey)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(servers.find(serverUniqueKey)==servers.cend())
    {
        std::cerr << "setIndexServerUniqueKey(" << std::to_string(indexOnFlatList) << "," << std::to_string(serverUniqueKey) << ") unique key not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
        abort();
    }
    #endif
    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
    std::cout << "setIndexServerUniqueKey(" << std::to_string(indexOnFlatList) << "," << std::to_string(serverUniqueKey) << ") in " << __FILE__ << ":" <<__LINE__ << std::endl;
    #endif
    servers[serverUniqueKey].indexOnFlatList=indexOnFlatList;
}

void CharactersGroupForLogin::removeServerUniqueKey(const uint32_t &serverUniqueKey)
{
    servers.erase(serverUniqueKey);
}

bool CharactersGroupForLogin::containsServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return servers.find(serverUniqueKey)!=servers.cend();
}

CharactersGroupForLogin::InternalGameServer CharactersGroupForLogin::getServerInformation(const uint32_t &serverUniqueKey) const
{
    return servers.at(serverUniqueKey);
}

const std::unordered_map<uint32_t,CharactersGroupForLogin::InternalGameServer> &CharactersGroupForLogin::getServerListRO() const
{
    return servers;
}

BaseClassSwitch::EpollObjectType CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

DatabaseBase::DatabaseType CharactersGroupForLogin::databaseType() const
{
    return databaseBaseCommon->databaseType();
}

DatabaseBase * CharactersGroupForLogin::database() const
{
    return databaseBaseCommon;
}

#ifdef CATCHCHALLENGER_EXTRA_CHECK
uint8_t CharactersGroupForLogin::serverCountForAllCharactersGroup()
{
    uint8_t count=0;
    unsigned int index=0;
    while(index<list.size())
    {
        const CharactersGroupForLogin * const group=list.at(index);
        count+=group->servers.size();
        index++;
    }
    return count;
}
#endif

#ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
void CharactersGroupForLogin::serverDumpCharactersGroup()
{
    unsigned int index=0;
    while(index<list.size())
    {
        std::cout << "dump CharactersGroup " << std::to_string(index) << ":" << std::endl;
        const CharactersGroupForLogin * const group=list.at(index);
        for(const auto& n:group->getServerListRO())
        {
            const uint32_t &uniqueKey=n.first;
            const InternalGameServer& server=n.second;
            std::cout << "server " << std::to_string(server.indexOnFlatList) << ", host: " << server.host << ":" << std::to_string(server.port) << ", unique key: " << std::to_string(uniqueKey) << std::endl;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(server.indexOnFlatList>=serverCountForAllCharactersGroup())
            {
                std::cerr << "CharactersGroupForLogin::server_list_object(): server.indexOnFlatList(" << std::to_string(server.indexOnFlatList) << ")>=servers.size(" << std::to_string(serverCountForAllCharactersGroup()) << ")" << std::endl;
                abort();
            }
            #endif
        }
        index++;
    }
}
#endif
