#include "CharactersGroupForLogin.h"
#include <iostream>
#include <QDebug>

using namespace CatchChallenger;

int CharactersGroupForLogin::serverWaitedToBeReady=0;
QHash<QString,CharactersGroupForLogin *> CharactersGroupForLogin::hash;
QList<CharactersGroupForLogin *> CharactersGroupForLogin::list;

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

BaseClassSwitch::Type CharactersGroupForLogin::getType() const
{
    return BaseClassSwitch::Type::Client;
}
