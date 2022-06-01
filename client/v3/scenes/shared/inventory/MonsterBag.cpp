// Copyright 2021 <CatchChallenger>
#include "MonsterBag.hpp"

#include <iostream>

#include "../../../Globals.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/SceneManager.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "MonsterItem.hpp"

using CatchChallenger::PlayerMonster;
using Scenes::MonsterBag;
using std::placeholders::_1;

MonsterBag::MonsterBag(Mode mode) {
  on_select_monster_ = nullptr;
  monster_list_ = UI::ListView::Create(this);
  monster_list_->SetItemSpacing(10);

  select_ = UI::Button::Create(":/CC/images/interface/validate.png");
  select_->SetOnClick(std::bind(&MonsterBag::OnAcceptClicked, this));

  connection_manager_ = ConnectionManager::GetInstance();
  client_ = connection_manager_->client;
  SetMode(mode);
}

MonsterBag::~MonsterBag() {
  delete monster_list_;
  monster_list_ = nullptr;
}

MonsterBag *MonsterBag::Create(Mode mode) {
  return new (std::nothrow) MonsterBag(mode);
}

void MonsterBag::Draw(QPainter *painter) {
  auto b2 = Sprite::Create(":/CC/images/interface/b2.png");
  b2->Strech(24, 22, 20, Width(), Height());
  b2->SetPos(0, 0);
  b2->Render(painter);
}

void MonsterBag::OnResize() {
  monster_list_->SetPos(20, 20);
  monster_list_->SetSize(Width() - 40, Height() - 40);
  ReDraw();
}

void MonsterBag::LoadMonster() {
  const std::vector<PlayerMonster> &monsters = client_->getPlayerMonster();

  monster_list_->Clear();
  auto i = monsters.begin();
  int index = 0;
  while (i != monsters.cend()) {
    auto item = MonsterItem::Create(*i);
    item->SetData(99, (*i).monster);
    item->SetData(98, index);
    item->SetOnClick(std::bind(&MonsterBag::OnSelectItem, this, _1));
    monster_list_->AddItem(item);
    ++i;
    index++;
  }
}

void MonsterBag::OnSelectItem(Node *node) {
  selected_item_ = node;
  auto items = monster_list_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<MonsterItem *>(item)->SetSelected(false);
  }
}

void MonsterBag::OnAcceptClicked() {
  if (selected_item_->Data(99) == client_->getCurrentMonster()->monster) {
    ShowMessageDialog(QObject::tr("Warning"),
                      QObject::tr("The monster selected is already on battle"));
    return;
  }
  on_select_monster_(selected_item_->Data(98));
}

void MonsterBag::ClearSelection() {}

void MonsterBag::ShowMessageDialog(const QString &title,
                                   const QString &message) {
  ClearSelection();
  Globals::GetAlertDialog()->Show(title, message);
}

void MonsterBag::RegisterEvents() {
  auto linked = static_cast<UI::LinkedDialog *>(Parent());
  linked->SetTitle(QObject::tr("MONSTERS"));
  LoadMonster();
  monster_list_->RegisterEvents();
  if (show_select_) {
    linked->AddActionButton(select_);
  }
}

void MonsterBag::UnRegisterEvents() {
  monster_list_->UnRegisterEvents();
  if (Parent() == nullptr) return;
  static_cast<UI::LinkedDialog *>(Parent())->RemoveActionButton(select_);
}

void MonsterBag::ShowSelectAction(bool show) {
  if (show_select_ == show) return;

  show_select_ = show;
  if (Parent() == nullptr) return;
  if (show_select_) {
    static_cast<UI::LinkedDialog *>(Parent())->AddActionButton(select_);
  } else {
    static_cast<UI::LinkedDialog *>(Parent())->RemoveActionButton(select_);
  }
}

void MonsterBag::SetOnSelectMonster(std::function<void(uint8_t)> callback) {
  on_select_monster_ = callback;
}

void MonsterBag::SetMode(Mode mode) {
  if (mode_ == mode) return;
  mode_ = mode;

  switch (mode_) {
    case kOnBattle:
      ShowSelectAction(true);
      break;
    case kOnItemUse:
      ShowSelectAction(true);
      break;
    case kDefault:
      ShowSelectAction(false);
      break;
  }
}

