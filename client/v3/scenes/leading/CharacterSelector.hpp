// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_CHARACTERSELECTOR_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_CHARACTERSELECTOR_HPP_

#include <QString>
#include <vector>

#include "../../../general/base/GeneralStructures.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/Scene.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Checkbox.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ListView.hpp"
#include "AddCharacter.hpp"
#include "NewGame.hpp"

namespace Scenes {
class CharacterSelector : public Scene {
 public:
  ~CharacterSelector();

  static CharacterSelector *Create();
  void add_clicked();
  void add_finished();
  void newGame_finished();
  void select_clicked();
  void remove_clicked();
  void newLanguage();
  void updateCharacterList(bool prevent_auto_select = false);
  void connectToSubServer(
      const int indexSubServer,
      const std::vector<std::vector<CatchChallenger::CharacterEntry>>
          &characterEntryList);
  void newCharacterId(const uint8_t &returnCode, const uint32_t &characterId);
  void OnCharacterClick(Node *node);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  AddCharacter *addCharacter;
  NewGame *newGame;

  UI::Button *add;
  UI::Button *remove;
  UI::Button *select;
  UI::Button *back;

  UI::ListView *characterEntryList;

  Sprite *wdialog;
  unsigned int serverSelected;
  ConnectionManager *connection_;
  std::unordered_map<
      uint8_t /*character group index*/,
      std::pair<uint8_t /*server count*/, uint8_t /*temp Index to display*/>>
      serverByCharacterGroup;
  std::vector<std::vector<CatchChallenger::CharacterEntry>>
      characterListForSelection;
  std::vector<CatchChallenger::CharacterEntry> characterEntryListInWaiting;
  unsigned int profileIndex;
  Node *selected_character_;

  CharacterSelector();
  void backSubServer();
  void selectCharacter(const int indexSubServer, const int indexCharacter);
  void GoToMap();
};
}  // namespace Scenes

#endif  // CLIENT_QTOPENGL_SCENES_LEADING_CHARACTERSELECTOR_HPP_
