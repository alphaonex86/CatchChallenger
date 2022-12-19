// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../ui/SelectableItem.hpp"
#include "../../../core/Sprite.hpp"

namespace Scenes {
class MonsterItem : public UI::SelectableItem {
 public:
  ~MonsterItem();
  static MonsterItem *Create(CatchChallenger::PlayerMonster monster);

  void DrawContent(QPainter *painter) override;
  void OnResize() override;

 private:
  CatchChallenger::PlayerMonster monster_;
  ConnectionManager *connection_manager_;
  CatchChallenger::Api_protocol_Qt *client_;
  QString name_;
  Sprite *icon_;
  uint32_t max_hp_;

  MonsterItem(CatchChallenger::PlayerMonster monster);
  void LoadMonster();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_

