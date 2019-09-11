#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>

#include "../../../../general/base/GeneralStructures.h"
#include "../../../../general/base/CommonMap.h"
#include "../../../../general/fight/CommonFightEngine.h"
#include "../../Api_protocol_Qt.h"

namespace CatchChallenger {
//only the logique here, store nothing
class ClientFightEngine : public QObject, public CommonFightEngine
{
    Q_OBJECT
public:
    struct MonsterSkillEffect
    {
        uint32_t skill;
    };
    virtual void resetAll();
    virtual bool isInFight() const;
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    void errorFightEngine(const std::string &error);
    void messageFightEngine(const std::string &message) const;
    std::vector<uint8_t> addPlayerMonster(const std::vector<PlayerMonster> &playerMonster);
    std::vector<uint8_t> addPlayerMonster(const PlayerMonster &playerMonster);
    //current fight
    std::vector<PublicPlayerMonster> battleCurrentMonster;
    std::vector<uint8_t> battleStat,botMonstersStat;
    std::vector<uint8_t> battleMonsterPlace;//is number with range of 1-max (2 if have 2 monster)
    std::vector<uint32_t> otherMonsterAttack;
    std::vector<PlayerMonster> playerMonster_catchInProgress;
    virtual void fightFinished();
    void setBattleMonster(const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void setBotMonster(const std::vector<PlayerMonster> &publicPlayerMonster, const uint16_t &fightId);
    bool addBattleMonster(const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    bool haveWin();
    void addAndApplyAttackReturnList(const std::vector<Skill::AttackReturn> &fightEffectList);
    std::vector<Skill::AttackReturn> getAttackReturnList() const;
    Skill::AttackReturn getFirstAttackReturn() const;
    void removeTheFirstLifeEffectAttackReturn();
    void removeTheFirstBuffEffectAttackReturn();
    void removeTheFirstAddBuffEffectAttackReturn();
    void removeTheFirstRemoveBuffEffectAttackReturn();
    void removeTheFirstAttackReturn();
    bool firstAttackReturnHaveMoreEffect();
    bool firstLifeEffectQuantityChange(int32_t quantity);
    virtual PublicPlayerMonster *getOtherMonster();
    uint8_t getOtherSelectedMonsterNumber() const;
    void setVariableContent(Player_private_and_public_informations player_informations_local);
    Skill::AttackReturn generateOtherAttack();
    bool isInBattle() const;
    bool haveBattleOtherMonster() const;
    virtual bool useSkill(const uint16_t &skill);
    bool finishTheTurn(const bool &isBot);
    bool dropKOOtherMonster();
    bool tryCatchClient(const uint16_t &item);
    bool catchInProgress() const;
    virtual uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster);
    void catchIsDone();
    bool doTheOtherMonsterTurn();
    PlayerMonster * evolutionByLevelUp();
    uint8_t getPlayerMonsterPosition(const PlayerMonster * const playerMonster);
    void confirmEvolutionByPosition(const uint8_t &monterPosition);
    void addToEncyclopedia(const uint16_t &monster);
    bool giveXPSP(int xp,int sp);
    uint32_t lastGivenXP();
    void newRandomNumber(const std::string &data);
    void setClient(Api_protocol_Qt * client);
    uint32_t randomSeedsSize() const;
private:
    uint32_t mLastGivenXP;
    std::vector<uint8_t> mEvolutionByLevelUp;
    std::vector<Skill::AttackReturn> fightEffectList;
    Player_private_and_public_informations player_informations_local;
    std::string randomSeeds;
    Api_protocol_Qt * client;
    uint16_t fightId;
    Skill::AttackReturn doTheCurrentMonsterAttack(const uint16_t &skill, const uint8_t &skillLevel);
    bool applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn);
    bool internalTryEscape();
    void levelUp(const uint8_t &level,const uint8_t &monsterIndex);
    void addXPSP();
    uint8_t getOneSeed(const uint8_t &max);
//private:
public:
    explicit ClientFightEngine();
    ~ClientFightEngine();
signals:
    void newError(std::string error,std::string detailedError);
    void error(std::string error);
    void message(std::string message);
};
}

#endif // FightEngine_H
