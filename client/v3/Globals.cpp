// Copyright 2021 CatchChallenger
#include "Globals.hpp"

#include "scenes/leading/Loading.hpp"

#ifndef NOSINGLEPLAYER
CatchChallenger::InternalServer *Globals::InternalServer = nullptr;
#endif

#ifndef NOTHREADS
QThread *Globals::ThreadSolo = nullptr;
#endif

Scenes::Leading *Globals::leading_ = nullptr;

Scenes::Map *Globals::map_ = nullptr;

Scenes::Battle *Globals::battle_ = nullptr;

UI::MessageDialog *Globals::alert_ = nullptr;

UI::InputDialog *Globals::input_ = nullptr;

bool Globals::is_solo_ = false;

Scenes::MonsterPicker *Globals::monster_picker_ = nullptr;

Scenes::Leading *Globals::GetLeadingScene() {
  if (!leading_) {
    leading_ = Scenes::Leading::Create();
  }
  return leading_;
}

Scenes::Map *Globals::GetMapScene() {
  if (!map_) {
    map_ = Scenes::Map::Create();
  }
  return map_;
}

Scenes::Battle *Globals::GetBattleScene() {
  if (!battle_) {
    battle_ = Scenes::Battle::Create();
  }
  return battle_;
}

UI::MessageDialog *Globals::GetAlertDialog() {
  if (!alert_) {
    alert_ = UI::MessageDialog::Create();
  }
  return alert_;
}

UI::InputDialog *Globals::GetInputDialog() {
  if (!input_) {
    input_ = UI::InputDialog::Create();
  }
  return input_;
}

Scenes::MonsterPicker *Globals::GetMonsterPicker() {
  if (!monster_picker_) {
    monster_picker_ = Scenes::MonsterPicker::Create();
  }
  return monster_picker_;
}

void Globals::ClearAllScenes() {
  Scenes::Loading::GetInstance()->SetParent(nullptr);
  if (map_ != nullptr) {
    delete map_;
    map_ = nullptr;
  }
  if (battle_ != nullptr) {
    delete battle_;
    battle_ = nullptr;
  }
  if (leading_ != nullptr) {
    delete leading_;
    leading_ = nullptr;
  }
}

bool Globals::IsSolo() {
  return is_solo_;
}

void Globals::SetSolo(bool is_solo) {
  is_solo_ = is_solo;
}
