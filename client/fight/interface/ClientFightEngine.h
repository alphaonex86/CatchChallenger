#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QString>

#include "../../../general/base/GeneralStructures.h"
#include "../../../general/base/CommonMap.h"
#include "../../../general/fight/CommonFightEngine.h"
#include "../../base/Api_protocol.h"

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
    static ClientFightEngine fightEngine;
    virtual void resetAll();
    virtual bool isInFight() const;
    bool useObjectOnMonsterByPosition(const uint16_t &object, const uint8_t &monsterPosition);
    void errorFightEngine(const std::string &error);
    void messageFightEngine(const std::string &message) const;
    std::vector<uint8_t> addPlayerMonster(const QList<PlayerMonster> &playerMonster);
    std::vector<uint8_t> addPlayerMonster(const std::vector<PlayerMonster> &playerMonster);
    std::vector<uint8_t> addPlayerMonster(const PlayerMonster &playerMonster);
    //current fight
    QList<PublicPlayerMonster> battleCurrentMonster;
    QList<uint8_t> battleStat,botMonstersStat;
    QList<uint8_t> battleMonsterPlace;//is number with range of 1-max (2 if have 2 monster)
    QList<uint32_t> otherMonsterAttack;
    QList<PlayerMonster> playerMonster_catchInProgress;
    virtual void fightFinished();
    void setBattleMonster(const QList<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void setBotMonster(const QList<PlayerMonster> &publicPlayerMonster);
    bool addBattleMonster(const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    bool haveWin();
    void addAndApplyAttackReturnList(const QList<Skill::AttackReturn> &fightEffectList);
    QList<Skill::AttackReturn> getAttackReturnList() const;
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
    bool dropKOOtherMonster();
    void tryCatchClient(const uint32_t &item);
    virtual uint32_t catchAWild(const bool &toStorage, const PlayerMonster &newMonster);
    void catchIsDone();
    bool doTheOtherMonsterTurn();
    PlayerMonster * evolutionByLevelUp();
    uint8_t getPlayerMonsterPosition(const PlayerMonster * const playerMonster);
    void confirmEvolutionByPosition(const uint8_t &monterPosition);
    void addToEncyclopedia(const uint16_t &monster);
    bool giveXPSP(int xp,int sp);
    uint32_t lastGivenXP();
    void newRandomNumber(const QByteArray &data);
private:
    uint32_t mLastGivenXP;
    QList<int> mEvolutionByLevelUp;
    QList<Skill::AttackReturn> fightEffectList;
    Player_private_and_public_informations player_informations_local;
    QByteArray randomSeeds;
    Skill::AttackReturn doTheCurrentMonsterAttack(const uint16_t &skill, const uint8_t &skillLevel);
    bool applyCurrentLifeEffectReturn(const Skill::LifeEffectReturn &effectReturn);
    bool internalTryEscape();
    void levelUp(const uint8_t &level,const uint8_t &monsterIndex);
    void addXPSP();
    uint8_t getOneSeed(const uint8_t &max);
    uint32_t randomSeedsSize() const;
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
