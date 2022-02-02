// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_NEWGAME_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_NEWGAME_HPP_

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Row.hpp"

namespace Scenes {
class NewGame : public UI::Dialog {
 public:
  ~NewGame();
  static NewGame *Create();

  void setDatapack(
      const std::string &skinPath, const std::string &monsterPath,
      std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup,
      const std::vector<uint8_t> &forcedSkin);
  void updateSkin();
  void on_cancel_clicked();
  void on_ok_clicked();
  bool haveSkin() const;
  bool isOk() const;
  void on_next_clicked();
  void on_previous_clicked();
  void on_pseudo_returnPressed();
  std::string pseudo();
  uint8_t skinId();
  uint8_t monsterGroupId();
  bool haveTheInformation();
  bool okCanBeEnabled();
  std::string skin();
  void on_pseudo_textChanged(const QString &);
  void newLanguage();
  void OnActionClick(Node *node);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  UI::Button *quit;
  UI::Button *validate;

  UI::Button *previous;
  UI::Row *row_;
  UI::Button *next;
  UI::Input *uipseudo;
  UI::Label *warning;

  int x, y;

  std::vector<uint8_t> forcedSkin;
  std::string monsterPath;
  std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup;
  enum Step {
    Step1,
    Step2,
    StepOk,
  };
  Step step;
  bool ok;
  bool skinLoaded;
  std::vector<std::string> skinList;
  std::vector<uint8_t> skinListId;
  uint8_t currentSkin;
  uint8_t currentMonsterGroup;
  std::string skinPath;

  NewGame();
  void removeAbove();
};
}  // namespace Scenes

#endif  // CLIENT_QTOPENGL_SCENES_LEADING_NEWGAME_HPP_
