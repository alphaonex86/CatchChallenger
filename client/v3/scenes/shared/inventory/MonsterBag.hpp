// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_

#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Scene.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Column.hpp"
#include "../../../ui/MessageDialog.hpp"
#include "../../../ui/Backdrop.hpp"

namespace Scenes {
class MonsterBag : public Scene {
 public:
  enum Mode { kDefault, kOnBattle, kOnItemUse };

  ~MonsterBag();
  static MonsterBag *Create(Mode mode = kDefault);

  void Draw(QPainter *painter);

  void OnAcceptClicked();
  void SetOnSelectMonster(std::function<void(uint8_t)> callback);
  void OnSelectMonster(uint8_t monsterPosition);
  void LoadMonster();
  void SetMode(Mode mode);

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
  Mode mode_;

  MonsterBag(Mode mode);
  void OnSelectItem(Node *node);
  void ShowMessageDialog(const QString &title, const QString &message);
  void ClearSelection();
  void BackgroundCanvas(QPainter *painter);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_