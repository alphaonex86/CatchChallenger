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
    PlayerMonster * getCurrentMonster();//no const due to error message
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
    virtual std::vector<uint8_t> addPlayerMonster(const std::vector<PlayerMonster> &playerMonster);
    virtual std::vector<uint8_t> addPlayerMonster(const PlayerMonster &playerMonster);
    virtual void insertPlayerMonster(const uint8_t &place,const PlayerMonster &playerMonster);
    std::vector<PlayerMonster> getPlayerMonster() const;
    virtual bool moveUpMonster(const uint8_t &number);
    virtual bool moveDownMonster(const uint8_t &number);
    virtual bool removeMonsterByPosition(const uint8_t &monsterPosition);
    bool haveThisMonsterByPosition(const uint8_t &monsterPosition) const;
    PlayerMonster * monsterByPosition(const uint8_t &monsterPosition);
    virtual bool canEscape();
    virtual bool tryEscape();
    bool canDoFightAction();
    virtual bool useSkill(const uint16_t &skill);
    uint8_t getSkillLevel(const uint16_t &skill);//no const due to error message
    bool haveTheSkill(const uint16_t &skill);//no const due to error message
    PlayerMonster::PlayerSkill * getTheSkill(const uint16_t &skill);//no const due to error message
    virtual uint8_t decreaseSkillEndurance(PlayerMonster::PlayerSkill * skill);
    bool haveMoreEndurance();
    std::vector<Skill::LifeEffectReturn> applyBuffLifeEffect(PublicPlayerMonster * playerMonster);
    std::vector<Skill::BuffEffect> removeOldBuff(PublicPlayerMonster *playerMonster);
    virtual bool isInBattle() const = 0;
    //return true if now have wild monter to fight
    virtual bool generateWildFightIfCollision(const CommonMap *map, const COORD_TYPE &x, const COORD_TYPE &y,
                                              #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
                                              const std::unordered_map<uint16_t, uint32_t> &items
                                              #else
                                              const std::map<uint16_t, uint32_t> &items
                                              #endif
                                              , const std::vector<uint8_t> &events);
    virtual bool doTheOtherMonsterTurn();
    virtual void doTheTurn(const uint16_t &skill,const uint8_t &skillLevel,const bool currentMonsterStatIsFirstToAttack);
    virtual bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    virtual uint32_t tryCapture(const uint16_t &item);
    virtual bool changeOfMonsterInFight(const uint8_t &monsterPosition);
    virtual int addBuffEffectFull(const Skill::BuffEffect &effect,PublicPlayerMonster * currentMonster,PublicPlayerMonster * otherMonster);
    virtual void removeBuffEffectFull(const Skill::BuffEffect &effect);
    virtual bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    virtual void confirmEvolutionTo(PlayerMonster * playerMonster,const uint32_t &monster);
    virtual void hpChange(PlayerMonster * currentMonster, const uint32_t &newHpValue);
    virtual bool removeBuffOnMonster(PlayerMonster * currentMonster, const uint32_t &buffId);
    virtual bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
    virtual bool haveBeatBot(const uint16_t &botFightId) const;
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
    virtual Skill::AttackReturn doTheCurrentMonsterAttack(const uint16_t &skill, const uint8_t &skillLevel);
    Skill::AttackReturn genericMonsterAttack(PublicPlayerMonster *currentMonster,PublicPlayerMonster *otherMonster,const uint16_t &skill, const uint8_t &skillLevel);
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
