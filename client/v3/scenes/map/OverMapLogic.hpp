// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_OVERMAPLOGIC_HPP_
#define CLIENT_V3_SCENES_MAP_OVERMAPLOGIC_HPP_

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../../libqtcatchchallenger/DisplayStructures.hpp"
#include "../../entities/Map_client.hpp"
#include "../../entities/render/MapVisualiserPlayer.hpp"
#include "../../ui/LinkedDialog.hpp"
#include "../shared/inventory/Inventory.hpp"
#include "../shared/shop/Shop.hpp"
#include "../shared/warehouse/Warehouse.hpp"
#include "OverMap.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include <QAudioOutput>

#include "../../../libqtcatchchallenger/QInfiniteBuffer.hpp"
#endif
#include <vector>

namespace Scenes {
class OverMapLogic : public OverMap {
  Q_OBJECT

 public:
  static OverMapLogic *Create();
  void resetAll();
  void setVar(CCMap *ccmap) override;
  void OnEnter() override;
  void OnExit() override;

  enum QueryType { QueryType_Seed, QueryType_CollectPlant };
  // plant seed in waiting
  struct SeedInWaiting {
    uint16_t seedItemId;
    uint8_t x, y;
    std::string map;
  };
  struct ClientPlantInCollecting {
    std::string map;
    uint8_t x, y;
    uint8_t plant_id;
    uint16_t seconds_to_mature;
  };
  struct BattleInformations {
    std::string pseudo;
    uint8_t skinId;
    std::vector<uint8_t> stat;
    uint8_t monsterPlace;
    CatchChallenger::PublicPlayerMonster publicPlayerMonster;
  };
  enum ObjectType {
    ObjectType_All,
    ObjectType_Seed,
    ObjectType_Sell,
    ObjectType_SellToMarket,
    ObjectType_Trade,
    ObjectType_MonsterToTrade,
    ObjectType_MonsterToTradeToMarket,
    ObjectType_MonsterToLearn,
    ObjectType_MonsterToFight,
    ObjectType_MonsterToFightKO,
    ObjectType_ItemOnMonsterOutOfFight,
    ObjectType_ItemOnMonster,
    ObjectType_ItemEvolutionOnMonster,
    ObjectType_ItemLearnOnMonster,
    ObjectType_UseInFight
  };
  enum ActionClan {
    ActionClan_Create,
    ActionClan_Leave,
    ActionClan_Dissolve,
    ActionClan_Invite,
    ActionClan_Eject
  };
 public slots:
  void connectAllSignals();
  void selectObject(const ObjectType &objectType);
  void objectSelection(const bool &ok, const uint16_t &itemId = 0,
                       const uint32_t &quantity = 1);
  void bag_open();
  void inventoryNext();
  void inventoryBack();
  void player_open();
  void playerNext();
  void playerBack();

  void pathFindingNotFound();
  void repelEffectIsOver();
  void teleportConditionNotRespected(const std::string &text);
  void stopped_in_front_of(CatchChallenger::Map_client *map, uint8_t x,
                           uint8_t y);
  void actionOn(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
  void actionOnNothing();
  void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
  bool actionOnCheckBot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y);
  int32_t havePlant(CatchChallenger::Map_client *map, uint8_t x,
                    uint8_t y) const;
  void errorWithTheCurrentMap();
  void currentMapLoaded();
  void addCash(const uint32_t &cash);
  void removeCash(const uint32_t &cash);

  void tipTimeout();
  void gainTimeout();
  void showTip(const std::string &tip);
  void hideTip();
  void showPlace(const std::string &place);
  void showGain();
  void hideGain();
  void composeAndDisplayGain();
  void addQuery(const QueryType &queryType);
  void removeQuery(const QueryType &queryType);
  void updateQueryList();
  bool stopped_in_front_of_check_bot(CatchChallenger::Map_client *map,
                                     uint8_t x, uint8_t y);

  // clan
  void ClanActionSuccessSlot(const uint32_t &clan_id);
  void ClanActionFailedSlot();
  void ClanDissolvedSlot();
  void ClanInformationSlot(const std::string &name);
  void ClanInviteSlot(const uint32_t &clan_id, const std::string &name);

