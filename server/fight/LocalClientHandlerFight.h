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
    bool tryEscape();
    bool getBattleIsValidated();
    bool isInFight();
    void setVariable(Player_internal_informations *player_informations);
    LocalClientHandlerFight *otherPlayerBattle;
    void saveCurrentMonsterStat();
    bool checkLoose();
    bool getInBattle();
    bool learnSkillInternal(const quint32 &monsterId,const quint32 &skill);
    void getRandomNumberIfNeeded();
    bool botFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
    bool checkFightCollision(Map *map,const COORD_TYPE &x,const COORD_TYPE &y);
    void registerBattleRequest(LocalClientHandlerFight * otherPlayerBattle);
    //random linked signals
    //void newRandomNumber(const QByteArray &randomData);
public slots:
    void battleCanceled();
    void battleAccepted();
    void battleFinished();
    void battleFinishedReset();
protected:
    bool checkKOCurrentMonsters();
    bool checkKOOtherMonstersForGain();
    void syncForEndOfTurn();
    void saveStat();
    bool botFightStart(const quint32 &botFightId);
    bool tryEscapeInternal();
    void useSkill(const quint32 &skill);
    void useSkillAgainstWildMonster(const quint32 &skill,const quint8 &skillLevel);
    void useSkillAgainstBotMonster(const quint32 &skill,const quint8 &skillLevel);
    bool buffIsValid(const Skill::BuffEffect &buffEffect);
    Skill::AttackReturn doTheCurrentMonsterAttack(const quint32 &skill,const quint8 &skillLevel,const Monster::Stat &currentMonsterStat,const Monster::Stat &otherMonsterStat);
    bool haveBattleSkill();
    void haveUsedTheBattleSkill();
    void useBattleSkill(const quint32 &skill,const quint8 &skillLevel);
    void sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn,const quint8 &monsterPlace=0,const PublicPlayerMonster &publicPlayerMonster=PublicPlayerMonster());
    inline quint8 selectedMonsterNumberToMonsterPlace(const quint8 &selectedMonsterNumber);
    void internalBattleCanceled(const bool &send);
    void internalBattleAccepted(const bool &send);
    void resetTheBattle();
    virtual PublicPlayerMonster * getOtherMonster();
private:
    bool battleIsValidated;
    quint32 currentSkill;
    bool haveCurrentSkill;
    Player_internal_informations *player_informations;
    quint32 botFightCash;
    quint32 botFightId;
signals:
    void dbQuery(const QString &sqlQuery);
    void askRandomNumber();
    void receiveSystemText(const QString &text,const bool &important=false);
    void postReply(const quint8 &queryNumber,const QByteArray &data);
    void sendBattleRequest(const QByteArray &data);
    void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    void addObjectAndSend(const quint32 &item,const quint32 &quantity=1);
    void addCash(const quint64 &cash,const bool &forceSave=false);
};
}

#endif // CATCHCHALLENGER_LOCALPLAYERHANDLER_H
