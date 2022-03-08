// Copyright 2021 <CatchChallenger>
#include "../../scenes/battle/Battle.hpp"

using Scenes::Battle;

void Battle::OnScreenResize() {
  auto bounding = BoundingRect();
  action_bar_->SetPos(bounding.width() - action_bar_->Width(),
                         bounding.height() - action_bar_->Height());
  player_status_->SetPos(
      -player_status_->Padding(),
      BoundingRect().height() - player_status_->Height() - 20);
}

void Battle::OnScreenSD() {}

void Battle::OnScreenHD() {
}

void Battle::OnScreenHDR() {
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

  //player_skills_->SetPos(20, 20);
  //player_skills_->SetSize(500, 100);

  //int space = 30;
  //uint frameWidth = action_background_->Width();
  //uint frameHeight = action_background_->Height();
}
