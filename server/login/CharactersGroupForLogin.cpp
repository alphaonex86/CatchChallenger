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
    servers.clear();
}

void CharactersGroupForLogin::setServerUniqueKey(const quint8 &indexOnFlatList,const quint32 &serverUniqueKey,const char * const hostData,const quint8 &hostDataSize,const quint16 &port)
{
    InternalGameServer tempServer;
    tempServer.host=QString::fromUtf8(hostData,hostDataSize);
    tempServer.port=port;
    tempServer.indexOnFlatList=indexOnFlatList,
    servers[serverUniqueKey]=tempServer;
}

bool CharactersGroupForLogin::containsServerUniqueKey(const quint32 &serverUniqueKey) const
{
    return servers.contains(serverUniqueKey);
}

CharactersGroupForLogin::InternalGameServer CharactersGroupForLogin::getServerInformation(const quint32 &serverUniqueKey) const
{
    return servers.value(serverUniqueKey);
}

BaseClassSwitch::Type CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::Type::Client;
}

DatabaseBase::Type CharactersGroupForLogin::databaseType() const
{
    return databaseBaseCommon->databaseType();
}
