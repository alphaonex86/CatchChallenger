#ifndef API_PROTOCOL_QT_H
#define API_PROTOCOL_QT_H

#include <QObject>
#include "ClientStructures.h"
#include "../../general/base/GeneralStructures.h"
#include "../libcatchchallenger/Api_protocol.h"
#include "ConnectedSocket.h"
namespace CatchChallenger {
class Api_protocol_Qt : public QObject, public Api_protocol
{
    Q_OBJECT
public:
    explicit Api_protocol_Qt(ConnectedSocket *socket);
    bool disconnectClient() override;

    void QtsocketDestroyed();
    void socketDestroyed() override;
    void resetAll();
    void disconnectFromHost();
    void connectTheExternalSocketInternal();
    bool tryLogin(const std::string &login, const std::string &pass);
    std::string socketDisconnectedForReconnect() override;
    void parseIncommingData() override;
    void sslHandcheckIsFinished();
    void closeSocket() override;
#ifndef NOTCPSOCKET
protected:
    void saveCert(const std::string &file);
#endif
public:
    void send_player_direction(const CatchChallenger::Direction &the_direction) override;
    void newError(const std::string &error,const std::string &detailedError) override;
    void message(const std::string &message) override;
    void lastReplyTime(const uint32_t &time) override;

    void useSeed(const uint8_t &plant_id);
    void collectMaturePlant();
    void destroyObject(const uint16_t &object,const uint32_t &quantity=1);

    //protocol/connection info
    virtual void hashSha224(const char * const data,const int size,char *buffer) override;
    virtual void readForFirstHeader() override;
    void disconnected(const std::string &reason) override;
    void notLogged(const std::string &reason) override;
    void logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList) override;
    virtual void tryDisconnect() override;
    void protocol_is_good() override;
    void connectedOnLoginServer() override;
    void connectingOnGameServer() override;
    void connectedOnGameServer() override;
    void haveDatapackMainSubCode() override;
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression) override;

    //general info
    void number_of_player(const uint16_t &number,const uint16_t &max_players) override;
    void random_seeds(const std::string &data) override;

    //character
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) override;
    void haveCharacter() override;
    //events
    void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) override;
    void newEvent(const uint8_t &event,const uint8_t &event_value) override;

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,
                       const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) override;
    void move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement) override;
    void remove_player(const uint16_t &id) override;
    void reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) override;
    void full_reinsert_player(const uint16_t &id,const uint32_t &mapId,
                              const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction) override;
    void dropAllPlayerOnTheMap() override;
    void teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) override;

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature) override;
    void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y) override;
    void seed_planted(const bool &ok) override;
    void plant_collected(const CatchChallenger::Plant_collect &stat) override;
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage) override;
    //inventory
    void objectUsed(const ObjectUsage &objectUsage) override;
    void monsterCatch(const bool &success) override;

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,
                       const std::string &pseudo,const CatchChallenger::Player_type &player_type) override;
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text) override;

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) override;
    void have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items) override;
    void add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) override;
    void remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items) override;

    //datapack
    void haveTheDatapack() override;
    void haveTheDatapackMainSub() override;
    //base
    void newFileBase(const std::string &fileName,const std::string &data) override;
    void newHttpFileBase(const std::string &url,const std::string &fileName) override;
    void removeFileBase(const std::string &fileName) override;
    void datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) override;
    //main
    void newFileMain(const std::string &fileName,const std::string &data) override;
    void newHttpFileMain(const std::string &url,const std::string &fileName) override;
    void removeFileMain(const std::string &fileName) override;
    void datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) override;
    //sub
    void newFileSub(const std::string &fileName,const std::string &data) override;
    void newHttpFileSub(const std::string &url,const std::string &fileName) override;
    void removeFileSub(const std::string &fileName) override;
    void datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize) override;

    //shop
    void haveShopList(const std::vector<ItemToSellOrBuy> &items) override;
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) override;

    //factory
    void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,
                         const std::vector<ItemToSellOrBuy> &products) override;
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) override;

    //trade
    void tradeRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeCanceledByOther() override;
    void tradeFinishedByOther() override;
    void tradeValidatedByTheServer() override;
    void tradeAddTradeCash(const uint64_t &cash) override;
    void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity) override;
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster) override;

    //battle
    void battleRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,
                               const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) override;
    void battleCanceledByOther() override;
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn) override;

    //clan
    void clanActionSuccess(const uint32_t &clanId) override;
    void clanActionFailed() override;
    void clanDissolved() override;
    void clanInformations(const std::string &name) override;
    void clanInvite(const uint32_t &clanId,const std::string &name) override;
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type) override;

    //city
    void captureCityYourAreNotLeader() override;
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone) override;
    void captureCityPreviousNotFinished() override;
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count) override;
    void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId) override;
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count) override;
    void captureCityWin() override;

    //market
    void marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,
                    const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,
                    const std::vector<MarketMonster> &marketOwnMonsterList) override;
    void marketBuy(const bool &success) override;
    void marketBuyMonster(const PlayerMonster &playerMonster) override;
    void marketPut(const bool &success) override;
    void marketGetCash(const uint64_t &cash) override;
    void marketWithdrawCanceled() override;
    void marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity) override;
    void marketWithdrawMonster(const PlayerMonster &playerMonster) override;

