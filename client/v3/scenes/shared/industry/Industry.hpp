// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_
#define CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../../../general/base/GeneralStructuresXml.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Combo.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/GridView.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ThemedButton.hpp"
#include "IndustryItem.hpp"

namespace Scenes {
class Industry : public UI::Dialog {
 public:
  static Industry *Create();
  ~Industry();

  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;

  void SetOnTransactionFinish(std::function<void()> callback);

 private:
  UI::Label *player_name_;
  UI::Label *storage_name_;
  UI::Combo *mode_;
  UI::Button *move_right_;
  UI::Button *move_left_;
  UI::Button *accept_;
  UI::GridView *inventory_content_;
  UI::GridView *storage_content_;
  UI::Label *player_cash_;
  UI::Label *storage_cash_;
  std::function<void()> on_transaction_finish_;
  ConnectionManager *connexionManager;
  Node *selected_item_;

  int64_t temp_cash_;
  int8_t current_mode_;
  std::unordered_map<uint16_t, int32_t> temp_items_;
  std::vector<uint8_t> temp_monster_withdraw_;
  std::vector<uint8_t> temp_monster_deposit_;

  Industry();
  void OnItemClick(Node *node);
  void OnModeChange(uint8_t index);
  void OnAcceptClick();
  void UpdateContent();
  void OnWithdraw();
  void OnWithdrawDelayed(const QString &value);
  void OnDeposit();
  void OnDepositDelayed(const QString &value);
  IndustryItem *ItemToItem(const uint16_t &item_id, const uint32_t &quantity);
  IndustryItem *ItemToMonster(const uint32_t &monster_id,
                               const uint16_t &monster,
                               const uint32_t &quantity);
  std::vector<CatchChallenger::PlayerMonster> MonstersWithdrawed() const;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INDUSTRY_INDUSTRY_HPP_
