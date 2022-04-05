// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_BATTLE_MONSTERSWAP_HPP_
#define CLIENT_V3_SCENES_BATTLE_MONSTERSWAP_HPP_

#include "../../base/ConnectionManager.hpp"
#include "../../core/Scene.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Column.hpp"
#include "../../ui/MessageDialog.hpp"
#include "../../ui/Backdrop.hpp"
#include "../../ui/PlainLabel.hpp"

namespace Scenes {
class MonsterSwap : public Scene {
 public:
  ~MonsterSwap();
  static MonsterSwap *Create();

  void Draw(QPainter *painter);

  void OnAcceptClicked();
  void SetOnSelectMonster(std::function<void(uint8_t)> callback);
  void OnSelectMonster(uint8_t monsterPosition);
  void LoadMonster();

 protected:
  void OnScreenSD() override;
  void OnScreenHD() override;
  void OnScreenHDR() override;
  void OnScreenResize() override;
  void OnEnter() override;
  void OnExit() override;

 private:
  UI::Column *monster_list_;
  UI::Backdrop *backdrop_;

  ConnectionManager *connection_manager_;
  CatchChallenger::Api_protocol_Qt *client_;
  std::function<void(uint8_t)> on_select_monster_;
  Node *selected_item_;
  UI::PlainLabel *title_; 

  MonsterSwap();
  void OnSelectItem(Node *node);
  void ShowMessageDialog(const QString &title, const QString &message);
  void ClearSelection();
  void BackgroundCanvas(QPainter *painter);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_MONSTERSWAP_HPP_
