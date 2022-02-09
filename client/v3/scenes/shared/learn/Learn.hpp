// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_LEARN_LEARN_HPP_
#define CLIENT_V3_SCENES_SHARED_LEARN_LEARN_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../../../general/base/GeneralStructuresXml.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/ThemedButton.hpp"

namespace Scenes {
class Learn : public UI::Dialog {
 public:
  static Learn *Create();
  ~Learn();

  void Draw(QPainter *painter) override;
  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;
  void Close();
  void OnActionClick(Node *node);

 private:
  Learn();
  void OnItemClick(Node *node);
  void LoadLearnSkills(uint monster_pos);
  void NewLanguage();

  Sprite *portrait_;
  UI::Button *quit_;
  UI::Button *learn_;
  UI::Button *monster_selector_;
  UI::Label *sp_;
  UI::Label *info_;
  UI::Label *description_;
  UI::ListView *content_;
  Node *selected_;

  CatchChallenger::PlayerMonster current_monster_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_LEARN_LEARN_HPP_
