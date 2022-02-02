// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_REPUTATIONS_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_REPUTATIONS_HPP_

#include "../../../core/Node.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"

namespace Scenes {
class Reputations : public Node {
 public:
  ~Reputations();

  static Reputations *Create();
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override;

 private:
  UI::ListView *list_;

  Reputations();
  void LoadReputations();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_REPUTATIONS_HPP_
