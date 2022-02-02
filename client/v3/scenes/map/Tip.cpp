// Copyright 2021 CatchChallenger
#include "Tip.hpp"

#include <iostream>

#include "../../core/EventManager.hpp"
#include "../../entities/Utils.hpp"

using Scenes::Tip;
using UI::BlueLabel;

Tip::Tip(Node *parent) : Node(parent) {
  background_ = Sprite::Create(":/CC/images/interface/tip.png", this);
  label_ = BlueLabel::Create(this);
  label_->SetPixelSize(10);
  label_->SetX(7);
  SetSize(background_->Width(), background_->Height());
}

Tip::~Tip() {
  delete background_;
  background_ = nullptr;
  delete label_;
  label_ = nullptr;
}

Tip *Tip::Create(Node *parent) { return new (std::nothrow) Tip(parent); }

void Tip::SetText(const std::string &text) {
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] " << text
            << std::endl;
  if (text_ == text) return;

  text_ = text;

  int space = 5;
  label_->SetText(Utils::RemoveHTMLEntities(QString::fromStdString(text)));
  background_->Strech(7, 7, 7, label_->Width() + space + 7,
                      label_->Height() + space);
  auto scene = Utils::GetCurrentScene(this);
  SetX(scene->Width() / 2 - label_->Width() / 2);
}

void Tip::Draw(QPainter *painter) {}

void Tip::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (press_validated) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void Tip::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      SetVisible(false);
    }
  }
}

void Tip::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Tip::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}
