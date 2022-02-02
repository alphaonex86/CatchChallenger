// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_FINISHEDQUESTS_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_FINISHEDQUESTS_HPP_

#include "../../../core/Node.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"

namespace Scenes {
class FinishedQuests : public Node {
 public:
  ~FinishedQuests();
  static FinishedQuests* Create();
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override; 

 private:
  UI::ListView *quests_;

  FinishedQuests();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_FINISHEDQUESTS_HPP_
