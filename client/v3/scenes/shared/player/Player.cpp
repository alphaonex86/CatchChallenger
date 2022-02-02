// Copyright 2021 CatchChallenger
#include "Player.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../ui/LinkedDialog.hpp"

using Scenes::Player;

Player::Player() {
  preview_ = Sprite::Create(this);
  preview_->SetTransformationMode(Qt::FastTransformation);
  preview_->SetSize(160, 160);
  name_label = UI::Label::Create(this); name_value = UI::Label::Create(this);
  cash_label = UI::Label::Create(this);
  cash_value = UI::Label::Create(this);
  type_label = UI::Label::Create(this);
  type_value = UI::Label::Create(this);

  connexionManager = ConnectionManager::GetInstance();

  newLanguage();
}

Player::~Player() {}

Player *Player::Create() { return new (std::nothrow) Player(); }

void Player::OnResize() {
  auto bounding = BoundingRect();
  int idealW = BoundingRect().width();
  unsigned int space = 30;

  int tempY = space + 30;
  preview_->SetPos(10, bounding.height() / 2 - preview_->Height() / 2);
  qreal x_pos = preview_->X() + preview_->Width() + 20;

  name_label->SetPos(x_pos, tempY);
  name_value->SetPos(idealW / 2, tempY);
  tempY += name_label->Height() + space;
  cash_label->SetPos(x_pos, tempY);
  cash_value->SetPos(idealW / 2, tempY);
  tempY += cash_label->Height() + space;
  type_label->SetPos(x_pos, tempY);
  type_value->SetPos(idealW / 2, tempY);
}

void Player::newLanguage() {
  name_label->SetText(QObject::tr("Name: "));
  cash_label->SetText(QObject::tr("Cash: "));
  type_label->SetText(QObject::tr("Type: "));
}

void Player::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(QObject::tr("PLAYER"));
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();

  preview_->SetPixmap(QString::fromStdString(
        QtDatapackClientLoader::datapackLoader->getFrontSkinPath(

          playerInformations.public_informations.skinId)), false);
  name_value->SetText(playerInformations.public_informations.pseudo);
  cash_value->SetText(QString::number(playerInformations.cash));
  switch (playerInformations.public_informations.type) {
    default:
    case CatchChallenger::Player_type_normal:
      type_value->SetText(QObject::tr("Normal player"));
      break;
    case CatchChallenger::Player_type_premium:
      type_value->SetText(QObject::tr("Premium player"));
      break;
    case CatchChallenger::Player_type_dev:
      type_value->SetText(QObject::tr("Developer player"));
      break;
    case CatchChallenger::Player_type_gm:
      type_value->SetText(QObject::tr("Admin"));
      break;
  }
}

void Player::UnRegisterEvents() {}

void Player::Draw(QPainter *painter) { (void)painter; }
