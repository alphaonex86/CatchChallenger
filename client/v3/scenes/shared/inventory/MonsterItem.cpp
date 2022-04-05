// Copyright 2021 <CatchChallenger>
#include "MonsterItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../ui/Label.hpp"

using CatchChallenger::PlayerMonster;
using Scenes::MonsterItem;

MonsterItem::MonsterItem(CatchChallenger::PlayerMonster monster)
    : UI::RoundedButton() {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");

  monster_ = monster;
  icon_ = Sprite::Create(this);
  icon_->SetTransformationMode(Qt::FastTransformation);
  hp_bar_ = UI::SlimProgressbar::Create(this);
  connection_manager_ = ConnectionManager::GetInstance();
  client_ = connection_manager_->client;

  icon_->SetSize(60, 60);
  SetSize(200, 80);
  LoadMonster();
}

MonsterItem::~MonsterItem() { delete icon_; }

MonsterItem *MonsterItem::Create(CatchChallenger::PlayerMonster monster) {
  return new (std::nothrow) MonsterItem(monster);
}

void MonsterItem::DrawContent(QPainter *painter, QColor color,
                              QRectF boundary) {
  auto rect = bounding_rect_;
  icon_->SetPos(10, rect.height() / 2 - icon_->Height() / 2);

  auto inner_width = bounding_rect_.width() - icon_->Width() - 60;
  auto padding = icon_->Right() - boundary.x();

  hp_bar_->SetSize(inner_width, 10);

  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(color);
  font_->setPixelSize(32);
  painter->setFont(*font_);
  painter->drawText(padding, 30, name_);

  hp_bar_->SetPos(icon_->Right(), rect.height() / 2 - hp_bar_->Height() / 2);

  painter->setFont(*font_);
  painter->drawText(boundary.width() - 70, bounding_rect_.height() - 20,
                    QStringLiteral("Lv. %1").arg(level_));
  painter->drawText(padding, bounding_rect_.height() - 20,
                    QStringLiteral("%1/%2").arg(hp_).arg(hp_max_));
}

void MonsterItem::LoadMonster() {
  const uint16_t id = monster_.monster;
  const DatapackClientLoader::MonsterExtra monster_extra =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(id);
  if (QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(id) !=
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend()) {
    level_ = monster_.level;
    QPixmap p =
        QtDatapackClientLoader::datapackLoader->getMonsterExtra(id).thumb;
    auto stat = client_->getStat(
        CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(id),
        level_);

    icon_->SetPixmap(p);
    name_ = QString::fromStdString(monster_extra.name);
    hp_ = monster_.hp;
    hp_max_ = stat.hp;

    hp_bar_->SetMinimum(0);
    hp_bar_->SetMaximum(hp_max_);
    hp_bar_->SetValue(hp_);
  } else {
    icon_->SetPixmap(
        QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
    name_ = QStringLiteral("id: %1").arg(id);
  }
}

void MonsterItem::OnResize() {
  auto square = bounding_rect_.height() - 20;
  icon_->SetSize(square, square);
  ReDraw();
}
