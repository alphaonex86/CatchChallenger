// Copyright 2021 <CatchChallenger>
#include "Battle.hpp"

#include <iostream>

using Scenes::Battle;

void Battle::OnScreenResize() {
  int padding = 100;

  action_return_->SetSize(UI::Button::kRectSmall);
  action_next_->SetSize(UI::Button::kRectSmall);
  action_attack_->SetSize(UI::Button::kRectSmall);
  action_bag_->SetSize(UI::Button::kRectSmall);
  action_monster_->SetSize(UI::Button::kRectSmall);
  action_escape_->SetSize(UI::Button::kRectSmall);

  action_background_->Strech(46, BoundingRect().width(),
                             BoundingRect().height() * 0.3);
  action_background_->SetPos(
      0, BoundingRect().height() - action_background_->Height());
  player_skills_->SetPos(20, 20);
  player_skills_->SetSize(action_background_->Width() * 0.5,
                          action_background_->Height() * 0.85);

  QPixmap t = labelFightBackgroundPix.scaledToWidth(BoundingRect().width());
  labelFightBackground->SetPixmap(t);
  t = labelFightForegroundPix.scaledToWidth(BoundingRect().width());
  labelFightForeground->SetPixmap(t);

  player_background_->Strech(28, 400, 170);
  player_background_->SetPos(
      BoundingRect().width() - player_background_->Width() - padding,
      BoundingRect().height() - player_background_->Height() - 10 -
          action_background_->Height());
  player_name_->SetPos(20, 20);
  player_lvl_->SetPos(player_background_->Width() - 100, 20);
  player_hp_bar_->SetPos(20, 60);
  player_hp_bar_->SetSize(player_background_->Width() - 40, 44);
  player_exp_bar_->SetPos(20, 110);
  player_exp_bar_->SetSize(player_background_->Width() - 40, 44);

  enemy_background_->Strech(28, 400, 120);
  enemy_background_->SetPos(padding, padding);
  enemy_name_->SetPos(20, 20);
  enemy_lvl_->SetPos(enemy_background_->Width() - 100, 20);
  enemy_hp_bar_->SetPos(20, 60);
  enemy_hp_bar_->SetSize(enemy_background_->Width() - 40, 44);

  int space = 30;
  uint frameWidth = action_background_->Width();
  uint frameHeight = action_background_->Height();
  status_label_->SetPos(space, frameHeight / 2 - 24 / 2);
  action_next_->SetPos(frameWidth - space - action_next_->Width(),
                       frameHeight / 2 - action_next_->Height() / 2);
  action_attack_->SetPos(frameWidth / 2 - space / 2 - action_attack_->Width(),
                         frameHeight /2 - (action_attack_->Height() * 2 + 5) / 2);
  action_bag_->SetPos(action_attack_->X() + action_attack_->Width() + 5,
                      action_attack_->Y());
  action_monster_->SetPos(action_attack_->X(),
                          action_attack_->Y() + action_attack_->Height() + 5);
  action_escape_->SetPos(action_bag_->X(),
                         action_attack_->Y() + action_attack_->Height() + 5);
  action_return_->SetPos(
      action_background_->Width() - space - action_return_->Width(),
      action_background_->Height() - space - action_return_->Height());
}

void Battle::OnScreenSD() {}

void Battle::OnScreenHD() {}

void Battle::OnScreenHDR() {}
