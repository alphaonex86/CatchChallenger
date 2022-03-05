// Copyright 2021 CatchChallenger
#include "StatusCard.hpp"

#include <QPainter>
#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../base/ConnectionManager.hpp"

using Scenes::StatusCard;

StatusCard::StatusCard(Node* parent) : Node(parent) {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");

  monster_ = nullptr;

  SetSize(500, 100);
  padding_ = bounding_rect_.height() / 3;
}

StatusCard::~StatusCard() { delete font_; }

StatusCard* StatusCard::Create(Node* parent) {
  return new (std::nothrow) StatusCard(parent);
}

void StatusCard::Draw(QPainter* painter) {
  painter->setBrush(Qt::white);
  painter->setPen(Qt::NoPen);
  painter->setRenderHint(QPainter::Antialiasing);

  QPainterPath base_path;
  base_path.moveTo(padding_, 0);
  base_path.lineTo(bounding_rect_.width(), 0);
  base_path.lineTo(bounding_rect_.width() - padding_, bounding_rect_.height());
  base_path.lineTo(0, bounding_rect_.height());
  base_path.lineTo(padding_, 0);
  painter->drawPath(base_path);

  if (monster_ != nullptr) {
    painter->setPen(Qt::black);
    font_->setPixelSize(24);
    painter->setFont(*font_);
    painter->drawText(padding_ + 20, 28, name_);
    font_->setPixelSize(28);
    painter->setFont(*font_);
    painter->drawText(bounding_rect_.width() - padding_ - 70, 32, "Lv. ");
    font_->setPixelSize(32);
    painter->setFont(*font_);
    painter->drawText(bounding_rect_.width() - padding_ - 40, 32, level_);
    painter->drawText(padding_ + 20, bounding_rect_.height() - 10, hp_);
  }
}

void StatusCard::RegisterEvents() {}

void StatusCard::UnRegisterEvents() {}

void StatusCard::OnResize() {}

void StatusCard::SetMonster(CatchChallenger::PlayerMonster* monster) {
  monster_ = monster;

  UpdateData();

  ReDraw();
}

void StatusCard::UpdateData() {
  auto monster_extra =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
          monster_->monster);
  auto stat = ConnectionManager::GetInstance()->client->getStat(
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
          monster_->monster),
      monster_->level);

  name_ = QString::fromStdString(monster_extra.name);
  level_ = QString::number(monster_->level);
  hp_ = QStringLiteral("%1/%2").arg(monster_->hp).arg(stat.hp);
  gender_ = monster_->gender;
}

qreal StatusCard::Padding() const {
  return padding_;
}
