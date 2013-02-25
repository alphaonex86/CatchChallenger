#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"

namespace CatchChallenger {
class FightLoader
{
public:
    static QHash<quint32,Monster> loadMonster(const QString &file,const QHash<quint32,Skill> &monsterSkills);
    static QHash<quint32,Skill> loadMonsterSkill(const QString &file,const QHash<quint32,Buff> &monsterBuffs);
    static QHash<quint32,Buff> loadMonsterBuff(const QString &file);
};
}

#endif // FIGHTLOADER_H
