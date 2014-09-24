#ifndef CommonFightEngine_H
#define CommonFightEngine_H

#include <QByteArray>
#include <QHash>
#include <QString>

#include "../base/GeneralStructures.h"
#include "../base/ClientBase.h"
#include "../../general/base/CommonMap.h"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngine : public ClientBase
{
public:
    CommonFightEngine();
    virtual void resetAll();
    virtual bool isInFight() const;
    bool isInFightWithWild() const;
    bool getAbleToFight() const;
    bool haveMonsters() const;
    static Monster::Stat getStat(const Monster &monster, const quint8 &level);
    PlayerMonster *getCurrentMonster();
    virtual PublicPlayerMonster *getOtherMonster();
    virtual Skill::AttackReturn generateOtherAttack();
    quint8 getCurrentSelectedMonsterNumber() const;
    quint8 getOtherSelectedMonsterNumber() const;
    virtual bool remainMonstersToFight(const quint32 &monsterId) const;
    virtual bool canDoRandomFight(const CommonMap &map,const quint8 &x,const quint8 &y) const;
    void updateCanDoFight();
    virtual bool haveAnotherMonsterOnThePlayerToFight() const;
    virtual bool haveAnotherEnnemyMonsterToFight();
    virtual bool otherMonsterIsKO();
    virtual bool genericMonsterIsKO(const PublicPlayerMonster * publicPlayerMonster) const;
    virtual bool currentMonsterIsKO();
    int addOtherBuffEffect(const Skill::BuffEffect &effect);
    virtual int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    bool dropKOCurrentMonster();
    virtual bool dropKOOtherMonster();
    virtual void healAllMonsters();
    virtual bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    virtual bool learnSkillByItem(PlayerMonster *playerMonster, const quint32 &itemId);
    virtual void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    virtual void addPlayerMonster(const PlayerMonster &playerMonster);
    virtual void insertPlayerMonster(const quint8 &place,const PlayerMonster &playerMonster);
    QList<PlayerMonster> getPlayerMonster() const;
    virtual bool moveUpMonster(const quint8 &number);
    virtual bool moveDownMonster(const quint8 &number);
    virtual bool removeMonster(const quint32 &monsterId);
    bool haveThisMonster(const quint32 &monsterId) const;
    PlayerMonster * monsterById(const quint32 &monsterId);
    virtual bool canEscape();
    virtual bool tryEscape();
    bool canDoFightAction();
    virtual bool useSkill(const quint32 &skill);
    quint8 getSkillLevel(const quint32 &skill);
    virtual quint8 decreaseSkillEndurance(const quint32 &skill);
    bool haveMoreEndurance();
    QList<Skill::LifeEffectReturn> applyBuffLifeEffect(PublicPlayerMonster * playerMonster);
    QList<Skill::BuffEffect> removeOldBuff(PublicPlayerMonster *playerMonster);
    static bool buffIsValid(const Skill::BuffEffect &buffEffect);
    virtual bool isInBattle() const = 0;
    //return true if now have wild monter to fight
    virtual bool generateWildFightIfCollision(CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y, const QHash<quint16, quint32> &items, const QList<quint8> &events);
    virtual bool doTheOtherMonsterTurn();
    virtual void doTheTurn(const quint32 &skill,const quint8 &skillLevel,const bool currentMonsterStatIsFirstToAttack);
    virtual bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    virtual quint32 tryCapture(const quint32 &item);
    virtual bool changeOfMonsterInFight(const quint32 &monsterId);
    virtual int addBuffEffectFull(const Skill::BuffEffect &effect,PublicPlayerMonster * currentMonster,PublicPlayerMonster * otherMonster);
    virtual void removeBuffEffectFull(const Skill::BuffEffect &effect);
    virtual bool useObjectOnMonster(const quint32 &object,const quint32 &monster);
    virtual void confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster);
    virtual void hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue);
    virtual bool removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId);
    virtual bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
protected:
    static ApplyOn invertApplyOn(const ApplyOn &applyOn);
    virtual PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList,bool *ok);
    static bool monsterIsKO(const PlayerMonster &playerMonter);
    Skill::LifeEffectReturn applyLifeEffect(const quint8 &type, const Skill::LifeEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster);
    virtual quint8 getOneSeed(const quint8 &max) = 0;
    virtual bool internalTryEscape();
    virtual bool internalTryCapture(const Trap &trap);
    virtual void fightFinished();
    virtual void wildDrop(const quint32 &monster);
    virtual bool checkKOOtherMonstersForGain();
    virtual bool giveXPSP(int xp,int sp);
    virtual bool addLevel(PlayerMonster * monster,const quint8 &numberOfLevel=1);
    virtual void levelUp(const quint8 &level,const quint8 &monsterIndex);
    virtual QList<Monster::AttackToLearn> autoLearnSkill(const quint8 &level,const quint8 &monsterIndex);
    virtual Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    Skill::AttackReturn genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const quint32 &skill, const quint8 &skillLevel);
    virtual quint32 catchAWild(const bool &toStorage, const PlayerMonster &newMonster) = 0;
    void startTheFight();
    virtual bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill);
    virtual bool setSkillLevel(PlayerMonster * currentMonster,const int &index,const quint8 &level);
    virtual bool removeSkill(PlayerMonster * currentMonster,const int &index);
protected:
    virtual void errorFightEngine(const QString &error) = 0;
    virtual void messageFightEngine(const QString &message) const = 0;
    virtual quint32 randomSeedsSize() const = 0;
protected:
    bool ableToFight;
    QList<PlayerMonster> wildMonsters,botFightMonsters;
    quint8 stepFight;
    int selectedMonster;
    bool doTurnIfChangeOfMonster;
};
}

#endif // CommonFightEngine_H
