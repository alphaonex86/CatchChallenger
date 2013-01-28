#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QPixmap>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../base/Api_protocol.h"

namespace Pokecraft {
//only the logique here, store nothing
class FightEngine : public QObject
{
    Q_OBJECT
public:
    static FightEngine fightEngine;
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool haveRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool canDoFight();
    bool isInFight();
    struct MonsterExtra
    {
        QString name;
        QString description;
        QPixmap front;
        QPixmap back;
        QPixmap small;
        struct Buff
        {
            QString name;
            QString description;
        };
        struct Skill
        {
            QString name;
            QString description;
        };
    };
    struct MonsterSkillEffect
    {
        quint32 skill;
    };
    //fight
    QHash<quint32,Monster> monsters;
    QHash<quint32,Monster::Skill> monsterSkills;
    QHash<quint32,Monster::Buff> monsterBuffs;
    QHash<quint32,MonsterExtra> monsterExtra;
    QHash<quint32,MonsterExtra::Buff> monsterBuffsExtra;
    QHash<quint32,MonsterExtra::Skill> monsterSkillsExtra;
    void setPlayerMonster(const QList<PlayerMonster> &playerMonsterList);
    QList<PlayerMonster> getPlayerMonster();
    PlayerMonster getFightMonster();
    PlayerMonster getOtherMonster();
    bool haveOtherMonster();
    //last step
    QList<quint8> stepFight_Water,stepFight_Grass,stepFight_Cave;
    //current fight
    QList<PlayerMonster> wildMonsters;
    QList<Monster::Stat> wildMonstersStat;
    bool tryEscape();//return true if is escaped
    bool canDoFightAction();
    QList<Monster::Skill::BuffEffect> buffEffectOtherMonster;
    QList<Monster::Skill::LifeEffect> lifeEffectOtherMonster;
    quint32 generateOtherAttack(bool *ok);
    bool wildMonsterIsKO();
    bool currentMonsterIsKO();
    bool dropKOWildMonster();
    bool dropKOCurrentMonster();
private:
    int selectedMonster;
    QByteArray m_randomSeeds;
    QList<PlayerMonster> playerMonsterList;
    bool m_canDoFight;
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList, bool *ok);
    inline quint8 getOneSeed(const quint8 &max=0);
    void applyOtherBuffEffect(const Monster::Skill::BuffEffect &effect);
    void applyOtherLifeEffect(const Monster::Skill::LifeEffect &effect);
private:
    void updateCanDoFight();
    explicit FightEngine();
    ~FightEngine();
public slots:
    //fight
    void appendRandomSeeds(const QByteArray &data);
    static Monster::Stat getStat(const Monster &monster,const quint8 &level);
};
}

#endif // FightEngine_H
