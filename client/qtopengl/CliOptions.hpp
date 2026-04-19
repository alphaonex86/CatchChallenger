#ifndef CLIOPTIONS_HPP
#define CLIOPTIONS_HPP

#include <QString>
#include <cstdint>

class CliOptions
{
public:
    static QString serverName;
    static QString host;
    static uint16_t port;
    static bool autologin;
    static QString characterName;
    static bool closeWhenOnMap;
    static bool dropSendDataAfterOnMap;
    static bool autosolo;
};

#endif // CLIOPTIONS_HPP
