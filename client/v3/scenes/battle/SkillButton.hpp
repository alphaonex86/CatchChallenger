// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_SKILLBUTTON_HPP_
#define CLIENT_V3_SCENES_BATTLE_SKILLBUTTON_HPP_

#include "../../../general/base/GeneralStructures.hpp"
#include "../../core/Node.hpp"
#include "../../ui/RoundedButton.hpp"

namespace Scenes {
class SkillButton : public UI::RoundedButton {
 public:
  static SkillButton *Create(Node *parent);
  ~SkillButton();
  void DrawContent(QPainter *painter, QColor color, QRectF boundary) override;

  void SetSkill(const CatchChallenger::PlayerMonster::PlayerSkill &skill);
  void SetRescueSkill();

 private:
  CatchChallenger::PlayerMonster::PlayerSkill skill_;
  QFont *font_;
  QFont *font_icon_;
  QString name_;
  QString effectiveness_;
  QString count_;

  SkillButton(Node *parent);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_SKILLBUTTON_HPP_