signals:
    void QtnewError(const std::string &error,const std::string &detailedError);
    void Qtmessage(const std::string &message);
    void QtlastReplyTime(const uint32_t &time);

    //protocol/connection info
    void Qtdisconnected(const std::string &reason);
    void QtnotLogged(const std::string &reason);
    void Qtlogged(const std::vector<std::vector<CharacterEntry> > &characterEntryList);
    void Qtprotocol_is_good();
    void QtconnectedOnLoginServer();
    void QtconnectingOnGameServer();
    void QtconnectedOnGameServer();
    void QthaveDatapackMainSubCode();
    void QtgatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);

    //general info
    void Qtnumber_of_player(const uint16_t &number,const uint16_t &max_players);
    void Qtrandom_seeds(const std::string &data);

    //character
    void QtnewCharacterId(const uint8_t &returnCode,const uint32_t &characterId);
    void QthaveCharacter();
    //events
    void QtsetEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events);
    void QtnewEvent(const uint8_t &event,const uint8_t &event_value);

    //map move
    void Qtinsert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void Qtmove_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement);
    void Qtremove_player(const uint16_t &id);
    void Qtreinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void Qtfull_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction);
    void QtdropAllPlayerOnTheMap();
    void QtteleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);

    //plant
    void Qtinsert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void Qtremove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y);
    void Qtseed_planted(const bool &ok);
    void Qtplant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void QtrecipeUsed(const RecipeUsage &recipeUsage);
    //inventory
    void QtobjectUsed(const ObjectUsage &objectUsage);
    void QtmonsterCatch(const bool &success);

    //chat
    void Qtnew_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type);
    void Qtnew_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text);

    //player info
    void Qthave_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void Qthave_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items);
    void Qtadd_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items);
    void Qtremove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items);

    //datapack
    void QthaveTheDatapack();
    void QthaveTheDatapackMainSub();
    //base
    void QtnewFileBase(const std::string &fileName,const std::string &data);
    void QtnewHttpFileBase(const std::string &url,const std::string &fileName);
    void QtremoveFileBase(const std::string &fileName);
    void QtdatapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    //main
    void QtnewFileMain(const std::string &fileName,const std::string &data);
    void QtnewHttpFileMain(const std::string &url,const std::string &fileName);
    void QtremoveFileMain(const std::string &fileName);
    void QtdatapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    //sub
    void QtnewFileSub(const std::string &fileName,const std::string &data);
    void QtnewHttpFileSub(const std::string &url,const std::string &fileName);
    void QtremoveFileSub(const std::string &fileName);
    void QtdatapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);

    //shop
    void QthaveShopList(const std::vector<ItemToSellOrBuy> &items);
    void QthaveBuyObject(const BuyStat &stat,const uint32_t &newPrice);
    void QthaveSellObject(const SoldStat &stat,const uint32_t &newPrice);

    //factory
    void QthaveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products);
    void QthaveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice);
    void QthaveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice);

    //trade
    void QttradeRequested(const std::string &pseudo,const uint8_t &skinInt);
    void QttradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt);
    void QttradeCanceledByOther();
    void QttradeFinishedByOther();
    void QttradeValidatedByTheServer();
    void QttradeAddTradeCash(const uint64_t &cash);
    void QttradeAddTradeObject(const uint32_t &item,const uint32_t &quantity);
    void QttradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);

    //battle
    void QtbattleRequested(const std::string &pseudo,const uint8_t &skinInt);
    void QtbattleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void QtbattleCanceledByOther();
    void QtsendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn);

    //clan
    void QtclanActionSuccess(const uint32_t &clanId);
    void QtclanActionFailed();
    void QtclanDissolved();
    void QtclanInformations(const std::string &name);
    void QtclanInvite(const uint32_t &clanId,const std::string &name);
    void QtcityCapture(const uint32_t &remainingTime,const uint8_t &type);

    //city
    void QtcaptureCityYourAreNotLeader();
    void QtcaptureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
    void QtcaptureCityPreviousNotFinished();
    void QtcaptureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void QtcaptureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId);
    void QtcaptureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void QtcaptureCityWin();

    //market
    void QtmarketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList);
    void QtmarketBuy(const bool &success);
    void QtmarketBuyMonster(const PlayerMonster &playerMonster);
    void QtmarketPut(const bool &success);
    void QtmarketGetCash(const uint64_t &cash);
    void QtmarketWithdrawCanceled();
    void QtmarketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity);
    void QtmarketWithdrawMonster(const PlayerMonster &playerMonster);

public:
    std::string getLanguage() const;
protected:
    ConnectedSocket *socket;
};
}

#endif // API_PROTOCOL_QT_H
