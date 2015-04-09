#include "CharactersGroupForLogin.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

QHash<QString,CharactersGroupForLogin *> CharactersGroupForLogin::hash;
QList<CharactersGroupForLogin *> CharactersGroupForLogin::list;
char CharactersGroupForLogin::tempBuffer[4096];

CharactersGroupForLogin::CharactersGroupForLogin(const char * const db,const char * const host,const char * const login,const char * const pass,const quint8 &considerDownAfterNumberOfTry,const quint8 &tryInterval) :
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
    dictionary_server_database_to_index.clear();
    dictionary_server_index_to_database.clear();
    servers.clear();
}

void CharactersGroupForLogin::setServerPair(const quint8 &index, const quint16 &databaseId)
{
    while(dictionary_server_database_to_index.size()<(databaseId+1))
        dictionary_server_database_to_index << -1;
    while(dictionary_server_index_to_database.size()<(index+1))
        dictionary_server_index_to_database << 0;
    dictionary_server_index_to_database[index]=databaseId;
    dictionary_server_database_to_index[databaseId]=index;
}

void CharactersGroupForLogin::setServerUniqueKey(const quint32 &serverUniqueKey,const char * const hostData,const quint8 &hostDataSize,const quint16 &port)
{
    InternalLoginServer tempServer;
    tempServer.host=QString::fromUtf8(hostData,hostDataSize);
    tempServer.port=port;
    servers[serverUniqueKey]=tempServer;
}

bool CharactersGroupForLogin::containsServerUniqueKey(const quint32 &serverUniqueKey) const
{
    return servers.contains(serverUniqueKey);
}

BaseClassSwitch::Type CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::Type::Client;
}

DatabaseBase::Type CharactersGroupForLogin::databaseType() const
{
    return databaseBaseCommon->databaseType();
}
