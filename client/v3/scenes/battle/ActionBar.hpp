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

  void SetMonster(CatchChallenger::PlayerMonster *monster);

 private:
  UI::TextRoundedButton *fight_;
  UI::TextRoundedButton *monster_;
  UI::TextRoundedButton *bag_;
  UI::TextRoundedButton *run_;
  std::vector<SkillButton *> skills_;

  ActionBar(Node *parent);

};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_ACTIONBAR_HPP_
