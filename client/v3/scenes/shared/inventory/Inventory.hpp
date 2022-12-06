// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORY_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORY_HPP_

#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/CommonTypes.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/GridView.hpp"
#include "../../../ui/Label.hpp"

namespace Scenes {
class Inventory : public Node {
 public:
  static Inventory *Create();
  ~Inventory();

  enum ObjectFilter {
    ObjectFilter_All,
    ObjectFilter_Seed,
    ObjectFilter_Sell,
    ObjectFilter_SellToMarket,
    ObjectFilter_Trade,
    ObjectFilter_MonsterToTrade,
    ObjectFilter_MonsterToTradeToMarket,
    ObjectFilter_MonsterToLearn,
    ObjectFilter_MonsterToFight,
    ObjectFilter_MonsterToFightKO,
    ObjectFilter_ItemOnMonsterOutOfFight,
    ObjectFilter_ItemOnMonster,
    ObjectFilter_ItemEvolutionOnMonster,
    ObjectFilter_ItemLearnOnMonster,
    ObjectFilter_UseInFight
  };

  void SetVar(const ObjectFilter &filter, const bool inSelection,
              const int lastItemSelected = -1);

  void on_inventory_itemSelectionChanged();
  void inventoryUse_slot();
  void inventoryDestroy_slot();
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override;
  void SetOnUseItem(
      std::function<void(ObjectCategory, uint16_t, uint32_t, uint8_t)> callback);
  void SetOnDeleteItem(std::function<void(uint16_t)> callback);

 private:
  UI::GridView *inventory;

  Sprite *descriptionBack;
  UI::Label *inventory_description;
  UI::Button *inventoryDestroy;
  UI::Button *inventoryUse;

  ConnectionManager *connexionManager;
  Inventory::ObjectFilter object_filter_;
  bool inSelection;
  uint8_t lastItemSize;
  uint8_t itemCount;
  int lastItemSelected;
  std::function<void(ObjectCategory, uint16_t, uint32_t, uint8_t)> on_use_item_;
  std::function<void(uint16_t)> on_delete_item_;

  Inventory();

  void OnItemClick(Node *node);
  void UpdateInventory(uint8_t targetSize, bool force);
  void OnUpdateInfo(PlayerInfo *info);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORY_HPP_
