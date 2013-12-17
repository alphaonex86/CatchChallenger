#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <QHash>
#include <QString>
#include <QMultiHash>

#include "../base/GeneralStructures.h"
#include "../base/DatapackGeneralLoader.h"

namespace CatchChallenger {
class FightLoader
{
public:
    static QList<Type> loadTypes(const QString &file);
    static QHash<quint32,Monster> loadMonster(const QString &file, const QHash<quint32,Skill> &monsterSkills, const QList<Type> &types, const QHash<quint32, Item> &items);
    static QHash<quint32, QHash<quint32, quint32> > loadMonsterEvolutionItems(const QHash<quint32,Monster> &monsters);
    static QHash<quint32,Skill> loadMonsterSkill(const QString &file, const QHash<quint32,Buff> &monsterBuffs, const QList<Type> &types);
    static QHash<quint32,Buff> loadMonsterBuff(const QString &file);
    static QHash<quint32,BotFight> loadFight(const QString &folder, const QHash<quint32,Monster> &monsters, const QHash<quint32, Skill> &monsterSkills);
    static QList<PlayerMonster::PlayerSkill> loadDefaultAttack(const quint32 &monsterId, const quint8 &level, const QHash<quint32,Monster> &monsters, const QHash<quint32, Skill> &monsterSkills);
};
bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2);
}

#endif // FIGHTLOADER_H
