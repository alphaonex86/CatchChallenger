// Copyright 2021 CatchChallenger
#ifndef OverMap_H
#define OverMap_H
#include <QObject>
#include <QTimer>
#include <string>
#include <vector>

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Combo.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../shared/npctalk/NpcTalk.hpp"
#include "../shared/toast/Toast.hpp"
#include "../shared/chat/Chat.hpp"
#include "CCMap.hpp"
#include "MonsterDetails.hpp"
#include "MonsterThumb.hpp"
#include "PlayerPortrait.hpp"
#include "PlayerCounter.hpp"
#include "Tip.hpp"
#include "MapMenu.hpp"

namespace Scenes {
class OverMap : public QObject, public Scene {
  Q_OBJECT

 public:
  enum ActionClan {
    ActionClan_Create,
    ActionClan_Leave,
    ActionClan_Dissolve,
    ActionClan_Invite,
    ActionClan_Eject
  };

  OverMap();
  ~OverMap();
  virtual void resetAll();
  virtual void setVar(CCMap *ccmap);
  void newLanguage();

  void buyClicked();

  void updatePlayerNumber(const uint16_t &number, const uint16_t &max_players);
  void have_current_player_info(
      const CatchChallenger::Player_private_and_public_informations
          &informations);

  void OnEnter() override;
  void OnExit() override;

 protected:
  ConnectionManager *connexionManager;

  PlayerPortrait *player_portrait_;
  MonsterThumb *monster_thumb_;

  PlayerCounter *player_counter_;

  UI::Button *bag;
  UI::Button *buy;
  UI::Button *crafting_btn_;
  Chat *chat_;
  UI::Button *menu_;
  MapMenu *map_menu_;

  UI::Row *action_buttons_;
  UI::Row *action_buttons2_;

  Toast *toast_;

  NpcTalk *npc_talk_;
  Tip *tip_;

  std::vector<ActionClan> actionClan;
  std::string clanName;
  bool haveClanInformations;

  void OnScreenResize() override;
  void menuClicked();
};
}  // namespace Scenes
#endif  // MULTI_H
