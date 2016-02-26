#ifndef CATCHCHALLENGER_GLOBALSERVERDATA_H
#define CATCHCHALLENGER_GLOBALSERVERDATA_H

#include "ServerStructures.h"

namespace CatchChallenger {
class GlobalServerData
{
public:
    //shared into all the program
    static GameServerSettings serverSettings;
    static ServerPrivateVariables serverPrivateVariables;
};
}

#endif
