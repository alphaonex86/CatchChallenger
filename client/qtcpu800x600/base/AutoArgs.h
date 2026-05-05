#ifndef AUTOARGS_H
#define AUTOARGS_H

#include <QString>
#include <cstdint>

class AutoArgs
{
public:
    // --server NAME: select an existing server-list entry by name (TCP or WS).
    static QString server;
    // --host + --port: TCP direct connect. Meaningful only when
    // CATCHCHALLENGER_NO_TCPSOCKET is NOT defined.
    static QString host;
    static uint16_t port;
    // --url: WebSocket direct connect. Meaningful only when
    // CATCHCHALLENGER_NO_WEBSOCKET is NOT defined.
    static QString url;
    static bool autologin;
    static QString character;
    static bool closeWhenOnMap;
    static int closeWhenOnMapAfter;
    static bool dropSendDataAfterOnMap;
    static bool autosolo;
    static QString mainDatapackCodeOverride;

    static void parse(int &argc, char *argv[]);
    static void printHelp(const char *progName);
};

#endif // AUTOARGS_H
