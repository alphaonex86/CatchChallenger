#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"

namespace Pokecraft {
class FightLoader
{
public:
    static QHash<quint32,Monster> loadMonster(const QString &file,const QHash<quint32,Monster::Skill> &monsterSkills);
    static QHash<quint32,Monster::Skill> loadMonsterSkill(const QString &file,const QHash<quint32,Monster::Buff> &monsterBuffs);
    static QHash<quint32,Monster::Buff> loadMonsterBuff(const QString &file);
};
}

#endif // FIGHTLOADER_H
