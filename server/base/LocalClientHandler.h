#include "ClientMapManagement/MapBasicMove.h"

#ifndef CATCHCHALLENGER_LOCALPLAYERHANDLER_H
#define CATCHCHALLENGER_LOCALPLAYERHANDLER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QHash>
#include <QHashIterator>
#include <QSqlQuery>
#include <QPair>

#include "../../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "EventThreader.h"
#include "../VariableServer.h"
#include "MapServer.h"
#include "SqlFunction.h"
#include "../fight/LocalClientHandlerFight.h"

/** \brief Do here only the calcule local to the client
 * That's mean map collision, monster event into grass, fight, object usage, ...
 * no access to other client -> broadcast, no file/db access, no visibility calcule, ...
 * Only here you need use the random list */

namespace CatchChallenger {
class LocalClientHandler : public MapBasicMove
{
    Q_OBJECT
public:
    explicit LocalClientHandler();
    virtual ~LocalClientHandler();
    virtual void setVariable(Player_internal_informations *player_informations);
    bool getInTrade();
    void registerTradeRequest(LocalClientHandler * otherPlayerTrade);
    void registerBattleRequest(LocalClientHandler * otherPlayerTrade);
    bool getIsFreezed();
    quint64 getTradeCash();
    QHash<quint32,quint32> getTradeObjects();
    QList<PlayerMonster> getTradeMonster();
    void resetTheTrade();
    void addExistingMonster(QList<PlayerMonster> tradeMonster);
    bool getAbleToFight();
    PlayerMonster &getSelectedMonster();
    quint8 getSelectedMonsterNumber();
    PlayerMonster& getEnemyMonster();
    LocalClientHandlerFight localClientHandlerFight;
private:
    bool checkCollision();

    //info linked
    static Direction	temp_direction;
    static QHash<QString,LocalClientHandler *> playerByPseudo;
    //trade
    LocalClientHandler * otherPlayerTrade;
    bool tradeIsValidated;
    bool tradeIsFreezed;
    quint64 tradeCash;
    QHash<quint32,quint32> tradeObjects;
    QList<PlayerMonster> tradeMonster;

    //map move
    bool singleMove(const Direction &direction);
    //trade
    void internalTradeCanceled(const bool &send);
    void internalTradeAccepted(const bool &send);
    //other
    static MonsterDrops questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster);
    bool otherPlayerIsInRange(LocalClientHandler * otherPlayer);
public slots:
    void put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
    bool moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    Direction lookToMove(const Direction &direction);
    //seed
    void useSeed(const quint8 &plant_id);
    //crafting
    void useRecipe(const quint8 &query_id,const quint32 &recipe_id);
    //inventory
    void addObjectAndSend(const quint32 &item,const quint32 &quantity=1);
    void addObject(const quint32 &item,const quint32 &quantity=1);
    void saveObjectRetention(const quint32 &item);
    quint32 removeObject(const quint32 &item,const quint32 &quantity=1);
    void sendRemoveObject(const quint32 &item,const quint32 &quantity=1);
    quint32 objectQuantity(const quint32 &item);
    void addCash(const quint64 &cash,const bool &forceSave=false);
    void removeCash(const quint64 &cash);

    void sendHandlerCommand(const QString &command,const QString &extraText);
    //inventory
    void destroyObject(const quint32 &itemId,const quint32 &quantity);
    void useObject(const quint8 &query_id,const quint32 &itemId);
    //teleportation
    void receiveTeleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    virtual void teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    //shop
    void getShopList(const quint32 &query_id,const quint32 &shopId);
    void buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    //trade
    void tradeCanceled();
    void tradeAccepted();
    void tradeFinished();
    void tradeAddTradeCash(const quint64 &cash);
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity);
    void tradeAddTradeMonster(const quint32 &monsterId);
    //quest
    void newQuestAction(const QuestAction &action,const quint32 &questId);
    static bool addQuestStepDrop(Player_internal_informations *player_informations, const quint32 &questId, const quint8 &step);
    static bool removeQuestStepDrop(Player_internal_informations *player_informations,const quint32 &questId,const quint8 &step);
    //reputation
    void appendReputationPoint(const QString &type,const qint32 &point);
    //battle
    void battleCanceled();
    void battleAccepted();
    void newRandomNumber(const QByteArray &randomData);
    bool tryEscape();
    void useSkill(const quint32 &skill);
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
private slots:
    virtual void extraStop();
    void savePosition();
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest);
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    void addQuestStepDrop(const quint32 &questId,const quint8 &questStep);
    void removeQuestStepDrop(const quint32 &questId,const quint8 &questStep);
signals:
    void dbQuery(const QString &sqlQuery);
    void askRandomNumber();
    void receiveSystemText(const QString &text,const bool &important=false);
    void postReply(const quint8 &queryNumber,const QByteArray &data);
    void sendTradeRequest(const QByteArray &data);
    void sendBattleRequest(const QByteArray &data);

    void seedValidated();
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
};
}

#endif // CATCHCHALLENGER_LOCALPLAYERHANDLER_H
