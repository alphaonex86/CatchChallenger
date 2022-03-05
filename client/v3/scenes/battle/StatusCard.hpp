
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
  void UpdateData();
  qreal Padding() const;

 private:
  QFont *font_;
  CatchChallenger::PlayerMonster *monster_;

  QString name_;
  QString level_;
  QString hp_;
  QString hp_max_;
  CatchChallenger::Gender gender_;
  qreal padding_;

  StatusCard(Node *parent);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_STATUSCARD_HPP_
