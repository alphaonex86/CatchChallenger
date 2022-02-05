// Copyright 2021 CatchChallenger
#include "Crafting.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"

using Scenes::Crafting;
using std::placeholders::_1;
using std::placeholders::_2;

Crafting::Crafting() {
  SetDialogSize(1200, 600);
  selected_ = nullptr;

  material_ = UI::Label::Create(this);
  name_ = UI::Label::Create(this);
  details_ = UI::Label::Create(this);
  material_->SetAlignment(Qt::AlignCenter);
  item_icon_ = Sprite::Create(this);
  item_icon_->TestMode();

  craft_content_ = UI::ListView::Create(this);
  craft_content_->TestMode();
  materials_content_ = UI::ListView::Create(this);
  materials_content_->TestMode();

  create_ = UI::Button::Create(":/CC/images/interface/button.png", this);
  create_->SetOnClick(std::bind(&Crafting::OnCreateClick, this));

  NewLanguage();
}

Crafting::~Crafting() {
}

Crafting *Crafting::Create() { return new (std::nothrow) Crafting(); }

void Crafting::NewLanguage() {
  SetTitle(tr("Crafting"));
  create_->SetText(tr("Create"));
  material_->SetText(tr("Materials"));
}

void Crafting::OnEnter() {
  UI::Dialog::OnEnter();
  craft_content_->RegisterEvents();
  materials_content_->RegisterEvents();
  create_->RegisterEvents();

  name_->SetText(QString::fromStdString("sasd"));

  OnScreenResize();
}

void Crafting::OnExit() {
  craft_content_->UnRegisterEvents();
  materials_content_->UnRegisterEvents();
  create_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void Crafting::OnCreateClick() {
  auto &info = PlayerInfo::GetInstance()->GetInformation();
  auto player_info = PlayerInfo::GetInstance();

}

void Crafting::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();
  qreal width = content.width() / 3;
  qreal block_1 = content.x();
  qreal block_2 = content.x() + width + 20;

  craft_content_->SetPos(block_1, content.y());
  craft_content_->SetSize(width, content.height());

  item_icon_->SetPos(block_2, content.y());
  item_icon_->SetSize(60, 60);

  name_->SetPos(item_icon_->Right() + 10, content.y());
  details_->SetPos(name_->X(), name_->Bottom() + 10);

  material_->SetPos(block_2, details_->Y() + 50);

  materials_content_->SetPos(block_2, material_->Bottom() + 10);
}

void Crafting::OnItemClick(Node *node) {
  selected_ = node;
  auto items = craft_content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    //static_cast<CraftingItem *>(item)->SetSelected(false);
  }
}

void Crafting::LoadMaterials() {

}
