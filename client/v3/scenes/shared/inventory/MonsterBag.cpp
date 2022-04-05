// Copyright 2021 <CatchChallenger>
#include "MonsterBag.hpp"

#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/SceneManager.hpp"
#include "../../../entities/Shapes.hpp"
#include "MonsterItem.hpp"

using CatchChallenger::PlayerMonster;
using Scenes::MonsterBag;
using std::placeholders::_1;

MonsterBag::MonsterBag(Mode mode) {
  on_select_monster_ = nullptr;
  backdrop_ = UI::Backdrop::Create(
      std::bind(&MonsterBag::BackgroundCanvas, this, _1), this);

  monster_list_ = UI::Column::Create(this);
  monster_list_->SetItemSpacing(10);

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
}

void MonsterBag::LoadMonster() {
  auto width = bounding_rect_.width() / 4;
  auto height = (bounding_rect_.height() - 200) / 7;
  const std::vector<PlayerMonster> &monsters = client_->getPlayerMonster();

  monster_list_->RemoveAllChildrens(true);
  auto i = monsters.begin();
  int index = 0;
  while (i != monsters.cend()) {
    auto item = MonsterItem::Create(*i);
    item->SetData(99, (*i).monster);
    item->SetData(98, index);
    item->SetOnClick(std::bind(&MonsterBag::OnSelectItem, this, _1));
    item->SetSize(width, height);
    monster_list_->AddChild(item);
    ++i;
    index++;
  }
}

void MonsterBag::OnSelectItem(Node *node) {
  selected_item_ = node;
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

void MonsterBag::SetOnSelectMonster(std::function<void(uint8_t)> callback) {
  on_select_monster_ = callback;
}

void MonsterBag::SetMode(Mode mode) {
  if (mode_ == mode) return;
  mode_ = mode;

}

void MonsterBag::OnScreenSD() {}

void MonsterBag::OnScreenHD() {}

void MonsterBag::OnScreenHDR() {}

void MonsterBag::OnScreenResize() {
  backdrop_->SetSize(bounding_rect_.width(), bounding_rect_.height());
  monster_list_->SetPos(20, 50);
}

void MonsterBag::BackgroundCanvas(QPainter *painter) {
  painter->setPen(Qt::NoPen);
  painter->setBrush(QColor(232, 251, 255));
  painter->drawRect(0, 0, bounding_rect_.width(), bounding_rect_.height());
  qreal padding = bounding_rect_.width() / 4;
  auto rect = QRectF(0, 0, bounding_rect_.width(), bounding_rect_.height());
  auto painter_path = Shapes::DrawDiamond(rect, padding);
  painter_path.translate(-padding, 0);
  painter->setBrush(QColor(196, 40, 53));
  painter->drawPath(painter_path);
  painter_path.translate(-200, 0);
  painter->setBrush(QColor(227, 49, 61));
  painter->drawPath(painter_path);
}

void MonsterBag::OnEnter() { 
  Scene::OnEnter();
  LoadMonster();
  monster_list_->RegisterEvents();
}

void MonsterBag::OnExit() {
  monster_list_->UnRegisterEvents();
}
