// Copyright 2021 CatchChallenger
#include "ActionBar.hpp"

#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"

using Scenes::ActionBar;

ActionBar::ActionBar(Node *parent) : Node(parent) {
  show_skills_ = false;
  on_action_click_ = nullptr;
  on_skill_click_ = nullptr;
  rescue_skill_ = nullptr;

  fight_ = UI::TextRoundedButton::Create(this);
  fight_->SetText(QObject::tr("Fight"));
  fight_->SetIcon("f", QColor(190, 13, 41));
  fight_->SetOnClick([this](Node *) { ShowSkills(true); });

  monster_ = UI::TextRoundedButton::Create(this);
  monster_->SetText(QObject::tr("Monsters"));
  monster_->SetIcon("s", QColor(111, 215, 128));
  monster_->SetOnClick([&](Node *) { on_action_click_(kActionType_Monster); });

  bag_ = UI::TextRoundedButton::Create(this);
  bag_->SetText(QObject::tr("Bag"));
  bag_->SetIcon("a", QColor(235, 183, 113));
  bag_->SetOnClick([&](Node *) { on_action_click_(kActionType_Bag); });

  run_ = UI::TextRoundedButton::Create(this);
  run_->SetText(QObject::tr("Run"));
  run_->SetIcon("d", QColor(217, 141, 233));
  run_->SetOnClick([&](Node *) { on_action_click_(kActionType_Run); });

  SetSize(300, 200);
}

ActionBar::~ActionBar() {}

ActionBar *ActionBar::Create(Node *parent) {
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

void ActionBar::OnResize() { ReDraw(); }

void ActionBar::SetMonster(CatchChallenger::PlayerMonster *monster) {
  for (auto item : skills_) {
    delete item;
  }
  skills_.clear();

  unsigned int index = 0;
  while (index < monster->skills.size()) {
    auto skill = SkillButton::Create(this);
    skill->SetSkill(monster->skills.at(index));
    skill->SetVisible(show_skills_);
    skills_.push_back(skill);
    index++;
  }
  if (CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().find(
          0) !=
      CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills()
          .cend()) {
    if (!CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills()
             .at(0)
             .level.empty()) {
      rescue_skill_ = SkillButton::Create(this);
      rescue_skill_->SetRescueSkill();
      rescue_skill_->SetVisible(false);
    }
  }
  for (auto item : skills_) {
    item->RegisterEvents();
  }
  ReDraw();
}

void ActionBar::CanRun(bool can_run) { run_->SetVisible(can_run); }

void ActionBar::ShowSkills(bool show) {
  show_skills_ = show;

  fight_->SetVisible(!show_skills_);
  monster_->SetVisible(!show_skills_);
  bag_->SetVisible(!show_skills_);
  run_->SetVisible(!show_skills_);

  for (auto item : skills_) {
    item->SetVisible(show_skills_);
  }
}

void ActionBar::SetOnActionClick(
    const std::function<void(const ActionType &)> &callback) {
  on_action_click_ = callback;
}

void ActionBar::SetOnSkillClick(
    const std::function<void(uint8_t, uint16_t, uint8_t)> &callback) {
  on_skill_click_ = callback;
}

void ActionBar::ShowRescueSkill(bool show) {
  if (rescue_skill_ == nullptr) return;
  rescue_skill_->SetVisible(show);
}
