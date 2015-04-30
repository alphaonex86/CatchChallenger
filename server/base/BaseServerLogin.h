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
protected:
    void preload_the_randomData();
    void unload();
    void unload_the_randomData();
};
}

#endif
