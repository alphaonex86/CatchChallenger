#ifndef CATCHCHALLENGER_BASESERVERLOGIN_H
#define CATCHCHALLENGER_BASESERVERLOGIN_H

#include "../../general/base/GeneralVariable.hpp"
#include "VariableServer.hpp"
#include "DatabaseBase.hpp"
#include <stdio.h>
#include <cstdint>

namespace CatchChallenger {
class BaseServerLogin
{
public:
    explicit BaseServerLogin();
    virtual ~BaseServerLogin();

    #ifdef __linux__
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

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_LOGIN
    DatabaseBase *databaseBaseLogin;
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif
protected:
    void preload_2_sync_the_randomData();
    void unload();
    void unload_the_randomData();
};
}

#endif
