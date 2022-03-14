// Copyright 2021 <CatchChallenger>
#include "../../scenes/battle/Battle.hpp"

using Scenes::Battle;

void Battle::OnScreenResize() {
  auto bounding = BoundingRect();
  action_bar_->SetPos(bounding.width() - action_bar_->Width(),
                         bounding.height() - action_bar_->Height());
  player_status_->SetPos(
      -player_status_->Padding(),
      bounding.height() - player_status_->Height() - 20);
  enemy_status_->SetPos(
      bounding.width() - enemy_status_->Width() + enemy_status_->Padding(),
      50);
}

void Battle::OnScreenSD() {}

void Battle::OnScreenHD() {
}

void Battle::OnScreenHDR() {
  QPixmap t = labelFightBackgroundPix.scaledToWidth(BoundingRect().width());
  labelFightBackground->SetPixmap(t);
  t = labelFightForegroundPix.scaledToWidth(BoundingRect().width());
  labelFightForeground->SetPixmap(t);
  //player_skills_->SetPos(20, 20);
  //player_skills_->SetSize(500, 100);

  //int space = 30;
  //uint frameWidth = action_background_->Width();
  //uint frameHeight = action_background_->Height();
}
