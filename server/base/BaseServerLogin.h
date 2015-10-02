#ifndef CATCHCHALLENGER_BASESERVERLOGIN_H
#define CATCHCHALLENGER_BASESERVERLOGIN_H

#include <QList>
#include <std::regex>
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
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    struct TokenLink
    {
        void * client;
        char value[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    };
    static TokenLink tokenForAuth[CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION];
    static uint32_t tokenForAuthSize;
    #endif

    #ifndef CATCHCHALLENGER_CLASS_LOGIN
    DatabaseBase *databaseBaseLogin;
    #endif
protected:
    void preload_the_randomData();
    void unload();
    void unload_the_randomData();
};
}

#endif
