// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_QUESTS_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_QUESTS_HPP_

#include "../../../core/Node.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/Backdrop.hpp"
#include "QuestItem.hpp"

namespace Scenes {
class Quests : public Node {
 public:
  ~Quests();
  static Quests *Create();
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override;

 private:
  UI::ListView *quests_;
  UI::ListView *details_;
  UI::Button *cancel_;
  UI::Backdrop *backdrop_;
  Node *selected_item_;

  Quests();
  void OnCancel();
  void OnQuestClick(Node *item);
  void LoadQuests();
  void OnQuestItemChanged();
  UI::Label *CreateLabel(const QString &text);
  UI::Label *CreateLabel(const std::string &text);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_QUESTS_HPP_
