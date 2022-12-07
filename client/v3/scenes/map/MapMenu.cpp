// Copyright 2022 CatchChallenger
#include "MapMenu.hpp"

#include "../../Constants.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/StackedScene.hpp"
#include "../../ui/ThemedButton.hpp"

using Scenes::MapMenu;

MapMenu::MapMenu() : UI::Dialog() {
  SetDialogSize(Constants::DialogSmallSize());
  SetTitle("Menu");

  options_dialog_ = nullptr;

  buttons_ = UI::Column::Create(this);
  buttons_->SetItemSpacing(Constants::ItemMediumSpacing());

  options_ = UI::GreenButton::Create();
  options_->SetText(tr("Options"));
  options_->SetOnClick([&](Node *node) {
    if (options_dialog_ == nullptr) {
      options_dialog_ = Scenes::Options::Create();
    }
    AddChild(options_dialog_);
  });
  buttons_->AddChild(options_);

  go_to_main_ = UI::GreenButton::Create();
  go_to_main_->SetText(tr("Go to main"));
  go_to_main_->SetOnClick([&](Node *node) {
    SceneManager::GetInstance()->PopScene();
    static_cast<StackedScene *>(SceneManager::GetInstance()->CurrentScene())
        ->Restart();
  });
  buttons_->AddChild(go_to_main_);

  exit_ = UI::GreenButton::Create();
  exit_->SetText(tr("Exit Catchchallenger"));
  exit_->SetOnClick([&](Node *node) {

  });
  buttons_->AddChild(exit_);
}

MapMenu::~MapMenu() {
  delete buttons_;
  buttons_ = nullptr;
}

MapMenu *MapMenu::Create() { return new (std::nothrow) MapMenu(); }

void MapMenu::OnEnter() {
  UI::Dialog::OnEnter();
  buttons_->RegisterEvents();
}

void MapMenu::OnExit() {
  buttons_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void MapMenu::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto content = ContentBoundary();
  buttons_->SetPos(content.x(), content.y());

  go_to_main_->SetWidth(content.width());
  exit_->SetWidth(content.width());
  options_->SetWidth(content.width());
}
