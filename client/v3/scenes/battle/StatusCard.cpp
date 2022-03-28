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

  monster_id_ = 0;

  SetSize(500, 100);
  padding_ = bounding_rect_.height() / 3;
  hp_bar_ = UI::SlimProgressbar::Create(this);
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

  if (monster_id_ != 0) {
    painter->setPen(Qt::black);
    font_->setPixelSize(24);
    painter->setFont(*font_);
    painter->drawText(padding_ + 20, 28, name_);
    font_->setPixelSize(28);
    painter->setFont(*font_);
    painter->drawText(bounding_rect_.width() - padding_ - 70, 32, "Lv. ");
    font_->setPixelSize(32);
    painter->setFont(*font_);
    painter->drawText(bounding_rect_.width() - padding_ - 40, 32,
                      QString::number(level_));
    painter->drawText(padding_ + 20, bounding_rect_.height() - 10,
                      QStringLiteral("%1/%2").arg(hp_).arg(hp_max_));

    hp_bar_->SetPos(padding_ + 20, 40);
    hp_bar_->SetSize(bounding_rect_.width() - padding_ * 2 - 40, 15);
  }
}

void StatusCard::RegisterEvents() {}

void StatusCard::UnRegisterEvents() {}

void StatusCard::OnResize() {}

void StatusCard::SetMonster(CatchChallenger::PlayerMonster* monster) {
  monster_id_ = monster->monster;
  level_ = monster->level;
  hp_ = monster->hp;

  auto monster_extra =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
          monster_id_);
  auto stat = ConnectionManager::GetInstance()->client->getStat(
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
          monster_id_),
      level_);

  name_ = QString::fromStdString(monster_extra.name);
  gender_ = monster->gender;
  hp_max_ = stat.hp;

  hp_bar_->SetMinimum(0);
  hp_bar_->SetMaximum(hp_max_);
  hp_bar_->SetValue(hp_);

  ReDraw();
}

void StatusCard::SetMonster(CatchChallenger::PublicPlayerMonster* monster) {
  monster_id_ = monster->monster;
  level_ = monster->level;
  hp_ = monster->hp;

  auto monster_extra =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
          monster_id_);
  auto stat = ConnectionManager::GetInstance()->client->getStat(
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
          monster_id_),
      level_);

  name_ = QString::fromStdString(monster_extra.name);
  gender_ = monster->gender;
  hp_max_ = stat.hp;

  hp_bar_->SetMinimum(0);
  hp_bar_->SetMaximum(hp_max_);
  hp_bar_->SetValue(hp_);

  ReDraw();
}

qreal StatusCard::Padding() const { return padding_; }

uint32_t StatusCard::HP() const { return hp_; }

QString StatusCard::Name() const {
  return name_;
}

uint32_t StatusCard::XP() const { return exp_; }

uint32_t StatusCard::XPMax() const { return exp_max_; }
