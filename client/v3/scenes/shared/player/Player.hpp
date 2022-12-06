// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_PLAYER_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_PLAYER_HPP_

#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Backdrop.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Label.hpp"

namespace Scenes {
class Player : public Node {
 public:
  ~Player();
  static Player *Create();

  void newLanguage();

  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override;

 private:
  UI::Label *name_label;
  UI::Label *name_value;
  UI::Label *cash_label;
  UI::Label *cash_value;
  UI::Label *type_label;
  UI::Label *type_value;
  Sprite *preview_;
  UI::Backdrop *backdrop_;

  Player();
  ConnectionManager *connexionManager;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_PLAYER_HPP_
