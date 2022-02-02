// Copyright 2021 CatchChallenger
#include "Input.hpp"

#include <QPainter>
#include <iostream>

#include "../core/EventManager.hpp"
#include "../core/Node.hpp"

using UI::Input;

Input::Input(Node *parent) : Node(parent) {
  proxy_ = new QGraphicsProxyWidget(this, Qt::Widget);
  proxy_->setAutoFillBackground(false);
  proxy_->setAttribute(Qt::WA_NoSystemBackground);
  line_edit_ = new QLineEdit();
  line_edit_->setAutoFillBackground(false);
  line_edit_->setAttribute(Qt::WA_NoSystemBackground);
  QPalette palette;
  palette.setColor(QPalette::Base, Qt::transparent);
  palette.setColor(QPalette::Text, QColor(25, 11, 1));
  palette.setColor(QPalette::Window, Qt::transparent);
  palette.setCurrentColorGroup(QPalette::Active);
  palette.setColor(QPalette::Window, Qt::transparent);
  line_edit_->setPalette(palette);
  proxy_->setWidget(line_edit_);
  on_text_change_ = nullptr;
  label_ = Label::Create();
  background_ = Sprite::Create(":/CC/images/interface/input.png");
  inner_padding_ = QPointF(14, 11);
  focus_ = false;
  pressed_ = false;

  QObject::connect(line_edit_, &QLineEdit::returnPressed, std::bind(&Input::OnReturnPressed, this));
}

Input::~Input() {
  UnRegisterEvents();
  on_text_change_ = nullptr;
  delete label_;
  delete background_;
}

Input *Input::Create(Node *parent) {
  Input *instance = new (std::nothrow) Input(parent);
  return instance;
}

void Input::MousePressEvent(const QPointF &point, bool &press_validated) {
  focus_ = false;
  if (press_validated) return;
  if (pressed_) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
    pressed_ = true;
  }
}

void Input::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) {
    pressed_ = false;
    focus_ = false;
    return;
  }
  if (!pressed_) return;
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  pressed_ = false;
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      prev_validated = true;
      focus_ = true;
#ifdef __ANDROID_API__
      VirtualInput::GetInstance()->Show(text_, [&](std::string text) {
        focus_ = false;
        SetValue(QString::fromStdString(text));
        if (on_text_change_) {
          on_text_change_(text_.toStdString());
        }
      });
#endif
    }
  }
}

void Input::MouseMoveEvent(const QPointF &point) { (void)point; }

void Input::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  if (!focus_) return;
  bool should_notify = false;

  if (event->key() > 31 && event->key() < 127) {
    text_ = text_ + event->text();
    should_notify = true;
  } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    text_.clear();
    should_notify = true;
  } else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace) {
    if (text_.isEmpty()) {
      return;
    }
    text_.remove(text_.size() - 1, 1);
    should_notify = true;
  }
  if (should_notify) {
    label_->SetText(text_);
    ReDraw();
    if (on_text_change_) {
      on_text_change_(text_.toStdString());
    }
  }
}

void Input::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Input::SetFocus(bool focus) { focus_ = focus; }

bool Input::HasFocus() { return focus_; }

void Input::Draw(QPainter *painter) {
  background_->Render(painter);
  label_->SetPos(inner_padding_.x(), Height() / 2 - label_->Height() / 2);
  label_->Render(painter);
}

void Input::SetSize(qreal width, qreal height) {
  if (width == Width() && height == Height()) return;
  Node::SetSize(width, height);
}

QString Input::Value() const { return line_edit_->text(); }

void Input::SetValue(const QString &value) {
  line_edit_->setText(value);
  ReDraw();
}

void Input::Clear() { 
  line_edit_->clear();
}

void Input::SetPlaceholder(const QString &value) {
  line_edit_->setPlaceholderText(value);
}

void Input::SetFont(const QFont &font) {
  label_->SetFont(font);
  ReDraw();
}

void Input::RegisterEvents() {
}

void Input::UnRegisterEvents() {
}

void Input::SetMode(InputMode mode) { 
  mode_ = mode;
  if (mode_ == InputMode::kPassword) {
    line_edit_->setEchoMode(QLineEdit::Password);
  } else {
    line_edit_->setEchoMode(QLineEdit::Normal);
  }
}

void Input::SetMaxLength(int size) {
  line_edit_->setMaxLength(size);
}

void Input::SetOnTextChange(std::function<void(std::string)> callback) {
  on_text_change_ = callback;
}

void Input::OnResize() {
  if (Width() <= 0 || Height() <= 0) return;

  inner_padding_ = background_->Strech(14, 11, 10, Width(), Height());
  qreal inner_width = Width() - inner_padding_.x() * 2;
  qreal inner_height = Height() - inner_padding_.y() * 2;

  proxy_->setPos(inner_padding_.x(), inner_padding_.y());
  proxy_->setMaximumSize(inner_width, inner_height);
  proxy_->setMinimumSize(inner_width, inner_height);
  //line_edit_->setFixedSize(inner_width, 10);
}

void Input::OnReturnPressed() {
  if (on_text_change_) {
    on_text_change_(text_.toStdString());
  }
}
