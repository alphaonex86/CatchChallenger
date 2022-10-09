// Copyright 2021 CatchChallenger
#include "Leading.hpp"

#include <iostream>
#include <string>

#include "../test/Test.hpp"

using Scenes::Leading;

Leading::Leading() {
  background_ = ParallaxForest::Create();
  SetBackground(background_);

  menu_ = Menu::Create();
  PushForeground(menu_);

  // Activate when you want to test single widget
  //auto test = Test::Create();
  //PushForeground(test);
}

Leading::~Leading() {
  delete background_;
  background_ = nullptr;
  delete menu_;
  menu_ = nullptr;
}

Leading* Leading::Create() {
  Leading* instance = new (std::nothrow) Leading();
  return instance;
}

void Leading::Restart() {
  PopForegroundUntilIndex(0);
}
