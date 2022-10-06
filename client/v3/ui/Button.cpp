// Copyright 2021 CatchChallenger
#include "Button.hpp"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

#include "../core/AssetsLoader.hpp"
#include "../core/EventManager.hpp"
#include "../Constants.hpp"

using UI::Button;

Button::Button(QString pix, Node *parent) : Node(parent) {
  pressed_ = false;
  background_ = pix;
  outline_color_ = QColor(217, 145, 0);
  checkable_ = false;
  checked_ = false;

  bounding_rect_ = QRectF(0.0, 0.0, 223.0, 92.0);

  label_ = Label::Create();
  label_->SetPixelSize(35);
  label_->SetWidth(bounding_rect_.width());
  label_->SetAlignment(Qt::AlignCenter);

  Button::SetSize(kRectMedium);
}

Button *Button::Create(QString pix, Node *parent) {
  Button *instance = new (std::nothrow) Button(pix, parent);
  return instance;
}

void Button::SetSize(const ButtonSize& size) {
  QSizeF buttonSize;
  int textSize;

  switch (size) {
    case kRectLarge:
      buttonSize = Constants::ButtonLargeSize();
      textSize = Constants::TextLargeSize();
      break;
    case kRectMedium:
      buttonSize = Constants::ButtonMediumSize();
      textSize = Constants::TextLargeSize();
      break;
    case kRectSmall:
      buttonSize = Constants::ButtonSmallSize();
      textSize = Constants::TextMediumSize();
      break;
    case kRoundLarge:
      buttonSize = Constants::ButtonRoundLargeSize();
      textSize = Constants::TextLargeSize();
      break;
    case kRoundMedium:
      buttonSize = Constants::ButtonRoundMediumSize();
      textSize = Constants::TextLargeSize();
      break;
    case kRoundSmall:
      buttonSize = Constants::ButtonRoundSmallSize();
      textSize = Constants::TextMediumSize();
      break;
  }

  label_->SetPixelSize(textSize);
  Button::SetSize(buttonSize.width(), buttonSize.height());
}

void Button::SetSize(const QSizeF& size) {
  Button::SetSize(size.width(), size.height());
}

void Button::SetSize(qreal w, qreal h) {
  if (bounding_rect_.width() == w && bounding_rect_.height() == h) return;
  Node::SetSize(w, h);
  label_->SetWidth(bounding_rect_.width());
  ReDraw();
}

void Button::SetWidth(qreal w) {
  if (bounding_rect_.width() == w) return;
  Node::SetWidth(w);
  label_->SetWidth(bounding_rect_.width());
  ReDraw();
}

void Button::SetPos(qreal x, qreal y) {
  if (bounding_rect_.x() == x && bounding_rect_.y() == y) return;
  Node::SetPos(x, y);
}

Button::~Button() {
  UnRegisterEvents();
  delete label_;
}

void Button::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;
  QImage image(bounding_rect_.width(), bounding_rect_.height(),
               QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  QPainter paint;
  if (image.isNull()) abort();
  paint.begin(&image);
  QPixmap scaled_background;
  QPixmap temp(*AssetsLoader::GetInstance()->GetImage(background_));
  if (temp.isNull()) abort();
  if (!IsEnabled())
    scaled_background =
        temp.copy(0, temp.height() * 2 / 3, temp.width(), temp.height() / 3);
  else if (checked_)
    scaled_background =
        temp.copy(0, temp.height() / 3, temp.width(), temp.height() / 3);
  else
    scaled_background = temp.copy(0, 0, temp.width(), temp.height() / 3);
  scaled_background =
      scaled_background.scaled(bounding_rect_.width(), bounding_rect_.height(),
                               Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  paint.drawPixmap(0, 0, bounding_rect_.width(), bounding_rect_.height(),
                   scaled_background);

  if (!text_.isEmpty()) {
    if (IsEnabled())
      label_->SetBorderColor(outline_color_);
    else
      label_->SetBorderColor(QColor(125, 125, 125));

    label_->SetTextColor(Qt::white);
    label_->SetText(text_);
    int height = 0;
    int percent = 75;
    int h = bounding_rect_.height();
    if (checked_)
      height = (h * (percent + 20) / 100);
    else
      height = (h * percent / 100);
    label_->SetPos(0, height / 2 - label_->Height() / 2);
    label_->Render(&paint);
  }

  painter->drawPixmap(0, 0, bounding_rect_.width(), bounding_rect_.height(),
                      QPixmap::fromImage(image));
}

void Button::SetText(const QString &text) {
  if (text_ == text) return;
  text_ = text;
  ReDraw();
}

bool Button::SetPixelSize(uint8_t size) {
  label_->SetPixelSize(size);
  ReDraw();
  return true;
}

void Button::SetFont(const QFont &font) {
  label_->SetFont(font);
  ReDraw();
}

QFont Button::GetFont() const { return label_->GetFont(); }

void Button::SetOutlineColor(const QColor &color) { outline_color_ = color; }

void Button::SetPressed(const bool &pressed) {
  if (checkable_) {
    if (pressed_ == false && pressed == true && checked_ == false)
      checked_ = true;
    else if (pressed_ == false && pressed == true && checked_ == true)
      checked_ = false;
  } else {
    checked_ = pressed;
  }
  pressed_ = pressed;
  ReDraw();
}

void Button::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) return;
  if (pressed_) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(p)) {
    press_validated = true;
    SetPressed(true);
  }
}

void Button::MouseReleaseEvent(const QPointF &p, bool &prev_validated) {
  if (prev_validated) {
    SetPressed(false);
    return;
  }
  if (!pressed_) return;
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  SetPressed(false);
  if (!prev_validated && IsVisible()) {
    if (t.contains(p)) {
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void Button::MouseMoveEvent(const QPointF &point) { (void)point; }

void Button::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Button::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Button::SetEnabled(bool enabled) {
  if (enabled == IsEnabled()) return;
  Node::SetEnabled(enabled);
  ReDraw();
}

void Button::SetCheckable(bool checkable) {
  checkable_ = checkable;
  ReDraw();
}

bool Button::IsChecked() const { return checked_; }

void Button::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Button::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void Button::SetIcon(const QString &icon) {}

void Button::SetIcon(const QPixmap &icon) {}

void Button::SetIcon(const QIcon &icon) {}
