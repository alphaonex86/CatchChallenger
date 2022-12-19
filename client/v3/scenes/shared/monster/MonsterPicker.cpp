// Copyright 2022 CatchChallenger
#include "MonsterPicker.hpp"

#include <iostream>
#include "../../../Constants.hpp"
#include "../../../core/SceneManager.hpp"

using std::placeholders::_1;
using Scenes::MonsterPicker;
using Scenes::MonsterBag;
using UI::LinkedDialog;

MonsterPicker::MonsterPicker(): LinkedDialog(false) {
  SetDialogSize(Constants::DialogSmallSize());
  bag_ = MonsterBag::Create(MonsterBag::kOnItemUse);
  AddItem(bag_, "monsters");

  SetCurrentItem("monsters");
  SetOnClose([&](){
    SceneManager::GetInstance()->RemoveOverlay();
  });
}

MonsterPicker* MonsterPicker::Create() {
  return new (std::nothrow)MonsterPicker();
}

MonsterPicker::~MonsterPicker() {
  delete bag_;
  bag_ = nullptr;
}

void MonsterPicker::Show(std::function<void(uint8_t)> on_select) {
  bag_->SetOnSelectMonster(on_select);
  SceneManager::GetInstance()->ShowOverlay(this);
}
