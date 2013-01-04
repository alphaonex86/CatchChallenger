#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"

namespace Pokecraft {
class FightLoader
{
public:
    static QHash<quint32,Monster> loadMonster(const QString &file,QHash<quint32,MonsterSkill> monsterSkills);
    static QHash<quint32,MonsterSkill> loadMonsterSkill(const QString &file,QHash<quint32,MonsterBuff> monsterBuffs);
    static QHash<quint32,MonsterBuff> loadMonsterBuff(const QString &file);
};
}

#endif // FIGHTLOADER_H
