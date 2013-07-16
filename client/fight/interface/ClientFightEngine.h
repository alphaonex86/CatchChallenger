#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QPixmap>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../base/Api_protocol.h"

namespace CatchChallenger {
//only the logique here, store nothing
class ClientFightEngine : public CommonFightEngine
{
    Q_OBJECT
public:
    struct MonsterSkillEffect
    {
        quint32 skill;
    };
    static ClientFightEngine fightEngine;
    virtual void resetAll();
    void healAllMonsters();
    bool isInFight();
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    void setPlayerMonster(const QList<PlayerMonster> &playerMonsterList);
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    QList<PlayerMonster> getPlayerMonster();
    bool removeMonster(const quint32 &monsterId);
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
    virtual PublicPlayerMonster *getOtherMonster();
    quint8 getOtherSelectedMonsterNumber();
    void setVariable(Player_private_and_public_informations player_informations);
private:
    QList<Skill::AttackReturn> attackReturnList;
    QList<PlayerMonster> playerMonsterList;
    Player_private_and_public_informations player_informations;
    void doTheCurrentMonsterAttack(const quint32 &skill);
    bool applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn);
    bool internalTryEscape();
    void addXPSP();
private:
    explicit ClientFightEngine();
    ~ClientFightEngine();
signals:
    void newError(QString error,QString detailedError);
    void error(QString error);
};
}

#endif // FightEngine_H
