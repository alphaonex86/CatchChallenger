#ifndef CATCHCHALLENGER_PROTOCOL_H
#define CATCHCHALLENGER_PROTOCOL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/MoveOnTheMap.h"

namespace CatchChallenger {
class Api_protocol : public ProtocolParsingInput, public MoveOnTheMap
{
    Q_OBJECT
public:
    explicit Api_protocol(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_protocol();

    //protocol command
    bool tryLogin(const QString &login,const QString &pass);
    bool sendProtocol();

    //get the stored data
    Player_private_and_public_informations get_player_informations();
    QString getPseudo();
    quint16 getId();
    quint64 getTXSize();

    virtual void sendDatapackContent() = 0;
    virtual void tryDisconnect() = 0;
    QString get_datapack_base_name();

    bool getHavePlantAction();
    bool getHaveShopAction();
    bool getHaveFactoryAction();

    //to reset all
    void resetAll();

    //to manipulate the monsters
    Player_private_and_public_informations player_informations;

    void startReadData();
private:
    //status for the query
    bool is_logged;
    bool have_send_protocol;
    bool have_receive_protocol;
    bool tolerantMode;

    //to send trame
    quint8 lastQueryNumber;
protected:
    //have message without reply
    virtual void parseMessage(const quint8 &mainCodeType,const QByteArray &data);
    virtual void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //have query with reply
    virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    virtual void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    virtual void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    virtual void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);

    //error
    void parseError(const QString &userMessage, const QString &errorString);

    //general data
    virtual void defineMaxPlayers(const quint16 &maxPlayers) = 0;

    //stored local player info
    quint16 max_player;
    quint32 number_of_map;

    //to send trame
    ProtocolParsingOutput *output;
    quint8 queryNumber();

    //inventory
    QList<quint32> lastObjectUsed;

    //datapack
    QString datapack_base_name;

    bool havePlantAction;
    bool haveShopAction;
    bool haveFactoryAction;

    //teleport list query id
    QList<quint8> teleportList;

    //trade
    QList<quint32> tradeRequestId;
    bool isInTrade;
    //battle
    QList<quint32> battleRequestId;
    bool isInBattle;
signals:
    void newError(const QString &error,const QString &detailedError) const;

    //protocol/connection info
    void disconnected(const QString &reason) const;
    void notLogged(const QString &reason) const;
    void logged() const;
    void protocol_is_good() const;

    //general info
    void number_of_player(const quint16 &number,const quint16 &max_player) const;
    void random_seeds(const QByteArray &data) const;

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction) const;
    void move_player(const quint16 &id,const QList<QPair<quint8,CatchChallenger::Direction> > &movement) const;
    void remove_player(const quint16 &id) const;
    void reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction) const;
    void full_reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 y,const CatchChallenger::Direction &direction) const;
    void dropAllPlayerOnTheMap() const;
    void teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction) const;

    //plant
    void insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature) const;
    void remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y) const;
    void seed_planted(const bool &ok) const;
    void plant_collected(const CatchChallenger::Plant_collect &stat) const;
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage) const;
    //inventory
    void objectUsed(const ObjectUsage &objectUsage) const;
    void monsterCatch(const quint32 &newMonsterId) const;

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type) const;
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const QString &text) const;

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) const;
    void have_inventory(const QHash<quint32,quint32> &items,const QHash<quint32,quint32> &warehouse_items) const;
    void add_to_inventory(const QHash<quint32,quint32> &items) const;
    void remove_to_inventory(const QHash<quint32,quint32> &items) const;

    //datapack
    void haveTheDatapack() const;
    void newFile(const QString &fileName,const QByteArray &data,const quint64 &mtime) const;
    void removeFile(const QString &fileName) const;

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items) const;
    void haveBuyObject(const BuyStat &stat,const quint32 &newPrice) const;
    void haveSellObject(const SoldStat &stat,const quint32 &newPrice) const;

    //factory
    void haveFactoryList(const QList<ItemToSellOrBuy> &resources,const QList<ItemToSellOrBuy> &products) const;
    void haveBuyFactoryObject(const BuyStat &stat,const quint32 &newPrice) const;
    void haveSellFactoryObject(const SoldStat &stat,const quint32 &newPrice) const;

    //trade
    void tradeRequested(const QString &pseudo,const quint8 &skinInt) const;
    void tradeAcceptedByOther(const QString &pseudo,const quint8 &skinInt) const;
    void tradeCanceledByOther() const;
    void tradeFinishedByOther() const;
    void tradeValidatedByTheServer() const;
    void tradeAddTradeCash(const quint64 &cash) const;
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity) const;
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster) const;

    //battle
    void battleRequested(const QString &pseudo,const quint8 &skinInt) const;
    void battleAcceptedByOther(const QString &pseudo,const quint8 &skinId,const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) const;
    void battleCanceledByOther() const;
    void sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn) const;
    void sendFullBattleReturn(const QList<Skill::AttackReturn> &attackReturn,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) const;

    //clan
    void clanActionSuccess(const quint32 &clanId) const;
    void clanActionFailed() const;
    void clanDissolved() const;
    void clanInformations(const QString &name) const;
    void clanInvite(const quint32 &clanId,const QString &name) const;
    void cityCapture(const quint32 &remainingTime,const quint8 &type) const;

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const QString &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const quint16 &player_count,const quint16 &clan_count);
    void captureCityStartBotFight(const quint16 &player_count,const quint16 &clan_count,const quint32 &fightId);
    void captureCityDelayedStart(const quint16 &player_count,const quint16 &clan_count);
    void captureCityWin();
public slots:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void send_player_move(const quint8 &moved_unit,const CatchChallenger::Direction &direction);
    void sendChatText(const CatchChallenger::Chat_type &chatType,const QString &text);
    void sendPM(const QString &text,const QString &pseudo);
    void teleportDone();

    //plant, can do action only if the previous is finish
    void useSeed(const quint8 &plant_id);
    void collectMaturePlant();
    //crafting
    void useRecipe(const quint32 &recipeId);
    void addRecipe(const quint32 &recipeId);

    //trade
    void tradeRefused();
    void tradeAccepted();
    void tradeCanceled();
    void tradeFinish();
    void addTradeCash(const quint64 &cash);
    void addObject(const quint32 &item,const quint32 &quantity);
    void addMonster(const quint32 &monsterId);

    //battle
    void battleRefused();
    void battleAccepted();

    //inventory
    void destroyObject(const quint32 &object,const quint32 &quantity=1);
    void useObject(const quint32 &object);
    void wareHouseStore(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters);

    //shop
    void getShopList(const quint32 &shopId);
    void buyObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);

    //factory
    void getFactoryList(const quint32 &factoryId);
    void buyFactoryObject(const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellFactoryObject(const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);

    //fight
    void tryEscape();
    void useSkill(const quint32 &skill);
    void heal();
    void requestFight(const quint32 &fightId);

    //lean
    void learnSkill(const quint32 &monsterId,const quint32 &skill);

    //quest
    void startQuest(const quint32 &questId);
    void finishQuest(const quint32 &questId);
    void cancelQuest(const quint32 &questId);
    void nextQuestStep(const quint32 &questId);

    //clan
    void createClan(const QString &name);
    void leaveClan();
    void dissolveClan();
    void inviteClan(const QString &pseudo);
    void ejectClan(const QString &pseudo);
    void inviteAccept(const bool &accept);
    void waitingForCityCapture(const bool &cancel);
};
}

#endif // CATCHCHALLENGER_PROTOCOL_H