  // city
  void cityCapture(const uint32_t &remainingTime, const uint8_t &type);
  void cityCaptureUpdateTime();
  void updatePageZoneCatch();
  void on_zonecaptureCancel_clicked();
  void captureCityYourAreNotLeader();
  void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone);
  void captureCityPreviousNotFinished();
  void captureCityStartBattle(const uint16_t &player_count,
                              const uint16_t &clan_count);
  void captureCityStartBotFight(const uint16_t &player_count,
                                const uint16_t &clan_count,
                                const uint16_t &fightId);
  void captureCityDelayedStart(const uint16_t &player_count,
                               const uint16_t &clan_count);
  void captureCityWin();
  // inventory
  // void have_inventory(const std::unordered_map<uint16_t, uint32_t> &items,
  // const std::unordered_map<uint16_t, uint32_t> &warehouse_items);
  void add_to_inventory(const uint16_t &item, const uint32_t &quantity = 1,
                        const bool &showGain = true);
  void add_to_inventory_slotpair(const uint16_t &item,
                                 const uint32_t &quantity = 1,
                                 const bool &showGain = true);
  void add_to_inventory(
      const std::vector<std::pair<uint16_t, uint32_t> > &items,
      const bool &showGain = true);
  void add_to_inventory(const std::unordered_map<uint16_t, uint32_t> &items,
                        const bool &showGain = true);
  void add_to_inventory_slot(
      const std::unordered_map<uint16_t, uint32_t> &items);
  void remove_to_inventory(const std::unordered_map<uint16_t, uint32_t> &items);
  void remove_to_inventory_slot(
      const std::unordered_map<uint16_t, uint32_t> &items);
  void remove_to_inventory(const uint16_t &itemId,
                           const uint32_t &quantity = 1);
  void remove_to_inventory_slotpair(const uint16_t &itemId,
                                    const uint32_t &quantity = 1);
  // void load_inventory();
  // void on_inventory_itemActivated(QListWidgetItem *item);
  void objectUsed(const CatchChallenger::ObjectUsage &objectUsage);
  // factory
  void updateFactoryStatProduction(
      const CatchChallenger::IndustryStatus &industryStatus,
      const CatchChallenger::Industry &industry);
  void haveBuyFactoryObject(const CatchChallenger::BuyStat &stat,
                            const uint32_t &newPrice);
  void haveSellFactoryObject(const CatchChallenger::SoldStat &stat,
                             const uint32_t &newPrice);
  void haveFactoryList(
      const uint32_t &remainingProductionTime,
      const std::vector<CatchChallenger::ItemToSellOrBuy> &resources,
      const std::vector<CatchChallenger::ItemToSellOrBuy> &products);
  // plant
  void insert_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y,
                    const uint8_t &plant_id, const uint16_t &seconds_to_mature);
  void remove_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y);
  void cancelAllPlantQuery(const std::string map, const uint8_t x,
                           const uint8_t y);  // without ref because after reset
                                              // them self will failed all reset
  void seed_planted(const bool &ok);
  void plant_collected(const CatchChallenger::Plant_collect &stat);
  // crafting -> go to inventory
  // void recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage);
  // bot
  void goToBotStep(const uint8_t &step);
  std::string parseHtmlToDisplay(const std::string &htmlContent);
  void IG_dialog_text_linkActivated(const std::string &rawlink);
  void on_IG_dialog_text_linkActivated(const QString &rawlink);
  // quest
  void appendReputationRewards(
      const std::vector<CatchChallenger::ReputationRewards> &reputationList);
  void showQuestText(const uint32_t &textId);
  bool tryValidateQuestStep(const uint16_t &questId, const uint16_t &botId,
                            bool silent = false);
  bool nextStepQuest(const CatchChallenger::Quest &quest);
  bool startQuest(const CatchChallenger::Quest &quest);
  bool botHaveQuest(const uint16_t &botId) const;
  std::vector<std::pair<uint16_t, std::string> > getQuestList(
      const uint16_t &botId) const;
  void updateDisplayedQuests();
  void appendReputationPoint(const std::string &type, const int32_t &point);
  bool haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const;
  bool haveStartQuestRequirement(const CatchChallenger::Quest &quest) const;
  // trade
  /*void tradeRequested(const std::string &pseudo, const uint8_t &skinInt);
  void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt);
  void tradeCanceledByOther();
  void tradeFinishedByOther();
  void tradeValidatedByTheServer();
  void tradeAddTradeCash(const uint64_t &cash);
  void tradeAddTradeObject(const uint16_t &item, const uint32_t &quantity);
  void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);
  void tradeUpdateCurrentObject();
  std::vector<CatchChallenger::PlayerMonster> warehouseMonsterOnPlayer()
  const;*/
  // battle
  /*void battleRequested(const std::string &pseudo, const uint8_t &skinInt);
  void battleAcceptedByOther(const std::string &pseudo, const uint8_t &skinId,
  const std::vector<uint8_t> &stat, const uint8_t &monsterPlace, const
  CatchChallenger::PublicPlayerMonster &publicPlayerMonster); void
  battleAcceptedByOtherFull(const BattleInformations &battleInformations); void
  battleCanceledByOther(); void sendBattleReturn(const
  std::vector<CatchChallenger::Skill::AttackReturn> &attackReturn);*/
  // market
  /*void marketList(const uint64_t &price,const
  std::vector<CatchChallenger::MarketObject> &marketObjectList,const
  std::vector<CatchChallenger::MarketMonster> &marketMonsterList,const
  std::vector<CatchChallenger::MarketObject> &marketOwnObjectList,const
  std::vector<CatchChallenger::MarketMonster> &marketOwnMonsterList); void
  addOwnMonster(const CatchChallenger::MarketMonster &marketMonster); void
  marketBuy(const bool &success); void marketBuyMonster(const
  CatchChallenger::PlayerMonster &playerMonster); void marketPut(const bool
  &success); void marketGetCash(const uint64_t &cash); void
  marketWithdrawCanceled(); void marketWithdrawObject(const uint16_t &objectId,
  const uint32_t &quantity); void marketWithdrawMonster(const
  CatchChallenger::PlayerMonster &playerMonster);
  */
  // fight
  void botFight(const uint16_t &fightId);
  void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x,
                          const uint8_t &y);
  void botFightCollision(CatchChallenger::Map_client *map, const uint8_t &x,
                         const uint8_t &y, const uint16_t &fightId);
  void BattleFinished();

 signals:
  void error(const std::string &error);
  // plant, can do action only if the previous is finish
  void useSeed(const uint8_t &plant_id);
  void collectMaturePlant();

 private:
  CCMap *ccmap;
  bool multiplayer;
  UI::LinkedDialog *inventory_tabs_;
  // Crafting *crafting;
  // int inventoryIndex;

  UI::LinkedDialog *player_tabs_;
  // Reputations *reputations;
  // Quests *quests;
  int playerIndex;

  QTimer tip_timeout;
  QTimer gain_timeout;
  std::vector<QueryType> queryList;

  QTimer checkQueryTime;
  uint32_t worseQueryTime;
  uint8_t lastStepUsed;

  std::vector<SeedInWaiting> seed_in_waiting;
  std::vector<ClientPlantInCollecting> plant_collect_in_waiting;

  std::vector<std::string> add_to_inventoryGainList;
  std::vector<QTime> add_to_inventoryGainTime;
  std::vector<std::string> add_to_inventoryGainExtraList;
  std::vector<QTime> add_to_inventoryGainExtraTime;

 private:
  std::string lastPlaceDisplayed;
  std::string currentAmbianceFile;
  std::string visualCategory;

  // Clan
  std::vector<ActionClan> actionClan;

  // industry
  uint16_t factoryId;
  CatchChallenger::IndustryStatus industryStatus;
  bool factoryInProduction;
  // bot
  CatchChallenger::Bot actualBot;
  // quest
  bool isInQuest;
  uint16_t questId;
  // city
  QTimer nextCityCatchTimer;
  CatchChallenger::City city;
  QTimer updater_page_zonecatch;
  QDateTime nextCatch, nextCatchOnScreen;
  std::string zonecatchName;
  bool zonecatch;
  // shop
  Shop *shop_;
  uint32_t tempQuantityForSell;
  bool waitToSell;
  std::vector<CatchChallenger::ItemToSellOrBuy> itemsToSell, itemsToBuy;
  std::vector<uint16_t> objectInUsing;
  std::vector<uint32_t> monster_to_deposit, monster_to_withdraw;

  Warehouse *warehouse_;

  OverMapLogic();
  void OnInventoryNav(std::string id);
  void CreateInventoryTabs();
  void CreatePlayerTabs();
  void OnUseItem(Inventory::ObjectType type, const uint16_t &item_id,
                 const uint32_t &quantity);
  void ShowPlantDialog();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_OVERMAPLOGIC_HPP_
