// Copyright 2022 CatchChallenger
#include "PlayerCounter.hpp"

#include "../../Constants.hpp"
#include "../../base/ConnectionManager.hpp"

using Scenes::PlayerCounter;

PlayerCounter::PlayerCounter(Node *parent) : Node(parent) {
  SetSize(Constants::ButtonSmallHeight() * 2, Constants::ButtonSmallHeight());

  playersCountBack =
      Sprite::Create(":/CC/images/interface/multicount.png", this);
  playersCount = UI::Label::Create(this);
}

PlayerCounter *PlayerCounter::Create(Node *parent) {
  return new (std::nothrow) PlayerCounter(parent);
}

PlayerCounter::~PlayerCounter() {
  delete playersCountBack;
  playersCountBack = nullptr;

  delete playersCount;
  playersCount = nullptr;
}

void PlayerCounter::Draw(QPainter *painter) {
  unsigned int space = Constants::ItemMediumSpacing();

  playersCountBack->SetPos(
      bounding_rect_.width() - space - playersCountBack->Width(), space);
  playersCount->SetPos(bounding_rect_.width() - space - 80, (Height() / 2 - playersCount->Height() / 2) - 16 );
}

void PlayerCounter::OnEnter() {}

void PlayerCounter::OnExit() {}

void PlayerCounter::UpdateData(const uint16_t &number,
                               const uint16_t &max_players) {
  (void)max_players;
  playersCount->SetText(QString::number(number));
}
