#ifndef AUTOARGS_H
#define AUTOARGS_H

#include <QString>

class AutoArgs
{
public:
    static QString server;
    static bool autologin;
    static QString character;
    static bool closeWhenOnMap;
    static bool dropSendDataAfterOnMap;
    static bool autosolo;

    static void parse(int &argc, char *argv[]);
    static void printHelp(const char *progName);
};

#endif // AUTOARGS_H
