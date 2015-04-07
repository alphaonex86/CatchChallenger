#ifndef CATCHCHALLENGER_BASESERVERLOGIN_H
#define CATCHCHALLENGER_BASESERVERLOGIN_H

#include <QList>
#include <QRegularExpression>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#include "DatabaseBase.h"

namespace CatchChallenger {
class BaseServerLogin
{
public:
    explicit BaseServerLogin();
    virtual ~BaseServerLogin();

    #ifdef Q_OS_LINUX
    static FILE *fpRandomFile;
    #endif
    struct TokenLink
    {
        void * client;
        char value[CATCHCHALLENGER_TOKENSIZE];
    };
    static TokenLink tokenForAuth[CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION];
    static quint32 tokenForAuthSize;

    DatabaseBase *databaseBaseLogin;

    QList<ActionAllow> dictionary_allow_database_to_internal;
    QList<quint8> dictionary_allow_internal_to_database;
    QList<int> dictionary_reputation_database_to_internal;//negative == not found
    QList<quint8> dictionary_skin_database_to_internal;
    QList<quint32> dictionary_skin_internal_to_database;
    QList<quint8> dictionary_starter_database_to_internal;
    QList<quint32> dictionary_starter_internal_to_database;
private:
    void preload_the_randomData();
protected:
    void unload();
    void unload_the_randomData();
};
}

#endif
