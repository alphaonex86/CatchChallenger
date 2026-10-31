#ifndef CATCHCHALLENGER_MONSTEREVOLUTION_H
#define CATCHCHALLENGER_MONSTEREVOLUTION_H

#include <cstdint>

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
/// \brief evolution rule and evolution screen timing shared by the clients and the bots
class DLL_PUBLIC MonsterEvolution
{
public:
    //same rule as the server confirmEvolution(): the first level evolution reached, or the trade evolution of a traded monster
    static bool evolveTo(const Monster &monster,const uint8_t &level,const bool &traded,CATCHCHALLENGER_TYPE_MONSTER &to);
    static const uint32_t animationMs=4000;
    //the screen blinks between the old and the new monster, faster and faster, then stays on the new one
    static bool showEvolved(const uint32_t &elapsedMs);
};
}

#endif // CATCHCHALLENGER_MONSTEREVOLUTION_H
