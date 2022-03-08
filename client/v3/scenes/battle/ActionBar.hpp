// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_ACTIONBAR_HPP_
#define CLIENT_V3_SCENES_BATTLE_ACTIONBAR_HPP_

#include <unordered_map>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../core/Node.hpp"
#include "../../ui/ListView.hpp"
#include "../../ui/ThemedRoundedButton.hpp"
#include "SkillButton.hpp"

namespace Scenes {
class ActionBar : public Node {
 public:
  static ActionBar *Create(Node *parent);
  ~ActionBar();

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;

  enum ActionType {
    kActionType_Attack,
    kActionType_Monster,
    kActionType_Bag,
    kActionType_Run
  };

  void SetMonster(CatchChallenger::PlayerMonster *monster);
  void CanRun(bool can_run);
  void SetOnActionClick(
      const std::function<void(const ActionType &)> &callback);
  void SetOnSkillClick(
      const std::function<void(uint8_t, uint16_t, uint8_t)> &callback);
  void ShowSkills(bool show);
  void ShowRescueSkill(bool show);

 private:
  UI::TextRoundedButton *fight_;
  UI::TextRoundedButton *monster_;
  UI::TextRoundedButton *bag_;
  UI::TextRoundedButton *run_;
  std::vector<SkillButton *> skills_;
  SkillButton *rescue_skill_;

  bool show_skills_;
  std::function<void(const ActionType &)> on_action_click_;
  std::function<void(uint8_t, uint16_t, uint8_t)> on_skill_click_;

  ActionBar(Node *parent);

  void OnFightClick();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_ACTIONBAR_HPP_
