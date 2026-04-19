#ifndef AUTOARGS_H
#define AUTOARGS_H

#include <QString>
#include <cstdint>

class AutoArgs
{
public:
    static QString server;
    static QString host;
    static uint16_t port;
    static bool autologin;
    static QString character;
    static bool closeWhenOnMap;
    static bool dropSendDataAfterOnMap;
    static bool autosolo;

    static void parse(int &argc, char *argv[]);
    static void printHelp(const char *progName);
};

#endif // AUTOARGS_H
