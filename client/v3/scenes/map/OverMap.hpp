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
#include "Tip.hpp"

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

  /*
  void mousePressEventXY(const QPointF &p, bool &pressValidated,
                         bool &callParentClass) override;
  void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,
                           bool &callParentClass) override;
  void mouseMoveEventXY(const QPointF &p,
                        bool &pressValidated,
                        bool &callParentClass);
  void keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
  void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
  */
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

  Sprite *playersCountBack;
  UI::Label *playersCount;

  UI::Button *bag;
  UI::Label *bagOver;
  UI::Button *buy;
  UI::Label *buyOver;
  UI::Button *crafting_btn_;
  Chat *chat_;

  Toast *toast_;

  NpcTalk *npc_talk_;
  Tip *tip_;

  Sprite *persistant_tipBack;
  UI::Label *persistant_tip;
  QString persistant_tipString;

  std::vector<ActionClan> actionClan;
  std::string clanName;
  bool haveClanInformations;

  void OnScreenResize() override;
};
}  // namespace Scenes
#endif  // MULTI_H
