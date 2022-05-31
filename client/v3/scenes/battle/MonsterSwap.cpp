// Copyright 2021 <CatchChallenger>
#include "MonsterSwap.hpp"

#include <iostream>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Globals.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/SceneManager.hpp"
#include "../../entities/Shapes.hpp"
#include "../shared/inventory/MonsterItem.hpp"

using CatchChallenger::PlayerMonster;
using Scenes::MonsterSwap;
using std::placeholders::_1;

MonsterSwap::MonsterSwap() {
  on_select_monster_ = nullptr;
  backdrop_ = UI::Backdrop::Create(
      std::bind(&MonsterSwap::BackgroundCanvas, this, _1), this);

  monster_list_ = UI::Column::Create(this);
  monster_list_->SetItemSpacing(10);
  title_ = UI::PlainLabel::Create(Qt::white, this);
  title_->SetText(QString::fromStdString("Can Battle"));
  title_->SetWidth(400);

  connection_manager_ = ConnectionManager::GetInstance();
  client_ = connection_manager_->client;
}

MonsterSwap::~MonsterSwap() {
  delete monster_list_;
  monster_list_ = nullptr;
}

MonsterSwap *MonsterSwap::Create() { return new (std::nothrow) MonsterSwap(); }

void MonsterSwap::Draw(QPainter *painter) {}

void MonsterSwap::LoadMonster() {
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
    item->SetOnClick(std::bind(&MonsterSwap::OnSelectItem, this, _1));
    item->SetSize(width, height);
    monster_list_->AddChild(item);
    ++i;
    index++;
  }
}

void MonsterSwap::OnSelectItem(Node *node) { selected_item_ = node; }

void MonsterSwap::OnAcceptClicked() {
  if (selected_item_->Data(99) == client_->getCurrentMonster()->monster) {
    ShowMessageDialog(QObject::tr("Warning"),
                      QObject::tr("The monster selected is already on battle"));
    return;
  }
  on_select_monster_(selected_item_->Data(98));
}

void MonsterSwap::ClearSelection() {}

void MonsterSwap::ShowMessageDialog(const QString &title,
                                    const QString &message) {
  ClearSelection();
  Globals::GetAlertDialog()->Show(title, message);
}

void MonsterSwap::SetOnSelectMonster(std::function<void(uint8_t)> callback) {
  on_select_monster_ = callback;
}

void MonsterSwap::OnScreenSD() {}

void MonsterSwap::OnScreenHD() {}

void MonsterSwap::OnScreenHDR() {}

void MonsterSwap::OnScreenResize() {
  backdrop_->SetSize(bounding_rect_.width(), bounding_rect_.height());
  monster_list_->SetPos(20, 50);
  title_->SetPos(bounding_rect_.width() / 2, 30 - title_->Height()/2);
}

void MonsterSwap::BackgroundCanvas(QPainter *painter) {
  painter->setPen(Qt::NoPen);
  QRadialGradient gradient(bounding_rect_.width() - bounding_rect_.width() / 4,
                           bounding_rect_.height()/2, bounding_rect_.height());
  gradient.setColorAt(0, QColor(213, 251, 254));
  gradient.setColorAt(1, QColor(255, 255, 255));
  painter->setBrush(gradient);
  painter->drawRect(0, 0, bounding_rect_.width(), bounding_rect_.height());
  painter->setBrush(QColor(165, 21, 45));
  painter->drawRect(0, 0, bounding_rect_.width(), 60);
  painter->setBrush(QColor(220, 220, 220));
  painter->drawRect(0, 60, bounding_rect_.width(), 60);

  qreal padding = bounding_rect_.width() / 3;
  auto rect = QRectF(0, 0, bounding_rect_.width(), bounding_rect_.height());
  auto painter_path = Shapes::DrawDiamond(rect, padding);
  painter_path.translate(-(bounding_rect_.width() / 2), 0);
  painter->setBrush(QColor(196, 40, 53));
  painter->drawPath(painter_path);
  painter_path.translate(-200, 0);
  painter->setBrush(QColor(227, 49, 61));
  painter->drawPath(painter_path);
}

void MonsterSwap::OnEnter() {
  Scene::OnEnter();
  LoadMonster();
  monster_list_->RegisterEvents();
}

void MonsterSwap::OnExit() { monster_list_->UnRegisterEvents(); }
