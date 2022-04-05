// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../ui/RoundedButton.hpp"
#include "../../../ui/SlimProgressbar.hpp"
#include "../../../core/Sprite.hpp"

namespace Scenes {
class MonsterItem : public UI::RoundedButton {
 public:
  static MonsterItem *Create(CatchChallenger::PlayerMonster monster);
  ~MonsterItem();

  void DrawContent(QPainter *painter, QColor color, QRectF boundary) override;
  void OnResize() override;

 private:
  QFont *font_;
  CatchChallenger::PlayerMonster monster_;
  ConnectionManager *connection_manager_;
  CatchChallenger::Api_protocol_Qt *client_;
  QString name_;
  Sprite *icon_;
  uint8_t level_;
  uint32_t hp_;
  uint32_t hp_max_;
  UI::SlimProgressbar *hp_bar_;

  MonsterItem(CatchChallenger::PlayerMonster monster);
  void LoadMonster();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_MONSTER_MONSTERITEM_HPP_
