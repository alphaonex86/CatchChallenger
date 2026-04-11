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

    static void parse(int &argc, char *argv[]);
};

#endif // AUTOARGS_H
