// Copyright 2021 CatchChallenger
#include "StatusCard.hpp"

#include <QPainter>
#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../entities/Shapes.hpp"

using Scenes::StatusCard;

StatusCard::StatusCard(Node* parent) : Node(parent) {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");

  monster_id_ = 0;
  is_enemy_ = false;

  SetSize(500, 100);
  padding_ = bounding_rect_.height() / 3;
  hp_bar_ = UI::SlimProgressbar::Create(this);
  xp_bar_ = UI::SlimProgressbar::Create(this);
  xp_bar_->SetForegroundColor(QColor(69, 193, 253));
}

StatusCard::~StatusCard() { delete font_; }

StatusCard* StatusCard::Create(Node* parent) {
  return new (std::nothrow) StatusCard(parent);
}

void StatusCard::Draw(QPainter* painter) {
  painter->setBrush(Qt::white);
  painter->setPen(Qt::NoPen);
  painter->setRenderHint(QPainter::Antialiasing);

  auto base_path = Shapes::DrawDiamond(bounding_rect_, padding_);
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
    if (!is_enemy_) {
      painter->drawText(padding_ + 20, bounding_rect_.height() - 10,
                        QStringLiteral("%1/%2").arg(hp_).arg(hp_max_));
    }

    hp_bar_->SetPos(padding_ + 20, 40);
    hp_bar_->SetSize(bounding_rect_.width() - padding_ * 2 - 40, 15);
    xp_bar_->SetSize(hp_bar_->Width() / 2, 10);
    xp_bar_->SetPos(hp_bar_->Right() - xp_bar_->Width(),
                    hp_bar_->Bottom() + 10);
  }
}

void StatusCard::RegisterEvents() {}

void StatusCard::UnRegisterEvents() {}

void StatusCard::OnResize() {}

void StatusCard::SetMonster(CatchChallenger::PlayerMonster* monster) {
  monster_id_ = monster->monster;
  level_ = monster->level;
  hp_ = monster->hp;
  is_enemy_ = false;

  auto monster_extra =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
          monster_id_);
  auto monster_info =
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
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

  xp_bar_->SetMinimum(0);

  xp_bar_->SetVisible(true);
  xp_bar_->SetMinimum(0);
  xp_bar_->SetMaximum(monster_info.level_to_xp.at(monster->level - 1));
  xp_bar_->SetValue(monster->remaining_xp);

  ReDraw();
}

void StatusCard::SetMonster(CatchChallenger::PublicPlayerMonster* monster) {
  monster_id_ = monster->monster;
  level_ = monster->level;
  hp_ = monster->hp;
  is_enemy_ = true;

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

  xp_bar_->SetVisible(false);

  ReDraw();
}

qreal StatusCard::Padding() const { return padding_; }

uint32_t StatusCard::HP() const { return hp_; }

QString StatusCard::Name() const { return name_; }

uint32_t StatusCard::XP() const { return exp_; }

uint32_t StatusCard::XPMax() const { return exp_max_; }

void StatusCard::UpdateXP(int32_t xp) {
  xp_bar_->IncrementValue(xp, true);
}

void StatusCard::UpdateHP(int32_t hp) {
  hp_bar_->IncrementValue(hp, true);
}
