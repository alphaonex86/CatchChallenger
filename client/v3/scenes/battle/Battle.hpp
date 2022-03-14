// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_BATTLE_BATTLE_HPP_
#define CLIENT_V3_SCENES_BATTLE_BATTLE_HPP_

#include <QObject>
#include <unordered_map>

#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../../../general/base/MoveOnTheMap.hpp"
#include "../../action/Animation.hpp"
#include "../../action/Delay.hpp"
#include "../../action/MoveTo.hpp"
#include "../../action/Sequence.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/ActionManager.hpp"
#include "../../core/Node.hpp"
#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../entities/Map_client.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/LinkedDialog.hpp"
#include "../../ui/ListView.hpp"
#include "../../ui/MessageBar.hpp"
#include "../../ui/Progressbar.hpp"
#include "../shared/inventory/Inventory.hpp"
#include "ActionBar.hpp"
#include "BattleStates.hpp"
#include "StatusCard.hpp"

namespace Scenes {
class Battle : public QObject, public Scene {
  Q_OBJECT
 public:
  static Battle *Create();
  ~Battle();
  void ChangeLanguage();

  enum BattleType { BattleType_Wild, BattleType_Bot, BattleType_OtherPlayer };
  enum SkillData { kSkillId = 0, kSkillEndurance = 1 };
  enum BattleStep {
    BattleStep_Presentation, BattleStep_PresentationMonster,
    BattleStep_Middle,
    BattleStep_Final,
    kBattleStep_CallMonster,
    kBattleStep_Fight,
    kBattleStep_UseItem,
    kBattleStep_UseTrap
  };

  enum DoNextActionStep {
    DoNextActionStep_Start,
    DoNextActionStep_Loose,
    DoNextActionStep_Win
  };

  enum SpriteStatus {
    kStatusEnter = 0,
    kStatusExit = 1,
    kStatusDead = 2,
    kStatusDamage = 3,
    kExperienceGain = 4
  };
  enum SpriteLocation { kOutScreen = 0, kReadyForBattle = 1 };
  struct BattleInformations {
    std::string pseudo;
    uint8_t skinId;
    std::vector<uint8_t> stat;
    uint8_t monsterPlace;
    CatchChallenger::PublicPlayerMonster publicPlayerMonster;
  };

  enum BattleActionType {
    kBattleAction_CallMonster,
    kBattleAction_ChooseMonster,
    kBattleAction_Fight,
    kBattleAction_UseItem,
    kBattleAction_UseTrap,
    kBattleAction_Run,
    kBattleAction_ApplyBuff,
    kBattleAction_AddBuff,
    kBattleAction_RemoveBuff,
    kBattleAction_MonsterDead,
    kBattleAction_End,
    kBattleAction_None
  };

  struct BattleAction {
    BattleActionType type;
    bool do_by_player;
    CatchChallenger::ApplyOn apply_on;
    bool apply_on_enemy;
    bool success;

    uint16_t id;
    uint8_t monster_index;
    uint8_t level;
    uint32_t quantity;
    bool critical;
    float effective;
  };

  std::vector<BattleAction> actions_;

  BattleStep battleStep;

  bool WildFightInitialize(CatchChallenger::Map_client *map, const uint8_t &x,
                           const uint8_t &y);
  void BotFightInitialize(CatchChallenger::Map_client *map, const uint8_t &x,
                          const uint8_t &y, const uint16_t &fightId);
  void SetVariables();
  void OnEnter() override;
  void OnExit() override;
  CatchChallenger::Api_protocol_Qt *client_;
  BattleType battleType;

  ActionBar *action_bar_;
  ActionBar::ActionType action_type;
  StatusCard *player_status_;
  StatusCard *enemy_status_;
  UI::MessageBar *message_bar_;

  // Attack
  Sprite *attack_;
  Sequence *attack_animation_;

  // Player
  Sequence *player_enter_;
  Sequence *player_exit_;
  Sequence *player_dead_;
  Sprite *player_;

  // Both
  Sequence *both_enter_;
  Sequence *both_exit_;
  Sequence *both_dead_;
  Sequence *both_delay_;

  // Enemy
  Sprite *enemy_;
  Sequence *enemy_enter_;
  Sequence *enemy_exit_;
  Sequence *enemy_dead_;

  Sequence *sprite_damage_;

  QPointF player_in_;
  QPointF player_out_;
  QPointF enemy_in_;
  QPointF enemy_out_;

  void Win();
  void ShowStatusMessage(const std::string &text, bool show_next_btn = false,
                         bool use_timeout = true);
  void ShowStatusMessage(const QString &text, bool show_next_btn = false,
                         bool use_timeout = true);
  void ShowMonsterDialog(bool show_close);
  void ConfigureBattleground();
  void init_other_monster_display();
  void resetPosition(bool outOfScreen, bool topMonster, bool bottomMonster);
  void ConfigureFighters();
  void ConfigureCommons();
  void doNextAction();
  void updateCurrentMonsterInformation();
  void ProcessActions();

