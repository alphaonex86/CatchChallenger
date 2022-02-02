// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_ADDCHARACTER_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_ADDCHARACTER_HPP_

#include <string>
#include <vector>

#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Combo.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Label.hpp"

namespace Scenes {
class AddCharacter : public UI::Dialog {
 public:
  static AddCharacter *Create();
  ~AddCharacter();

  bool isOk() const;
  void setDatapack(std::string path);
  void on_cancel_clicked();
  void on_ok_clicked();

  void loadProfileText();
  int getProfileIndex();
  int getProfileCount();
  void on_comboBox_currentIndexChanged(int index);
  void newLanguage();
  void OnActionClick(Node *node);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize();

 private:
  UI::Button *quit;
  UI::Button *validate;

  UI::Combo *comboBox;

  UI::Label *description;

  bool ok;

  std::string datapackPath;
  struct ProfileText {
    std::string name;
    std::string description;
  };
  std::vector<ProfileText> profileTextList;

  AddCharacter();
  void removeAbove();
};
}  // namespace Scenes

#endif  // CLIENT_QTOPENGL_SCENES_LEADING_ADDCHARACTER_HPP_
