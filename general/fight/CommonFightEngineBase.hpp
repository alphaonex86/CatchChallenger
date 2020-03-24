#ifndef CommonFightEngineBase_BASE_H
#define CommonFightEngineBase_BASE_H

#include <vector>

#include "../base/GeneralStructures.hpp"
#include "../base/ClientBase.hpp"
#include "../../general/base/CommonMap.hpp"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngineBase
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
