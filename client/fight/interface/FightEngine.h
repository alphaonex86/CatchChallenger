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
    struct MonsterSkillEffect
    {
        quint32 skill;
    };
    static FightEngine fightEngine;
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool haveRandomFight(const Map &map,const quint8 &x,const quint8 &y);
    bool canDoFight();
    void healAllMonsters();
    bool isInFight();
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    void setPlayerMonster(const QList<PlayerMonster> &playerMonsterList);
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    QList<PlayerMonster> getPlayerMonster();
    bool removeMonster(const quint32 &monsterId);
    bool remainMonstersToFight(const quint32 &monsterId);
    PlayerMonster getFightMonster();
    PublicPlayerMonster getOtherMonster();
    bool haveOtherMonster();
    //last step
    QList<quint8> stepFight_Water,stepFight_Grass,stepFight_Cave;
    //current fight
    QList<PlayerMonster> wildMonsters,botFightMonsters;
    QList<PublicPlayerMonster> battleCurrentMonster;
    QList<quint8> battleStat,botMonstersStat;
    QList<quint8> battleMonsterPlace;
    bool tryEscape();//return true if is escaped
    bool canDoFightAction();
    void useSkill(const quint32 &skill);
    void useSkillAgainstWildMonster(const quint32 &skill);
    void useSkillAgainstBotMonster(const quint32 &skill);
    QList<quint32> otherMonsterAttack;
    void generateOtherAttack();
    bool otherMonsterIsKO();
    bool currentMonsterIsKO();
    bool dropKOOtherMonster();
    void finishTheBattle();
    bool dropKOCurrentMonster();
    void setBattleMonster(const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void setBotMonster(const QList<PlayerMonster> &publicPlayerMonster);
    void addBattleMonster(const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    bool haveWin();
    void addAndApplyAttackReturnList(const QList<Skill::AttackReturn> &attackReturnList);
    QList<Skill::AttackReturn> getAttackReturnList() const;
    void removeTheFirstLifeEffectAttackReturn();
    bool firstLifeEffectQuantityChange(qint32 quantity);
private:
    QList<Skill::AttackReturn> attackReturnList;
    int selectedMonster;
    QByteArray m_randomSeeds;
    QList<PlayerMonster> playerMonsterList;
    bool m_canDoFight;
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList, bool *ok);
    inline quint8 getOneSeed(const quint8 &max=0);
    void applyOtherBuffEffect(const Skill::BuffEffect &effect);
    Skill::LifeEffectReturn applyOtherLifeEffect(const Skill::LifeEffect &effect);
    void doTheCurrentMonsterAttack(const quint32 &skill);
    bool applyCurrentBuffEffect(const Skill::BuffEffect &effect);
    ApplyOn invertApplyOn(const ApplyOn &applyOn);
    Skill::LifeEffectReturn applyCurrentLifeEffect(const Skill::LifeEffect &effect);
    bool applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn);
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
signals:
    void newError(QString error,QString detailedError);
    void error(QString error);
};
}

#endif // FightEngine_H
