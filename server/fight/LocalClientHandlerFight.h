#ifndef CATCHCHALLENGER_LOCALPLAYERHANDLERFIGHT_H
#define CATCHCHALLENGER_LOCALPLAYERHANDLERFIGHT_H

#include <QObject>
#include <QList>
#include <QString>
#include <QHash>
#include <QHashIterator>
#include <QSqlQuery>
#include <QPair>

#include "../../general/fight/CommonFightEngine.h"
#include "../base/ServerStructures.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...
 * no access to other client -> broadcast, no file/db access, no visibility calcule, ...
 * Only here you need use the random list */

namespace CatchChallenger {
class LocalClientHandlerFight : public CommonFightEngine
{
    Q_OBJECT
public:
    explicit LocalClientHandlerFight();
    virtual ~LocalClientHandlerFight();
    bool getBattleIsValidated();
    bool isInFight() const;
    void setVariableInternal(Player_internal_informations *player_informations);
    void saveCurrentMonsterStat();
    void saveMonsterStat(const PlayerMonster &monster);
    void savePosition();
    bool checkLoose();
    bool isInBattle() const;
    bool learnSkillInternal(const quint32 &monsterId,const quint32 &skill);
    void getRandomNumberIfNeeded() const;
    bool tryEscape();
    quint32 tryCapture(const quint32 &item);
    bool botFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
    bool checkFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
    void registerBattleRequest(LocalClientHandlerFight * otherPlayerBattle);
    //random linked signals
    //void newRandomNumber(const QByteArray &randomData);
    void battleCanceled();
    void battleAccepted();
    void battleFinished();
    void battleFinishedReset();
    LocalClientHandlerFight * getOtherPlayerBattle() const;
    bool finishTheTurn(const bool &isBot);
    virtual bool useSkill(const quint32 &skill);
    virtual bool currentMonsterAttackFirst(const PlayerMonster * currentMonster,const PublicPlayerMonster * otherMonster) const;
    void requestFight(const quint32 &fightId);
    void healAllMonsters();
    void battleFakeAccepted(LocalClientHandlerFight * otherPlayer);
    void battleFakeAcceptedInternal(LocalClientHandlerFight * otherPlayer);
    bool botFightStart(const quint32 &botFightId);
    void setInCityCapture(const bool &isInCityCapture);
    int addCurrentBuffEffect(const Skill::BuffEffect &effect);
    bool moveUpMonster(const quint8 &number);
    bool moveDownMonster(const quint8 &number);
    void saveMonsterPosition(const quint32 &monsterId,const quint8 &monsterPosition);
    bool changeOfMonsterInFight(const quint32 &monsterId);
    bool doTheOtherMonsterTurn();
    Skill::AttackReturn generateOtherAttack();
    Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill, const quint8 &skillLevel);
    quint8 decreaseSkillEndurance(const quint32 &skill);
    void confirmEvolution(const quint32 &monterId);
    void emitBattleWin();
    virtual void hpChange(PlayerMonster * currentMonster, const quint32 &newHpValue);
    bool removeBuffOnMonster(PlayerMonster * currentMonster, const quint32 &buffId);
    bool removeAllBuffOnMonster(PlayerMonster * currentMonster);
protected:
    bool checkKOCurrentMonsters();
    void syncForEndOfTurn();
    void saveStat();
    bool buffIsValid(const Skill::BuffEffect &buffEffect);
    bool haveBattleAction() const;
    void resetBattleAction();
    quint8 getOtherSelectedMonsterNumber() const;
    void haveUsedTheBattleAction();
    void sendBattleReturn();
    void sendBattleMonsterChange();
    inline quint8 selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber);
    void internalBattleCanceled(const bool &send);
    void internalBattleAccepted(const bool &send);
    void resetTheBattle();
    virtual PublicPlayerMonster *getOtherMonster() const;
    virtual void fightFinished();
    virtual bool giveXPSP(int xp,int sp);
    bool useSkillAgainstBotMonster(const quint32 &skill, const quint8 &skillLevel);
    virtual void wildDrop(const quint32 &monster);
    virtual quint8 getOneSeed(const quint8 &max);
    bool bothRealPlayerIsReady() const;
    bool checkIfCanDoTheTurn();
    bool dropKOOtherMonster();
    quint32 captureAWild(const bool &toStorage, const PlayerMonster &newMonster);
    bool haveCurrentSkill() const;
    quint32 getCurrentSkill() const;
    bool haveMonsterChange() const;
private:
    LocalClientHandlerFight *otherPlayerBattle;
    bool battleIsValidated;
    quint32 mCurrentSkillId;
    bool mHaveCurrentSkill,mMonsterChange;
    Player_internal_informations *player_informations;
    quint32 botFightCash;
    quint32 botFightId;
    bool isInCityCapture;
    QList<Skill::AttackReturn> attackReturn;
signals:
    void dbQuery(const QString &sqlQuery) const;
    void askRandomNumber() const;
    void receiveSystemText(const QString &text,const bool &important=false) const;
    void postReply(const quint8 &queryNumber,const QByteArray &data) const;
    void sendBattleRequest(const QByteArray &data) const;
    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray()) const;
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray()) const;
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation) const;
    void addObjectAndSend(const quint32 &item,const quint32 &quantity=1) const;
    void addCash(const quint64 &cash,const bool &forceSave=false) const;
    void fightOrBattleFinish(const bool &win,const quint32 &fightId);
};
}

#endif // CATCHCHALLENGER_LOCALPLAYERHANDLER_H