 private:
  Sprite *labelFightBackground;
  QPixmap labelFightBackgroundPix;
  Sprite *labelFightForeground;
  QPixmap labelFightForegroundPix;
  Sprite *labelFightPlateformBottom;
  QPixmap labelFightPlateformBottomPix;
  Sprite *labelFightPlateformTop;

  // bot
  CatchChallenger::Bot actualBot;

  QPixmap playerBackImage;
  QPixmap playerFrontImage;

  uint8_t monsterEvolutionPostion;

  bool escape;
  bool escapeSuccess;
  bool useTheRescueSkill;
  ConnectionManager *connexionManager;
  bool fightTimerFinish;
  uint16_t fightId;

  DoNextActionStep doNextActionStep;

  UI::LinkedDialog *linked_;
  UI::ListView *player_skills_;

  bool waiting_server_reply_;

  // Pointer to use it in lambda function
  MoveTo *enemy_move_enter_;
  MoveTo *enemy_move_exit_;
  MoveTo *enemy_move_dead_;

  // Trap
  Sprite *trap_;
  Sequence *trap_throw_;
  Sequence *trap_catch_;
  bool is_trap_success_;

  // Commons
  Sequence *status_timeout_;
  CatchChallenger::MonstersCollisionValue zone_collision_;

  // Experience
  Sequence *experience_gain_;
  uint32_t delta_given_xp_;
  uint8_t player_monster_lvl_;

  CatchChallenger::CommonFightEngine *fightEngine;
  std::vector<uint16_t> botFightList;
  std::vector<BattleInformations> battleInformationsList;
  std::vector<uint8_t> tradeEvolutionMonsters;

  BattleContext *battle_context_;
  QTimer wait_timer_;

  Battle();

  void OnWaitEngineMoveSlot();

  void OnPlayerExitDone();
  void OnPlayerEnterDone();
  void OnPlayerDeadDone();
  void OnEnemyExitDone();
  void OnEnemyEnterDone();
  void OnEnemyDeadDone();
  UI::Button *CreateSkillButton();
  void OnSkillClick(uint8_t index, uint16_t skill_id, uint8_t endurance);
  void UpdateAttackSkills();
  void OnMonsterSelect(uint8_t monster_index);
  void StartAttack();
  void OnAttackDone();
  void OnDamageDone();
  bool AddBuffEffect(CatchChallenger::PublicPlayerMonster *current_monster,
                     CatchChallenger::PublicPlayerMonster *other_monster,
                     const CatchChallenger::Skill::AttackReturn current_attack,
                     QString attack_owner);
  bool RemoveBuffEffect(
      CatchChallenger::PublicPlayerMonster *current_monster,
      CatchChallenger::PublicPlayerMonster *other_monster,
      const CatchChallenger::Skill::AttackReturn current_attack,
      QString attack_owner);
  bool ProcessAttack();
  void UseTrap(const uint16_t &item_id);
  void OnTrapThrowDone();
  void OnTrapCatchDone();
  void MonsterCatch(const bool &success);
  void OnActionNextClick(Node *);
  void OnActionAttackClick(Node *);
  void OnActionBagClick(Node *);
  void OnActionMonsterClick(Node *);
  void OnActionEscapeClick(Node *);
  void OnActionReturnClick(Node *);
  void ShowActionBarTab(int index);
  void CheckEvolution();
  void OnExperienceGainDone();
  void ProcessEnemyMonsterKO();
  void OnExperienceIncrementDone();
  void OnBothActionDone(int8_t type);
  void BotFightFullDiffered();
  void Finished();

  void updateOtherMonsterInformation();
  void updateCurrentMonsterInformationXp();
  void loose();
  void botFightFull(const uint16_t &fightId);
  void prepareFight();
  QPixmap GetBackSkin(const uint32_t &skinId);

  void battleAcceptedByOtherFull(const BattleInformations &battleInformations);
  void ShowMessageDialog(const QString &title, const QString &message);
  void ShowBagDialog();
  void UseBagItem(Scenes::Inventory::ObjectType type, uint16_t item,
                  uint32_t quantity);
  void newError(std::string error, std::string detailedError);
  void error(std::string error);
  void TeleportToSlot(const uint32_t &mapId, const uint16_t &x,
                      const uint16_t &y,
                      const CatchChallenger::Direction &direction);

 protected:
  void OnScreenSD() override;
  void OnScreenHD() override;
  void OnScreenHDR() override;
  void OnScreenResize() override;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_BATTLE_HPP_
