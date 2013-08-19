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
    quint32 getPlayerId() const;
    void dissolvedClan();
    bool inviteToClan(const quint32 &clanId);
    void insertIntoAClan(const quint32 &clanId);
    void ejectToClan();
    quint32 getClanId() const;
    bool haveAClan() const;
    static void resetAll();
    //market
    static QList<quint16> marketObjectIdList;
private:
    bool checkCollision();

    struct Clan
    {
        QString captureCityInProgress;
        QString capturedCity;
        quint32 clanId;
        QList<LocalClientHandler *> players;

        //the db info
        bool haveTheInformations;
        QString name;
        quint64 cash;
    };
    struct CaptureCityValidated
    {
        QList<LocalClientHandler *> players;
        QList<LocalClientHandler *> playersInFight;
        QList<quint32> bots;
        QList<quint32> botsInFight;
        QHash<quint32,quint16> clanSize;
    };
    //info linked
    static Direction	temp_direction;
    static QHash<quint32,Clan *> clanList;
    static QHash<QString,LocalClientHandler *> playerByPseudo;
    static QHash<quint32,LocalClientHandler *> playerById;
    static QHash<QString,QList<LocalClientHandler *> > captureCity;
    static QHash<QString,CaptureCityValidated> captureCityValidatedList;

    //trade
    LocalClientHandler * otherPlayerTrade;
    bool tradeIsValidated;
    bool tradeIsFreezed;
    quint64 tradeCash;
    QHash<quint32,quint32> tradeObjects;
    QList<PlayerMonster> tradeMonster;
    QList<quint32> inviteToClanList;
    Clan *clan;

    //city
    LocalClientHandler * otherCityPlayerBattle;

    //map move
    bool singleMove(const Direction &direction);
    bool captureCityInProgress();
    //trade
    void internalTradeCanceled(const bool &send);
    void internalTradeAccepted(const bool &send);
    //other
    static MonsterDrops questItemMonsterToMonsterDrops(const Quest::ItemMonster &questItemMonster);
    bool otherPlayerIsInRange(LocalClientHandler * otherPlayer);
public slots:
    void put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
    void createMemoryClan();
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
    bool addMarketCashWithoutSave(const quint64 &cash,const double &bitcoin);
    void addCash(const quint64 &cash,const bool &forceSave=false);
    void removeCash(const quint64 &cash);
    void addBitcoin(const double &bitcoin);
    void removeBitcoin(const double &bitcoin);
    void addWarehouseCash(const quint64 &cash,const bool &forceSave=false);
    void removeWarehouseCash(const quint64 &cash);
    void wareHouseStore(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters);
    bool wareHouseStoreCheck(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters) const;
    void addWarehouseObject(const quint32 &item,const quint32 &quantity=1);
    quint32 removeWarehouseObject(const quint32 &item,const quint32 &quantity=1);

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
    //factory
    void saveIndustryStatus(const quint32 &factoryId,const IndustryStatus &industryStatus,const Industry &industry);
    static IndustryStatus factoryCheckProductionStart(const IndustryStatus &industryStatus,const Industry &industry);
    static IndustryStatus industryStatusWithCurrentTime(const IndustryStatus &industryStatus, const Industry &industry);
    quint32 getFactoryResourcePrice(const quint32 &quantityInStock,const Industry::Resource &resource,const Industry &industry);
    quint32 getFactoryProductPrice(const quint32 &quantityInStock,const Industry::Product &product,const Industry &industry);
    void getFactoryList(const quint32 &query_id,const quint32 &factoryId);
    void buyFactoryObject(const quint32 &query_id,const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellFactoryObject(const quint32 &query_id,const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
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
    void appendAllow(const ActionAllow &allow);
    void removeAllow(const ActionAllow &allow);
    void updateAllow();
    //reputation
    void appendReputationPoint(const QString &type,const qint32 &point);
    //battle
    void battleCanceled();
    void battleAccepted();
    void newRandomNumber(const QByteArray &randomData);
    bool tryEscape();
    void heal();
    void requestFight(const quint32 &fightId);
    void useSkill(const quint32 &skill);
    bool learnSkill(const quint32 &monsterId,const quint32 &skill);
    LocalClientHandlerFight * getLocalClientHandlerFight();
    //clan
    void clanAction(const quint8 &query_id,const quint8 &action,const QString &text);
    void haveClanInfo(const QString &clanName, const quint64 &cash);
    void sendClanInfo();
    void clanInvite(const bool &accept);
    void waitingForCityCaputre(const bool &cancel);
    quint32 clanId() const;
    void previousCityCaptureNotFinished();
    static void startTheCityCapture();
    void leaveTheCityCapture();
    void removeFromClan();
    void cityCaptureBattle(const quint16 &number_of_player,const quint16 &number_of_clan);
    void cityCaptureBotFight(const quint16 &number_of_player,const quint16 &number_of_clan,const quint32 &fightId);
    void cityCaptureInWait(const quint16 &number_of_player,const quint16 &number_of_clan);
    void cityCaptureWin();
    void fightOrBattleFinish(const bool &win,const quint32 &fightId);//fightId == 0 if is in battle
    static void cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated,const quint16 &number_of_player,const quint16 &number_of_clan);
    static quint16 cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated);
    static quint16 cityCaptureClanCount(const CaptureCityValidated &captureCityValidated);
    void moveMonster(const bool &up,const quint8 &number);
    void changeOfMonsterInFight(const quint32 &monsterId);
    //market
    void getMarketList(const quint32 &query_id);
    void buyMarketObject(const quint32 &query_id,const quint32 &marketObjectId,const quint32 &quantity);
    void buyMarketMonster(const quint32 &query_id,const quint32 &monsterId);
    void putMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity,const quint32 &price,const double &bitcoin);
    void putMarketMonster(const quint32 &query_id,const quint32 &monsterId,const quint32 &price,const double &bitcoin);
    inline bool bitcoinEnabled() const;
    void recoverMarketCash(const quint32 &query_id);
    void withdrawMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity);
    void withdrawMarketMonster(const quint32 &query_id, const quint32 &monsterId);
private slots:
    virtual void extraStop();
    void savePosition();
    static QString directionToStringToSave(const Direction &direction);
    static QString orientationToStringToSave(const Orientation &orientation);
    //quest
    bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest);
    bool haveStartQuestRequirement(const CatchChallenger::Quest &quest);
    bool nextStepQuest(const Quest &quest);
    bool startQuest(const Quest &quest);
    void addQuestStepDrop(const quint32 &questId,const quint8 &questStep);
    void removeQuestStepDrop(const quint32 &questId,const quint8 &questStep);
signals:
    void askClan(const quint32 &clanId);
    void dbQuery(const QString &sqlQuery) const;
    void askRandomNumber() const;
    void receiveSystemText(const QString &text,const bool &important=false) const;
    void postReply(const quint8 &queryNumber,const QByteArray &data) const;
    void sendTradeRequest(const QByteArray &data) const;
    void sendBattleRequest(const QByteArray &data) const;
    void clanChange(const quint32 &clanId) const;

    void seedValidated() const;
    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation) const;
};
}

#endif // CATCHCHALLENGER_LOCALPLAYERHANDLER_H
