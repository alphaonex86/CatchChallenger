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
    void resetAll();
    void setVariable(Player_private_and_public_informations *player_informations);
    bool isInFight() const;
    bool isInFightWithWild() const;
    bool getAbleToFight() const;
    bool haveMonsters() const;
    static Monster::Stat getStat(const Monster &monster, const quint8 &level);
    PlayerMonster *getCurrentMonster();
    PublicPlayerMonster *getOtherMonster();
    Skill::AttackReturn generateOtherAttack();
    quint8 getCurrentSelectedMonsterNumber() const;
    quint8 getOtherSelectedMonsterNumber() const;
    bool remainMonstersToFight(const quint32 &monsterId) const;
    bool canDoRandomFight(const CommonMap &map,const quint8 &x,const quint8 &y) const;
    void updateCanDoFight();
    bool haveAnotherMonsterOnThePlayerToFight() const;
    bool haveAnotherEnnemyMonsterToFight();
    bool otherMonsterIsKO();
    bool genericMonsterIsKO(const PublicPlayerMonster * publicPlayerMonster) const;
    bool currentMonsterIsKO();
    int addOtherBuffEffect(const Skill::BuffEffect &effect);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    bool dropKOCurrentMonster();
    bool dropKOOtherMonster();
    void healAllMonsters();
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    bool learnSkillByItem(PlayerMonster *playerMonster, const quint32 &itemId);
    void addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    void addPlayerMonster(const PlayerMonster &playerMonster);
    void insertPlayerMonster(const quint8 &place,const PlayerMonster &playerMonster);
    QList<PlayerMonster> getPlayerMonster() const;
    bool moveUpMonster(const quint8 &number);
    bool moveDownMonster(const quint8 &number);
    bool removeMonster(const quint32 &monsterId);
    bool haveThisMonster(const quint32 &monsterId) const;
    PlayerMonster * monsterById(const quint32 &monsterId);
    bool canEscape();
    bool tryEscape();
    bool canDoFightAction();
    bool useSkill(const quint32 &skill);
    quint8 getSkillLevel(const quint32 &skill);
    quint8 decreaseSkillEndurance(const quint32 &skill);
    bool haveMoreEndurance();
    QList<Skill::LifeEffectReturn> buffLifeEffect(PublicPlayerMonster * playerMonster);
    QList<Skill::BuffEffect> removeOldBuff(PublicPlayerMonster *playerMonster);
    static bool buffIsValid(const Skill::BuffEffect &buffEffect);
    virtual bool isInBattle() const = 0;
    //return true if now have wild monter to fight
    bool generateWildFightIfCollision(CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y, const QHash<quint32, quint32> &items, const QList<quint8> &events);
    bool doTheOtherMonsterTurn();
    void doTheTurn(const quint32 &skill,const quint8 &skillLevel,const bool currentMonsterStatIsFirstToAttack);
    bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    quint32 tryCapture(const quint32 &item);
    bool changeOfMonsterInFight(const quint32 &monsterId);
    int addBuffEffectFull(const Skill::BuffEffect &effect,PublicPlayerMonster * currentMonster,PublicPlayerMonster * otherMonster);
    void removeBuffEffectFull(const Skill::BuffEffect &effect);
    bool useObjectOnMonster(const quint32 &object,const quint32 &monster);
    void confirmEvolutionTo(PlayerMonster * playerMonster,const quint32 &monster);
    void hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue);
    bool removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId);
    bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
protected:
    static ApplyOn invertApplyOn(const ApplyOn &applyOn);
    PlayerMonster getRandomMonster(const QList<MapMonster> &monsterList,bool *ok);
    static bool monsterIsKO(const PlayerMonster &playerMonter);
    Skill::LifeEffectReturn applyLifeEffect(const quint8 &type, const Skill::LifeEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster);
    virtual quint8 getOneSeed(const quint8 &max) = 0;
    bool internalTryEscape();
    bool internalTryCapture(const Trap &trap);
    void fightFinished();
    void wildDrop(const quint32 &monster);
    bool checkKOOtherMonstersForGain();
    bool giveXPSP(int xp,int sp);
    bool addLevel(PlayerMonster * monster,const quint8 &numberOfLevel=1);
    void levelUp(const quint8 &level,const quint8 &monsterIndex);
    Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    Skill::AttackReturn genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const quint32 &skill, const quint8 &skillLevel);
    virtual quint32 catchAWild(const bool &toStorage, const PlayerMonster &newMonster) = 0;
    void startTheFight();
    bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill);
    bool setSkillLevel(PlayerMonster * currentMonster,const int &index,const quint8 &level);
    bool removeSkill(PlayerMonster * currentMonster,const int &index);
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
