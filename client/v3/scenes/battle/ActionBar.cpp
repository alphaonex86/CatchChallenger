// Copyright 2021 CatchChallenger
#include "ActionBar.hpp"

#include <iostream>

using Scenes::ActionBar;

ActionBar::ActionBar(Node* parent): Node(parent) {
  fight_ = UI::TextRoundedButton::Create(this);
  fight_->SetText(QObject::tr("Fight"));
  fight_->SetIcon("f", QColor(190, 13, 41));

  monster_ = UI::TextRoundedButton::Create(this);
  monster_->SetText(QObject::tr("Monsters"));
  monster_->SetIcon("s", QColor(111, 215, 128));

  bag_ = UI::TextRoundedButton::Create(this);
  bag_->SetText(QObject::tr("Bag"));
  bag_->SetIcon("a", QColor(235, 183, 113));

  run_ = UI::TextRoundedButton::Create(this);
  run_->SetText(QObject::tr("Run"));
  run_->SetIcon("d", QColor(217, 141, 233));

  SetSize(300, 200);
}

ActionBar::~ActionBar() {

}

ActionBar* ActionBar::Create(Node* parent) {
  return new (std::nothrow) ActionBar(parent);
}

void ActionBar::Draw(QPainter *painter) {
  qreal pos_x = bounding_rect_.width() - fight_->Width();
  fight_->SetPos(pos_x, 0);
  monster_->SetPos(pos_x, 50);
  bag_->SetPos(pos_x, 100);
  run_->SetPos(pos_x, 150);
  unsigned int index = 0;

  if (skills_.empty()) return;
  pos_x = bounding_rect_.width() - skills_.at(0)->Width();
  qreal height = skills_.at(0)->Height();
  while (index < skills_.size()) {
    skills_.at(index)->SetPos(pos_x, height * index);
    index++;
  }
}

void ActionBar::RegisterEvents() {
  fight_->RegisterEvents();
  monster_->RegisterEvents();
  bag_->RegisterEvents();
  run_->RegisterEvents();
  for (auto item : skills_) {
    item->RegisterEvents();
  }
}

void ActionBar::UnRegisterEvents() {
  fight_->UnRegisterEvents();
  monster_->UnRegisterEvents();
  bag_->UnRegisterEvents();
  run_->UnRegisterEvents();
  for (auto item : skills_) {
    item->UnRegisterEvents();
  }
}

void ActionBar::OnResize() {}

void ActionBar::SetMonster(CatchChallenger::PlayerMonster *monster) {
  unsigned int index = 0;
  while (index < monster->skills.size()) {
    auto skill = SkillButton::Create(this);
    skill->SetSkill(monster->skills.at(index));
    skills_.push_back(skill);
    index++;
  }
  for (auto item : skills_) {
    item->RegisterEvents();
  }
  ReDraw();
}
