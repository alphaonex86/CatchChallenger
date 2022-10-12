// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_PLAYER_CLAN_HPP_
#define CLIENT_V3_SCENES_SHARED_PLAYER_CLAN_HPP_

#include "../../../core/Node.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/Row.hpp"

namespace Scenes {
class Clan : public Node {
 public:
  ~Clan();
  static Clan* Create();

  void newLanguage();

  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void Draw(QPainter *painter) override; 

 private:

  UI::Label *clan_label_;
  UI::Label *clan_value_;
  UI::Label *status_label_;
  UI::Label *admin_label_;
  UI::Button *leave_clan_;
  UI::Button *invite_clan_;
  UI::Button *disolve_clan_;
  UI::Button *expulse_clan_;
  UI::Row *action_buttons_;

  Clan();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_PLAYER_CLAN_HPP_
