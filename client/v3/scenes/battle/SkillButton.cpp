// Copyright 2021 CatchChallenger
#include "SkillButton.hpp"

#include <QPainter>
#include <iostream>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

using Scenes::SkillButton;

SkillButton::SkillButton(Node *parent) : RoundedButton(parent) {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");

  name_ = QString();
  effectiveness_ = QString();
  count_ = QString();

  font_icon_ = new QFont();
  font_icon_->setFamily("icomoon");
  font_icon_->setPixelSize(56);

  SetEnd(0.3, Qt::black, Qt::black);
  SetBorder(3, Qt::black);

  SetSize(300, 50);
}

SkillButton::~SkillButton() {
  delete font_;
  delete font_icon_;
}

SkillButton *SkillButton::Create(Node *parent) {
  return new (std::nothrow) SkillButton(parent);
}

void SkillButton::DrawContent(QPainter *painter, QColor color,
                              QRectF boundary) {
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::black);
  font_->setPixelSize(16);
  painter->setFont(*font_);
  painter->drawText(0, 20, name_);
  font_->setPixelSize(12);
  painter->setFont(*font_);
  painter->drawText(0, 35, effectiveness_);

  painter->setPen(Qt::white);
  font_->setPixelSize(26);
  painter->setFont(*font_);
  painter->drawText(boundary.width() - 55, 28, count_);

  painter->setFont(*font_icon_);
  // painter->drawText(boundary.width() - 55, 50, icon_);
}

void SkillButton::SetSkill(
    const CatchChallenger::PlayerMonster::PlayerSkill &skill) {
  skill_ = skill;

  name_ = QString::fromStdString(
      QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
          .at(skill.skill)
          .name);
  effectiveness_ = QString("Effective");
  count_ = QStringLiteral("%1/0").arg(skill.endurance);
  SetStart(Qt::blue, Qt::blue);

  ReDrawContent();
  ReDraw();
}
