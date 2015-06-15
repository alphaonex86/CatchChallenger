#ifndef CommonFightEngineBase_BASE_H
#define CommonFightEngineBase_BASE_H

#include <QByteArray>
#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"
#include "../base/ClientBase.h"
#include "../../general/base/CommonMap.h"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngineBase
{
public:
    static QList<PlayerMonster::PlayerSkill> generateWildSkill(const Monster &monster, const quint8 &level);
    static Monster::Stat getStat(const Monster &monster, const quint8 &level);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static bool buffIsValid(const Skill::BuffEffect &buffEffect);
    #endif
};
}

#endif // CommonFightEngineBase_H
