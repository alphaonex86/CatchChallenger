#include "CharactersGroupForLogin.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

QHash<QString,CharactersGroupForLogin *> CharactersGroupForLogin::hash;
QList<CharactersGroupForLogin *> CharactersGroupForLogin::list;
char CharactersGroupForLogin::tempBuffer[4096];

CharactersGroupForLogin::CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const uint8_t &considerDownAfterNumberOfTry,const uint8_t &tryInterval) :
    databaseBaseCommon(new EpollPostgresql())
{
    databaseBaseCommon->considerDownAfterNumberOfTry=considerDownAfterNumberOfTry;
    databaseBaseCommon->tryInterval=tryInterval;
    if(!databaseBaseCommon->syncConnect(host,db,login,pass))
    {
        std::cerr << "Connect to login database failed:" << databaseBaseCommon->errorMessage() << std::endl;
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
    tempServer.host=QString::fromUtf8(hostData,hostDataSize);
    tempServer.port=port;
    tempServer.indexOnFlatList=indexOnFlatList,
    servers[serverUniqueKey]=tempServer;
}

bool CharactersGroupForLogin::containsServerUniqueKey(const uint32_t &serverUniqueKey) const
{
    return servers.contains(serverUniqueKey);
}

CharactersGroupForLogin::InternalGameServer CharactersGroupForLogin::getServerInformation(const uint32_t &serverUniqueKey) const
{
    return servers.value(serverUniqueKey);
}

BaseClassSwitch::EpollObjectType CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

DatabaseBase::DatabaseType CharactersGroupForLogin::databaseType() const
{
    return databaseBaseCommon->databaseType();
}
