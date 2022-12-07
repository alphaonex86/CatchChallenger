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
#include "../../entities/CommonTypes.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/LinkedDialog.hpp"
#include "../../ui/ListView.hpp"
#include "../../ui/Progressbar.hpp"
#include "../shared/inventory/Inventory.hpp"

namespace Scenes {
class Battle : public QObject, public Scene {
  Q_OBJECT
 public:
  static Battle *Create();
  ~Battle();
  void ChangeLanguage();

  bool check_monsters();
  bool WildFightInitialize(CatchChallenger::Map_client *map, const uint8_t &x,
                           const uint8_t &y);
  void BotFightInitialize(CatchChallenger::Map_client *map, const uint8_t &x,
                          const uint8_t &y, const uint16_t &fightId);
  void SetVariables();
  void OnEnter() override;
  void OnExit() override;

  enum BattleType { BattleType_Wild, BattleType_Bot, BattleType_OtherPlayer };
  enum SkillData { kSkillId = 0, kSkillEndurance = 1 };
  enum BattleStep {
    BattleStep_Presentation,
    BattleStep_PresentationMonster,
    BattleStep_Middle,
    BattleStep_Final
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

  BattleStep battleStep;
  BattleType battleType;
  DoNextActionStep doNextActionStep;

  UI::LinkedDialog *linked_;
  UI::ListView *player_skills_;
  ActionManager *action_manager_;

  bool waiting_server_reply_;

  // Action bar
  Sprite *action_background_;
  UI::Label *status_label_;
  UI::Button *action_next_;
  UI::Button *action_attack_;
  UI::Button *action_bag_;
  UI::Button *action_monster_;
  UI::Button *action_escape_;
  UI::Button *action_return_;

  // Player
  Sprite *player_;
  Sequence *player_enter_;
  Sequence *player_exit_;
  Sequence *player_dead_;
  UI::Progressbar *player_hp_bar_;
  UI::Progressbar *player_exp_bar_;
  UI::ListView *player_buff_;
  Sprite *player_background_;
  UI::Label *player_name_;
  UI::Label *player_lvl_;

  // Enemy
  Sprite *enemy_;
  Sequence *enemy_enter_;
  Sequence *enemy_exit_;
  Sequence *enemy_dead_;
  // Pointer to use it in lambda function
  MoveTo *enemy_move_enter_;
  MoveTo *enemy_move_exit_;
  MoveTo *enemy_move_dead_;
  UI::Progressbar *enemy_hp_bar_;
  UI::ListView *enemy_buff_;
  Sprite *enemy_background_;
  UI::Label *enemy_name_;

  // Attack
  Sprite *attack_;
  Sequence *attack_animation_;

  // Trap
  Sprite *trap_;
  Sequence *trap_throw_;
  Sequence *trap_catch_;
  bool is_trap_success_;

  // Commons
  Sequence *sprite_damage_;
  Sequence *status_timeout_;
  CatchChallenger::MonstersCollisionValue zone_collision_;

  // Both
  Sequence *both_enter_;
  Sequence *both_exit_;
  Sequence *both_dead_;
  Sequence *both_delay_;

  // Experience
  Sequence *experience_gain_;
  uint32_t delta_given_xp_;
  uint8_t player_monster_lvl_;

  CatchChallenger::CommonFightEngine *fightEngine;
  CatchChallenger::Api_protocol_Qt *client_;
  std::vector<uint16_t> botFightList;
  std::vector<BattleInformations> battleInformationsList;
  std::vector<uint8_t> tradeEvolutionMonsters;

  Battle();
  void ConfigureFighters();
  void ConfigureCommons();
  void OnPlayerExitDone();
  void OnPlayerEnterDone();
  void OnPlayerDeadDone();
  void OnEnemyExitDone();
  void OnEnemyEnterDone();
  void OnEnemyDeadDone();
  void OnStatusActionDone();
  UI::Button *CreateSkillButton();
  void OnSkillClick(Node *button);
  void ShowStatusMessage(const std::string &text, bool show_next_btn = false,
                         bool use_timeout = true);
  void ShowStatusMessage(const QString &text, bool show_next_btn = false,
                         bool use_timeout = true);
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
  void ConfigureBattleground();
  void Win();
  void CheckEvolution();
  void OnExperienceGainDone();
  void ProcessEnemyMonsterKO();
  void OnExperienceIncrementDone();
  void OnBothActionDone(int8_t type);
  void BotFightFullDiffered();
  void PlayerMonsterInitialize(CatchChallenger::PlayerMonster *fight_monster);
  void Finished();

  void doNextAction();
  void updateCurrentMonsterInformation();
  void updateOtherMonsterInformation();
  void updateCurrentMonsterInformationXp();
  void loose();
  void resetPosition(bool outOfScreen, bool topMonster, bool bottomMonster);
  void botFightFull(const uint16_t &fightId);
  void prepareFight();
  QPixmap GetBackSkin(const uint32_t &skinId);
  void load_monsters();

  void init_other_monster_display();
  void battleAcceptedByOtherFull(const BattleInformations &battleInformations);
  void ShowMessageDialog(const QString &title, const QString &message);
  void ShowBagDialog();
  void ShowMonsterDialog(bool show_close);
  void UseBagItem(ObjectCategory type, uint16_t item,
                  uint32_t quantity, uint8_t monster_index);
  void newError(std::string error, std::string detailedError);
  void error(std::string error);
  void TeleportToSlot(const uint32_t &mapId, const uint16_t &x,
                      const uint16_t &y, const CatchChallenger::Direction &direction);

 protected:
  void OnScreenSD() override;
  void OnScreenHD() override;
  void OnScreenHDR() override;
  void OnScreenResize() override;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_BATTLE_BATTLE_HPP_
