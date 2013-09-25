#ifndef CommonFightEngine_H
#define CommonFightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../base/Api_protocol.h"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngine : public QObject
{
    Q_OBJECT
public:
    CommonFightEngine();
    virtual void resetAll();
    virtual void setVariable(Player_private_and_public_informations *player_informations);
    virtual bool isInFight() const;
    bool isInFightWithWild() const;
    virtual bool getAbleToFight() const;
    bool haveMonsters() const;
    static Monster::Stat getStat(const Monster &monster, const quint8 &level);
    PlayerMonster *getCurrentMonster() const;
    virtual PublicPlayerMonster *getOtherMonster() const;
    virtual Skill::AttackReturn generateOtherAttack();
    quint8 getCurrentSelectedMonsterNumber() const;
    virtual quint8 getOtherSelectedMonsterNumber() const;
    bool remainMonstersToFight(const quint32 &monsterId) const;
    virtual bool canDoRandomFight(const Map &map,const quint8 &x,const quint8 &y) const;
    void updateCanDoFight();
    virtual bool haveAnotherMonsterOnThePlayerToFight() const;
    virtual bool haveAnotherEnnemyMonsterToFight() const;
    bool otherMonsterIsKO() const;
    bool genericMonsterIsKO(const PublicPlayerMonster * publicPlayerMonster) const;
    bool currentMonsterIsKO() const;
    int addOtherBuffEffect(const Skill::BuffEffect &effect);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    virtual bool dropKOCurrentMonster();
    virtual bool dropKOOtherMonster();
    virtual void healAllMonsters();
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    void addPlayerMonster(const PlayerMonster &playerMonster);
    void insertPlayerMonster(const quint8 &place,const PlayerMonster &playerMonster);
    QList<PlayerMonster> getPlayerMonster() const;
    virtual bool moveUpMonster(const quint8 &number);
    virtual bool moveDownMonster(const quint8 &number);
    bool removeMonster(const quint32 &monsterId);
    bool canEscape() const;
    virtual bool tryEscape();
    bool canDoFightAction();
    virtual bool useSkill(const quint32 &skill);
    quint8 getSkillLevel(const quint32 &skill) const;
    virtual quint8 decreaseSkillEndurance(const quint32 &skill);
    bool haveMoreEndurance() const;
    QList<Skill::LifeEffectReturn> buffLifeEffect(PublicPlayerMonster * playerMonster);
    QList<Skill::BuffEffect> removeOldBuff(PublicPlayerMonster *playerMonster);
    static bool buffIsValid(const Skill::BuffEffect &buffEffect);
    virtual bool isInBattle() const = 0;
    //return true if now have wild monter to fight
    bool generateWildFightIfCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
    virtual bool doTheOtherMonsterTurn();
    void doTheTurn(const quint32 &skill,const quint8 &skillLevel,const bool currentMonsterStatIsFirstToAttack);
    virtual bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    virtual bool tryCapture(const quint32 &item);
    virtual bool changeOfMonsterInFight(const quint32 &monsterId);
    int addBuffEffectFull(const Skill::BuffEffect &effect,PublicPlayerMonster * currentMonster,PublicPlayerMonster * otherMonster);
    void removeBuffEffectFull(const Skill::BuffEffect &effect);
public slots:
    void newRandomNumber(const QByteArray &randomData);
protected:
    static ApplyOn invertApplyOn(const ApplyOn &applyOn);
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList,bool *ok);
    static bool monsterIsKO(const PlayerMonster &playerMonter);
    Skill::LifeEffectReturn applyLifeEffect(const Skill::LifeEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster);
    virtual quint8 getOneSeed(const quint8 &max);
    virtual bool internalTryEscape();
    bool internalTryCapture(const Trap &trap);
    virtual void fightFinished();
    virtual void wildDrop(const quint32 &monster);
    virtual bool checkKOOtherMonstersForGain();
    virtual bool giveXPSP(int xp,int sp);
    virtual Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    Skill::AttackReturn genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const quint32 &skill, const quint8 &skillLevel);
    virtual void captureAWild(const bool &toStorage, const PlayerMonster &newMonster) = 0;
    virtual void startTheFight();
signals:
    void error(const QString &error) const;
    void message(const QString &message) const;
protected:
    Player_private_and_public_informations *player_informations;
    bool ableToFight;
    QList<PlayerMonster> wildMonsters,botFightMonsters;
    quint8 stepFight_Grass,stepFight_Water,stepFight_Cave;
    QByteArray randomSeeds;
    int selectedMonster;
    bool doTurnIfChangeOfMonster;
};
}

#endif // CommonFightEngine_H
