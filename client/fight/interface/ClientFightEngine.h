#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QPixmap>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
#include "../../general/fight/CommonFightEngine.h"
#include "../../base/Api_protocol.h"

namespace CatchChallenger {
//only the logique here, store nothing
class ClientFightEngine : public QObject, public CommonFightEngine
{
    Q_OBJECT
public:
    struct MonsterSkillEffect
    {
        quint32 skill;
    };
    static ClientFightEngine fightEngine;
    virtual void resetAll();
    bool isInFight() const;
    void errorFightEngine(const QString &error);
    void messageFightEngine(const QString &message) const;
    //current fight
    QList<PublicPlayerMonster> battleCurrentMonster;
    QList<quint8> battleStat,botMonstersStat;
    QList<quint8> battleMonsterPlace;//is number with range of 1-max (2 if have 2 monster)
    QList<quint32> otherMonsterAttack;
    QList<PlayerMonster> playerMonster_catchInProgress;
    void fightFinished();
    void setBattleMonster(const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void setBotMonster(const QList<PlayerMonster> &publicPlayerMonster);
    bool addBattleMonster(const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    bool haveWin();
    void addAndApplyAttackReturnList(const QList<Skill::AttackReturn> &attackReturnList);
    QList<Skill::AttackReturn> getAttackReturnList() const;
    Skill::AttackReturn getFirstAttackReturn() const;
    void removeTheFirstLifeEffectAttackReturn();
    void removeTheFirstBuffEffectAttackReturn();
    void removeTheFirstAddBuffEffectAttackReturn();
    void removeTheFirstRemoveBuffEffectAttackReturn();
    void removeTheFirstAttackReturn();
    bool firstAttackReturnHaveMoreEffect();
    bool firstLifeEffectQuantityChange(qint32 quantity);
    virtual PublicPlayerMonster *getOtherMonster();
    quint8 getOtherSelectedMonsterNumber() const;
    void setVariableContent(Player_private_and_public_informations player_informations_local);
    Skill::AttackReturn generateOtherAttack();
    bool isInBattle() const;
    bool haveBattleOtherMonster() const;
    virtual bool useSkill(const quint32 &skill);
    bool dropKOOtherMonster();
    void tryCatchClient(const quint32 &item);
    virtual quint32 catchAWild(const bool &toStorage, const PlayerMonster &newMonster);
    void catchIsDone();
    bool doTheOtherMonsterTurn();
    PlayerMonster * evolutionByLevelUp();
    void confirmEvolution(const quint32 &monterId);
    bool giveXPSP(int xp,int sp);
    quint32 lastGivenXP();
    void newRandomNumber(const QByteArray &data);
private:
    quint32 mLastGivenXP;
    QList<int> mEvolutionByLevelUp;
    QList<Skill::AttackReturn> attackReturnList;
    Player_private_and_public_informations player_informations_local;
    QByteArray randomSeeds;
    Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    bool applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn);
    bool internalTryEscape();
    void levelUp(const quint8 &level,const quint8 &monsterIndex);
    void addXPSP();
    quint8 getOneSeed(const quint8 &max);
    quint32 randomSeedsSize() const;
private:
    explicit ClientFightEngine();
    ~ClientFightEngine();
signals:
    void newError(QString error,QString detailedError);
    void error(QString error);
    void message(QString message);
};
}

#endif // FightEngine_H
