// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENE_MAP_MONSTERDETAILS_HPP_ 
#define CLIENT_V3_SCENE_MAP_MONSTERDETAILS_HPP_

#include "../../../general/base/GeneralStructures.hpp"
#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Backdrop.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ListView.hpp"
#include "../../ui/Progressbar.hpp"

namespace Scenes {
class MonsterDetails : public Scene {
 public:
  ~MonsterDetails();
  static MonsterDetails* Create();

  void SetMonster(const CatchChallenger::PlayerMonster &monster);
  void NewLanguage();
  void Draw(QPainter *painter) override;
  void OnScreenResize() override;
  void OnEnter() override;
  void OnExit() override;
  void Close();
  void SetOnClose(std::function<void()> callback);

 private:
  UI::Button *quit;
  UI::Label *details;

  Sprite *monsterDetailsImage;

  UI::ListView *monsterDetailsSkills;

  UI::Backdrop *backdrop_;
  Sprite *monsterDetailsCatched;
  UI::Label *monsterDetailsName;
  UI::Label *monsterDetailsLevel;
  UI::Label *hp_text;
  UI::Progressbar *hp_bar;
  UI::Label *xp_text;
  UI::Progressbar *xp_bar;
  std::function<void()> on_close_;

  MonsterDetails();
  void DrawBackground(QPainter *painter);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENE_MAP_MONSTERDETAILS_HPP_
