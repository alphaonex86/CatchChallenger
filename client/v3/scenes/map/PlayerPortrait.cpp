// Copyright 2021 CatchChallenger
#include "PlayerPortrait.hpp"

#include <QPainter>
#include <iostream>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../core/EventManager.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Label.hpp"
#include "../../Constants.hpp"

using Scenes::PlayerPortrait;
using std::placeholders::_1;

PlayerPortrait::PlayerPortrait(Node *parent) : Node(parent) {
  SetSize(450, 150);
}

PlayerPortrait *PlayerPortrait::Create(Node *parent) {
  return new (std::nothrow) PlayerPortrait(parent);
}

void PlayerPortrait::SetInformation(
    const CatchChallenger::Player_private_and_public_informations
        &informations) {
  image_ = QString::fromStdString(
      QtDatapackClientLoader::GetInstance()->getFrontSkinPath(
          informations.public_informations.skinId));
  name_ = QString::fromStdString(informations.public_informations.pseudo);
  std::cout << "info.public_informations.pseudo: "
            << informations.public_informations.pseudo << std::endl;
  cash_ = QString::number(informations.cash);

  ReDraw();
}

void PlayerPortrait::Draw(QPainter *painter) {
  if (name_.isEmpty()) return;
  QString background =
      QString::fromStdString(":/CC/images/interface/playerui.png");

  Sprite *sprite = Sprite::Create(background);
  sprite->Draw(painter);

  QPixmap pPixm = image_;
  if (true)
    pPixm = pPixm.scaledToHeight(pPixm.height() * 2, Qt::FastTransformation);
  painter->drawPixmap(0, -5, pPixm);

  auto text = UI::Label::Create();
  text->SetText(name_);
  text->SetPos(sprite->X() + 175, sprite->Y() + 18);
  text->Render(painter);

  text->SetPixelSize(Constants::TextSmallSize());
  text->SetText(cash_);
  text->SetPos(sprite->X() + 230, sprite->Y() + 65);
  text->Render(painter);

  delete text;
  delete sprite;
}

void PlayerPortrait::MousePressEvent(const QPointF &point,
                                     bool &press_validated) {
  if (press_validated) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void PlayerPortrait::MouseReleaseEvent(const QPointF &point,
                                       bool &prev_validated) {
  if (prev_validated) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void PlayerPortrait::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
  connect(PlayerInfo::GetInstance(), &PlayerInfo::OnUpdateInfo, this,
          &PlayerPortrait::OnUpdateInfo);
}

void PlayerPortrait::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
  disconnect(PlayerInfo::GetInstance(), &PlayerInfo::OnUpdateInfo, this,
             &PlayerPortrait::OnUpdateInfo);
}

void PlayerPortrait::OnUpdateInfo(PlayerInfo *info) {
  SetInformation(info->GetInformationRO());
}
