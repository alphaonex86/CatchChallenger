#ifndef CATCHCHALLENGER_CLIENTBASE_H
#define CATCHCHALLENGER_CLIENTBASE_H

#include "GeneralStructures.h"
#include "GeneralVariable.h"

namespace CatchChallenger {
class ClientBase
{
public:
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    static Player_private_and_public_informations *public_and_private_informations_solo;
    #endif
    Player_private_and_public_informations public_and_private_informations;
};
}

#endif // CHAT_PARSING_H
