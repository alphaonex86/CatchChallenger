#ifndef CLIOPTIONS_HPP
#define CLIOPTIONS_HPP

#include <QString>

class CliOptions
{
public:
    static QString serverName;
    static bool autologin;
    static QString characterName;
    static bool closeWhenOnMap;
    static bool dropSendDataAfterOnMap;
};

#endif // CLIOPTIONS_HPP
