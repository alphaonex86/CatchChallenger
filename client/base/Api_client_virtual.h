#ifndef CATCHCHALLENGER_API_CLIENT_VIRTUAL_H
#define CATCHCHALLENGER_API_CLIENT_VIRTUAL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ConnectedSocket.h"
#include "Api_protocol.h"

namespace CatchChallenger {
class Api_client_virtual : public Api_protocol
{
    Q_OBJECT
public:
    explicit Api_client_virtual(ConnectedSocket *socket);
    ~Api_client_virtual();
    void sendDatapackContentBase(const std::string &hashBase=std::string());
    void sendDatapackContentMainSub(const std::string &hashMain=std::string(),const std::string &hashSub=std::string());
    void tryDisconnect();
protected:
    //general data
    void defineMaxPlayers(const uint16_t &);
signals:
    void newError(const std::string &error,const std::string &detailedError) const;
    void message(const std::string &message) const;
    void lastReplyTime(const uint32_t &time) const;

    //protocol/connection info
    void disconnected(const std::string &reason) const;
    void notLogged(const std::string &reason) const;
    void logged(const std::vector<ServerFromPoolForDisplay *> &serverOrdenedList,const std::vector<std::vector<CharacterEntry> > &characterEntryList) const;
    void protocol_is_good() const;
    void connectedOnLoginServer() const;
    void connectingOnGameServer() const;
    void connectedOnGameServer() const;
    void haveDatapackMainSubCode();
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);

    //general info
    void number_of_player(const uint16_t &number,const uint16_t &max_players) const;
    void random_seeds(const std::string &data) const;

    //character
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) const;
    void haveCharacter() const;
    //events
    void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) const;
    void newEvent(const uint8_t &event,const uint8_t &event_value) const;

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;
    void move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement) const;
    void remove_player(const uint16_t &id) const;
    void reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;
    void full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction) const;
    void dropAllPlayerOnTheMap() const;
    void teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) const;

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature) const;
    void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y) const;
    void seed_planted(const bool &ok) const;
    void plant_collected(const CatchChallenger::Plant_collect &stat) const;
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage) const;
    //inventory
    void objectUsed(const ObjectUsage &objectUsage) const;
    void monsterCatch(const bool &success) const;

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type) const;
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text) const;

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) const;
    void have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items) const;
    void add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) const;
    void remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) const;

    //datapack
    void haveTheDatapack() const;
    void haveTheDatapackMainSub() const;
    //base
    void newFileBase(const std::string &fileName,const std::string &data) const;
    void newHttpFileBase(const std::string &url,const std::string &fileName) const;
    void removeFileBase(const std::string &fileName) const;
    void datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;
    //main
    void newFileMain(const std::string &fileName,const std::string &data) const;
    void newHttpFileMain(const std::string &url,const std::string &fileName) const;
    void removeFileMain(const std::string &fileName) const;
    void datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;
    //sub
    void newFileSub(const std::string &fileName,const std::string &data) const;
    void newHttpFileSub(const std::string &url,const std::string &fileName) const;
    void removeFileSub(const std::string &fileName) const;
    void datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) const;

    //shop
    void haveShopList(const std::vector<ItemToSellOrBuy> &items) const;
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) const;
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) const;

    //factory
    void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products) const;
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) const;
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) const;

    //trade
    void tradeRequested(const std::string &pseudo,const uint8_t &skinInt) const;
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) const;
    void tradeCanceledByOther() const;
    void tradeFinishedByOther() const;
    void tradeValidatedByTheServer() const;
    void tradeAddTradeCash(const uint64_t &cash) const;
    void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity) const;
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster) const;

    //battle
    void battleRequested(const std::string &pseudo,const uint8_t &skinInt) const;
    void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) const;
    void battleCanceledByOther() const;
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn) const;

    //clan
    void clanActionSuccess(const uint32_t &clanId) const;
    void clanActionFailed() const;
    void clanDissolved() const;
    void clanInformations(const std::string &name) const;
    void clanInvite(const uint32_t &clanId,const std::string &name) const;
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type) const;

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId);
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityWin();

    //market
    void marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList) const;
    void marketBuy(const bool &success) const;
    void marketBuyMonster(const PlayerMonster &playerMonster) const;
    void marketPut(const bool &success) const;
    void marketGetCash(const uint64_t &cash) const;
    void marketWithdrawCanceled() const;
    void marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity) const;
    void marketWithdrawMonster(const PlayerMonster &playerMonster) const;
};
}

#endif // Protocol_and_virtual_data_H
