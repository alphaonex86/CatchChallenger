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
    void newError(const std::string &error,const std::string &detailedError);
    void message(const std::string &message);
    void lastReplyTime(const uint32_t &time);

    //protocol/connection info
    void disconnected(const std::string &reason);
    void notLogged(const std::string &reason);
    void logged(const std::vector<ServerFromPoolForDisplay *> &serverOrdenedList,const std::vector<std::vector<CharacterEntry> > &characterEntryList);
    void protocol_is_good();
    void connectedOnLoginServer();
    void connectingOnGameServer();
    void connectedOnGameServer();
    void haveDatapackMainSubCode();
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);

    //general info
    void number_of_player(const uint16_t &number,const uint16_t &max_players);
    void random_seeds(const std::string &data);

    //character
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId);
    void haveCharacter();
    //events
    void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events);
    void newEvent(const uint8_t &event,const uint8_t &event_value);

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement);
    void remove_player(const uint16_t &id);
    void reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction);
    void dropAllPlayerOnTheMap();
    void teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);

    //plant
    void insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature);
    void remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);
    //inventory
    void objectUsed(const ObjectUsage &objectUsage);
    void monsterCatch(const bool &success);

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type);
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text);

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items);
    void add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items);
    void remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items);

    //datapack
    void haveTheDatapack();
    void haveTheDatapackMainSub();
    //base
    void newFileBase(const std::string &fileName,const std::string &data);
    void newHttpFileBase(const std::string &url,const std::string &fileName);
    void removeFileBase(const std::string &fileName);
    void datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    //main
    void newFileMain(const std::string &fileName,const std::string &data);
    void newHttpFileMain(const std::string &url,const std::string &fileName);
    void removeFileMain(const std::string &fileName);
    void datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    //sub
    void newFileSub(const std::string &fileName,const std::string &data);
    void newHttpFileSub(const std::string &url,const std::string &fileName);
    void removeFileSub(const std::string &fileName);
    void datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);

    //shop
    void haveShopList(const std::vector<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice);

    //factory
    void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products);
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice);
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice);

    //trade
    void tradeRequested(const std::string &pseudo,const uint8_t &skinInt);
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt);
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);

    //battle
    void battleRequested(const std::string &pseudo,const uint8_t &skinInt);
    void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster);
    void battleCanceledByOther();
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn);

    //clan
    void clanActionSuccess(const uint32_t &clanId);
    void clanActionFailed();
    void clanDissolved();
    void clanInformations(const std::string &name);
    void clanInvite(const uint32_t &clanId,const std::string &name);
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type);

    //city
    void captureCityYourAreNotLeader();
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
    void captureCityPreviousNotFinished();
    void captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId);
    void captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count);
    void captureCityWin();

    //market
    void marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList);
    void marketBuy(const bool &success);
    void marketBuyMonster(const PlayerMonster &playerMonster);
    void marketPut(const bool &success);
    void marketGetCash(const uint64_t &cash);
    void marketWithdrawCanceled();
    void marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity);
    void marketWithdrawMonster(const PlayerMonster &playerMonster);
};
}

#endif // Protocol_and_virtual_data_H
