#ifndef CommonFightEngine_H
#define CommonFightEngine_H

#include <unordered_map>
#include <string>
#include <vector>

#include "../base/GeneralStructures.h"
#include "../base/ClientBase.h"
#include "../../general/base/CommonMap.h"
#include "CommonFightEngineBase.h"

namespace CatchChallenger {
//only the logique here, store nothing
class CommonFightEngine : public ClientBase, public CommonFightEngineBase
{
public:
    CommonFightEngine();
    virtual void resetAll();
    virtual bool isInFight() const;
    bool isInFightWithWild() const;
    bool getAbleToFight() const;
    bool haveMonsters() const;
    PlayerMonster *getCurrentMonster();
    virtual PublicPlayerMonster *getOtherMonster();
    virtual Skill::AttackReturn generateOtherAttack();
    uint8_t getCurrentSelectedMonsterNumber() const;
    uint8_t getOtherSelectedMonsterNumber() const;
    virtual bool remainMonstersToFightWithoutThisMonster(const uint8_t &monsterPosition) const;
    virtual bool canDoRandomFight(const CommonMap &map,const uint8_t &x,const uint8_t &y) const;
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
    virtual bool learnSkill(PlayerMonster *monsterPlayer, const uint16_t &skill);
    virtual bool learnSkillByItem(PlayerMonster *playerMonster, const uint32_t &itemId);
    virtual void addPlayerMonster(const std::vector<PlayerMonster> &playerMonster);
    virtual void addPlayerMonster(const PlayerMonster &playerMonster);
    virtual void insertPlayerMonster(const uint8_t &place,const PlayerMonster &playerMonster);
    std::vector<PlayerMonster> getPlayerMonster() const;
    virtual bool moveUpMonster(const uint8_t &number);
    virtual bool moveDownMonster(const uint8_t &number);
    virtual bool removeMonster(const uint32_t &monsterId);
    bool haveThisMonster(const uint32_t &monsterId) const;
    PlayerMonster * monsterById(const uint32_t &monsterId);
    PlayerMonster * monsterByPosition(const uint8_t &monsterPosition);
    virtual bool canEscape();
    virtual bool tryEscape();
    bool canDoFightAction();
    virtual bool useSkill(const uint32_t &skill);
    uint8_t getSkillLevel(const uint32_t &skill);
    virtual uint8_t decreaseSkillEndurance(const uint32_t &skill);
    bool haveMoreEndurance();
    std::vector<Skill::LifeEffectReturn> applyBuffLifeEffect(PublicPlayerMonster * playerMonster);
    std::vector<Skill::BuffEffect> removeOldBuff(PublicPlayerMonster *playerMonster);
    virtual bool isInBattle() const = 0;
    //return true if now have wild monter to fight
    virtual bool generateWildFightIfCollision(CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y, const std::unordered_map<uint16_t, uint32_t> &items, const std::vector<uint8_t> &events);
    virtual bool doTheOtherMonsterTurn();
    virtual void doTheTurn(const uint32_t &skill,const uint8_t &skillLevel,const bool currentMonsterStatIsFirstToAttack);
    virtual bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    virtual uint32_t tryCapture(const uint16_t &item);
    virtual bool changeOfMonsterInFight(const uint8_t &monsterPosition);
    virtual int addBuffEffectFull(const Skill::BuffEffect &effect,PublicPlayerMonster * currentMonster,PublicPlayerMonster * otherMonster);
    virtual void removeBuffEffectFull(const Skill::BuffEffect &effect);
    virtual bool useObjectOnMonster(const uint16_t &object, const uint32_t &monster);
    virtual void confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster);
    virtual void hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue);
    virtual bool removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId);
    virtual bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
protected:
    static ApplyOn invertApplyOn(const ApplyOn &applyOn);
    virtual PlayerMonster getRandomMonster(const std::vector<MapMonster> &monsterList,bool *ok);
    static bool monsterIsKO(const PlayerMonster &playerMonter);
    Skill::LifeEffectReturn applyLifeEffect(const uint8_t &type, const Skill::LifeEffect &effect, PublicPlayerMonster *currentMonster, PublicPlayerMonster *otherMonster);
    virtual uint8_t getOneSeed(const uint8_t &max) = 0;
    virtual bool internalTryEscape();
    virtual bool internalTryCapture(const Trap &trap);
    virtual void fightFinished();
    virtual void wildDrop(const uint32_t &monster);
    virtual bool checkKOOtherMonstersForGain();
    virtual bool giveXPSP(int xp,int sp);
    virtual bool addLevel(PlayerMonster * monster,const uint8_t &numberOfLevel=1);
    virtual void levelUp(const uint8_t &level,const uint8_t &monsterIndex);
    virtual std::vector<Monster::AttackToLearn> autoLearnSkill(const uint8_t &level,const uint8_t &monsterIndex);
    virtual Skill::AttackReturn doTheCurrentMonsterAttack(const uint32_t &skill, const uint8_t &skillLevel);
    Skill::AttackReturn genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const uint32_t &skill, const uint8_t &skillLevel);
    virtual uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster) = 0;
    void startTheFight();
    virtual bool addSkill(PlayerMonster * currentMonster,const PlayerMonster::PlayerSkill &skill);
    virtual bool setSkillLevel(PlayerMonster * currentMonster,const unsigned int &index,const uint8_t &level);
    virtual bool removeSkill(PlayerMonster * currentMonster,const unsigned int &index);
protected:
    virtual void errorFightEngine(const std::string &error) = 0;
    virtual void messageFightEngine(const std::string &message) const = 0;
    virtual uint32_t randomSeedsSize() const = 0;
protected:
    bool ableToFight;
    std::vector<PlayerMonster> wildMonsters,botFightMonsters;
    uint8_t stepFight;
    int selectedMonster;
    bool doTurnIfChangeOfMonster;
};
}

#endif // CommonFightEngine_H
