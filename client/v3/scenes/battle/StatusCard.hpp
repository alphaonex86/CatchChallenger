// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_STATUSCARD_HPP_
#define CLIENT_V3_SCENES_BATTLE_STATUSCARD_HPP_

#include <unordered_map>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../core/Node.hpp"

namespace Scenes {
class StatusCard : public Node {
 public:
  static StatusCard *Create(Node *parent);
  ~StatusCard();
  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;

  void SetMonster(CatchChallenger::PlayerMonster *monster);
  void SetMonster(CatchChallenger::PublicPlayerMonster *monster);
  void UpdateData();
  qreal Padding() const;
  uint32_t HP() const;
  uint32_t XP() const;
  uint32_t XPMax() const;
  QString Name() const;

 private:
  QFont *font_;

  QString name_;
  uint16_t monster_id_;
  uint8_t level_;
  uint32_t hp_;
  uint32_t hp_max_;
  uint32_t exp_;
  uint32_t exp_max_;
  CatchChallenger::Gender gender_;
  qreal padding_;

  StatusCard(Node *parent);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_STATUSCARD_HPP_
