// Copyright 2021 CatchChallenger
#include "SkillButton.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
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
  painter->drawText(boundary.width() - 65, 28, count_);

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
  const CatchChallenger::Skill &skill_data =
      CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(
          skill.skill);
  effectiveness_ = QString("Effective");
  count_ = QStringLiteral("%1/%2")
               .arg(skill.endurance)
               .arg(skill_data.level.at(skill.level - 1).endurance);

  QColor color(168, 168, 120, 255);
  if (skill_data.type != 255) {
    if (QtDatapackClientLoader::datapackLoader->get_typeExtra().find(
            skill_data.type) !=
        QtDatapackClientLoader::datapackLoader->get_typeExtra().cend()) {
      if (!QtDatapackClientLoader::datapackLoader->get_typeExtra()
               .at(skill_data.type)
               .name.empty()) {
        auto cc_color = QtDatapackClientLoader::datapackLoader->get_typeExtra()
                            .at(skill_data.type)
                            .color;
        color = QColor(cc_color.r * (cc_color.r < 0 ? -1 : 1),
                       cc_color.g * (cc_color.g < 0 ? -1 : 1),
                       cc_color.b * (cc_color.b < 0 ? -1 : 1), 255);
      }
    }
  }
  SetStart(color, color);

  ReDrawContent();
  ReDraw();
}

void SkillButton::SetRescueSkill() {
  skill_.skill = 0;

  name_ = QString::fromStdString(
      QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
          .at(0)
          .name);
  const CatchChallenger::Skill &skill_data =
      CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(
          0);
  effectiveness_ = QString();
  count_ = "-/-";

  QColor color(168, 168, 120, 255);
  SetStart(color, color);

  ReDrawContent();
  ReDraw();
}
