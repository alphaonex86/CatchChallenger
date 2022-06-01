// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_

#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/MessageDialog.hpp"

namespace Scenes {
class MonsterBag : public Node {
 public:
  enum Mode { kDefault, kOnBattle, kOnItemUse };

  ~MonsterBag();
  static MonsterBag *Create(Mode mode = kDefault);


  void OnResize() override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void Draw(QPainter *painter);

  void OnAcceptClicked();
  void SetOnSelectMonster(std::function<void(uint8_t)> callback);
  void OnSelectMonster(uint8_t monsterPosition);
  void LoadMonster();
  void ShowSelectAction(bool show);
  void SetMode(Mode mode);

 private:
  UI::ListView *monster_list_;
  UI::Button *select_;

  ConnectionManager *connection_manager_;
  CatchChallenger::Api_protocol_Qt *client_;
  bool show_select_;
  std::function<void(uint8_t)> on_select_monster_;
  Node *selected_item_;
  Mode mode_;

  MonsterBag(Mode mode);
  void OnSelectItem(Node *node);
  void ShowMessageDialog(const QString &title, const QString &message);
  void ClearSelection();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_MONSTERBAG_HPP_

