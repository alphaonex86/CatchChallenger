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

namespace CatchChallenger {
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
    void healAllMonsters();
    bool isInFight();
    struct MonsterExtra
    {
        QString name;
        QString description;
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
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
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    QList<PlayerMonster> getPlayerMonster();
    bool removeMonster(const quint32 &monsterId);
    bool remainMonstersToFight(const quint32 &monsterId);
    PlayerMonster getFightMonster();
    PlayerMonster getOtherMonster();
    bool haveOtherMonster();
    //last step
    QList<quint8> stepFight_Water,stepFight_Grass,stepFight_Cave;
    //current fight
    QList<PlayerMonster> wildMonsters;
    bool tryEscape();//return true if is escaped
    bool canDoFightAction();
    void useSkill(const quint32 &skill);
    QList<Monster::Skill::AttackReturn> attackReturnList;
    QList<quint32> otherMonsterAttack;
    void generateOtherAttack();
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
    Monster::Skill::LifeEffectReturn applyOtherLifeEffect(const Monster::Skill::LifeEffect &effect);
    void doTheCurrentMonsterAttack(const quint32 &skill);
    void applyCurrentBuffEffect(const Monster::Skill::BuffEffect &effect);
    Monster::Skill::LifeEffectReturn applyCurrentLifeEffect(const Monster::Skill::LifeEffect &effect);
    bool internalTryEscape();
    void addXPSP();
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
