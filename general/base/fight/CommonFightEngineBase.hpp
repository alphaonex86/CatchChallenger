#ifndef CommonFightEngineBase_BASE_H
#define CommonFightEngineBase_BASE_H

#include <vector>

#include "../GeneralStructures.hpp"
#include "../ClientBase.hpp"
#include "../CommonMap/CommonMap.hpp"
#include "../lib.h"

namespace CatchChallenger {
//only the logique here, store nothing
class DLL_PUBLIC CommonFightEngineBase
{
public:
    static std::vector<PlayerMonster::PlayerSkill> generateWildSkill(const Monster &monster, const uint8_t &level);
    static Monster::Stat getStat(const Monster &monster, const uint8_t &level);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static bool buffIsValid(const Skill::BuffEffect &buffEffect);
    #endif
};
}

#endif // CommonFightEngineBase_H
