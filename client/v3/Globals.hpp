// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_GLOBALS_HPP_
#define CLIENT_V3_GLOBALS_HPP_

#include <QThread>

#ifndef NOSINGLEPLAYER
#include "../../server/qt/InternalServer.hpp"
#endif
#include "scenes/battle/Battle.hpp"
#include "scenes/leading/Leading.hpp"
#include "scenes/map/Map.hpp"
#include "ui/MessageDialog.hpp"
#include "ui/InputDialog.hpp"

class Globals {
 public:
#ifndef NOSINGLEPLAYER
  static CatchChallenger::InternalServer* InternalServer;
#endif
#ifndef NOTHREADS
  static QThread* ThreadSolo;
#endif
  static Scenes::Leading* GetLeadingScene();
  static Scenes::Map* GetMapScene();
  static Scenes::Battle* GetBattleScene();
  static UI::MessageDialog* GetAlertDialog();
  static UI::InputDialog* GetInputDialog();
  static void ClearAllScenes();

  static bool IsSolo();
  static void SetSolo(bool is_solo);

 private:
  static Scenes::Leading* leading_;
  static Scenes::Map* map_;
  static Scenes::Battle* battle_;
  static UI::MessageDialog *alert_;
  static UI::InputDialog *input_;
  static bool is_solo_;
};

#endif  // CLIENT_V3_GLOBALS_HPP_
