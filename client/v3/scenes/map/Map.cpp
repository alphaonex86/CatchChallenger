// Copyright 2021 CatchChallenger
#include "Map.hpp"

#include <iostream>
#include <string>

using Scenes::Map;

Map::Map() {
  // todo: optimise by prevent create each time
  background_ = CCMap::Create();

  overmap_ = OverMapLogic::Create();
  /*
  if (!connect(overmap, &OverMapLogic::error, this,
               &ScreenTransition::errorString))
    abort();*/
  overmap_->resetAll();
  overmap_->setVar(background_);
  overmap_->connectAllSignals();
  SetBackground(background_);
  PushForeground(overmap_);
}

Map::~Map() {
  delete background_;
  delete overmap_;
}

Map* Map::Create() {
  Map* instance = new (std::nothrow) Map();
  return instance;
}
