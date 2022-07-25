// Copyright 2021 <CatchChallenger>
#include "Battle.hpp"

using Scenes::Battle;

void Battle::OnScreenResize() {}

void Battle::OnScreenSD() {}

void Battle::OnScreenHD() {
  action_next_->SetSize(148, 61);
  action_next_->SetPixelSize(23);

  action_attack_->SetSize(148, 61);
  action_attack_->SetPixelSize(23);
  action_bag_->SetSize(148, 61);
  action_bag_->SetPixelSize(23);
  action_monster_->SetSize(148, 61);
  action_monster_->SetPixelSize(23);
  action_escape_->SetSize(148, 61);
  action_escape_->SetPixelSize(23);

  action_return_->SetSize(148, 61);
  action_return_->SetPixelSize(23);
}

void Battle::OnScreenHDR() {
  action_next_->SetSize(100, 50);
  action_next_->SetPixelSize(20);

  action_attack_->SetSize(200, 70);
  action_attack_->SetPixelSize(20);
  action_bag_->SetSize(200, 70);
  action_bag_->SetPixelSize(20);
  action_monster_->SetSize(200, 70);
  action_monster_->SetPixelSize(20);
  action_escape_->SetSize(200, 70);
  action_escape_->SetPixelSize(20);

  action_return_->SetSize(223, 92);
  action_return_->SetPixelSize(35);

  QPixmap t = labelFightBackgroundPix.scaledToWidth(BoundingRect().width());
  labelFightBackground->SetPixmap(t);
  t = labelFightForegroundPix.scaledToWidth(BoundingRect().width());
  labelFightForeground->SetPixmap(t);

  player_background_->Strech(28, 400, 170);
  player_background_->SetPos(BoundingRect().width() * 0.63,
                             BoundingRect().height() * 0.6);
  player_name_->SetPos(20, 20);
  player_hp_bar_->SetPos(20, 60);
  player_hp_bar_->SetSize(player_background_->Width() - 40, 44);
  player_exp_bar_->SetPos(20, 110);
  player_exp_bar_->SetSize(player_background_->Width() - 40, 44);

  enemy_background_->Strech(28, 400, 120);
  enemy_background_->SetPos(100, 100);
  enemy_name_->SetPos(20, 20);
  enemy_hp_bar_->SetPos(20, 60);
  enemy_hp_bar_->SetSize(enemy_background_->Width() - 40, 44);

  action_background_->Strech(46, BoundingRect().width(), 160);
  action_background_->SetPos(0, BoundingRect().height() - 160);

  player_skills_->SetPos(20, 20);
  player_skills_->SetSize(500, 100);

  int space = 30;
  uint frameWidth = action_background_->Width();
  uint frameHeight = action_background_->Height();
  status_label_->SetPos(space, frameHeight / 2 - 24 / 2);
  action_next_->SetPos(frameWidth - space - action_next_->Width(),
                       frameHeight / 2 - action_next_->Height() / 2);
  action_attack_->SetPos(frameWidth / 2 - space / 2 - action_attack_->Width(),
                         10);
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
