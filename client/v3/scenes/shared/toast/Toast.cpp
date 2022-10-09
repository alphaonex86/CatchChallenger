// Copyright 2021 CatchChallenger
#include "Toast.hpp"

#include <iostream>

#include "../../../Constants.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"

using Scenes::Toast;
using UI::BlueLabel;
using UI::Column;
using UI::Row;

Toast::Toast(Node *parent) : Node(parent) {
  background_ = Sprite::Create(":/CC/images/interface/tip.png", this);
  content_ = Column::Create(this);
  content_->SetX(7);
  SetSize(background_->Width(), background_->Height());
}

Toast::~Toast() {
  delete background_;
  background_ = nullptr;
  delete content_;
  content_ = nullptr;
}

Toast *Toast::Create(Node *parent) { return new (std::nothrow) Toast(parent); }

void Toast::SetContent(const std::vector<std::string> &content) {
  if (prev_rendered_ == content.at(0)) return;

  prev_rendered_ = content.at(0);

  int space = 5;
  content_->RemoveAllChildrens();
  for (auto it = begin(content); it != end(content); it++) {
    QString str = QString::fromStdString((*it));
    if (str.contains("<img ")) {
      auto parts = str.split("<img src=\"");
      str = parts.last().split("\"").first();
      auto item = Sprite::Create();
      item->SetPixmap(Utils::B64Png2Pixmap(str.split("base64,").last()));
      auto row = Row::Create();
      row->AddChild(item);
      auto label = BlueLabel::Create();
      label->SetPixelSize(Constants::TextSmallSize());
      label->SetText(Utils::RemoveHTMLEntities(parts.first()));
      row->AddChild(label);
      content_->AddChild(row);
    } else {
      auto item = BlueLabel::Create();
      item->SetPixelSize(Constants::TextSmallSize());
      item->SetText(Utils::RemoveHTMLEntities(str));
      content_->AddChild(item);
    }
  }
  background_->Strech(7, 7, 7, content_->Width() + space + 7,
                      content_->Height() + space);
  auto scene = Utils::GetCurrentScene(this);
  SetX(scene->Width() / 2 - background_->Width() / 2);
}

void Toast::Draw(QPainter *painter) {}

void Toast::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (press_validated) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void Toast::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      SetVisible(false);
    }
  }
}

void Toast::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Toast::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}
