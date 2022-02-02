// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_SUBSERVER_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_SUBSERVER_HPP_

#include <QIcon>
#include <QSet>
#include <QString>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../../general/base/GeneralStructures.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Checkbox.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ListView.hpp"
#include "CharacterSelector.hpp"

namespace Scenes {
class SubServer : public Scene {
 public:
  ~SubServer();

  static SubServer *Create();

  void server_select_clicked();
  void newLanguage();
  void logged(std::vector<CatchChallenger::ServerFromPoolForDisplay> servers);
  void itemSelectionChanged(Node *node);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  CharacterSelector *character_selector_;

  std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList;
  UI::Button *server_select;
  UI::Button *back;

  UI::ListView *serverList;

  Sprite *wdialog;
  uint32_t averagePlayedTime;
  uint32_t averageLastConnect;
  int server_selected_;
  bool is_loaded_;
  std::unordered_map<
      uint8_t /*character group index*/,
      std::pair<uint8_t /*server count*/, uint8_t /*temp Index to display*/>>
      serverByCharacterGroup;
  std::vector<std::vector<CatchChallenger::CharacterEntry>> characters_;

  QString icon_server_list_star1;
  QString icon_server_list_star2;
  QString icon_server_list_star3;
  QString icon_server_list_star4;
  QString icon_server_list_star5;
  QString icon_server_list_star6;
  QString icon_server_list_stat1;
  QString icon_server_list_stat2;
  QString icon_server_list_stat3;
  QString icon_server_list_stat4;
  QString icon_server_list_bug;
  std::vector<QString> icon_server_list_color;
  ConnectionManager *connection_;

  SubServer();
  void Initialize();
  void backMulti();
  void addToServerList(CatchChallenger::LogicialGroup &logicialGroup,
                       const uint64_t &currentDate, const bool &fullView,
                       uint8_t level);
  void OnLogged(
      std::vector<std::vector<CatchChallenger::CharacterEntry>> characters);
};
}  // namespace Scenes

#endif  // CLIENT_QTOPENGL_SCENES_LEADING_SUBSERVER_HPP_
