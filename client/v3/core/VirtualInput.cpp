// Copyright 2021 CatchChallenger
#include "VirtualInput.hpp"

#include <QPainter>
#include <QStyle>
#include <iostream>

#include "SceneManager.hpp"

VirtualInput *VirtualInput::instance_ = nullptr;

VirtualInput::VirtualInput(QGraphicsItem *parent)
    : QGraphicsProxyWidget(parent, Qt::Widget) {
  on_text_change_ = nullptr;
  visible_ = false;
  control_ = new CustomTextEdit();
  setWidget(control_);
  auto manager = SceneManager::GetInstance();
  int height = manager->height();
  int width = manager->width();

  setMaximumSize(width, height);
  setMinimumSize(width, height);
  control_->setFixedSize(width, height);
  auto font = control_->font();
  font.setPixelSize(36);
  control_->setFont(font);
  setPos(0, 0);

  QObject::connect(control_, &CustomTextEdit::OnReturnPressed,
                   std::bind(&VirtualInput::OnTextComplete, this));
}

VirtualInput::~VirtualInput() {
  on_text_change_ = nullptr;
  delete control_;
  control_ = nullptr;
}

VirtualInput *VirtualInput::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new (std::nothrow) VirtualInput(nullptr);
  }
  return instance_;
}

QString VirtualInput::Value() const { return control_->toPlainText(); }

void VirtualInput::SetValue(const QString &value) {
  control_->setPlainText(value);
}

void VirtualInput::Clear() { control_->clear(); }

void VirtualInput::SetMode(InputMode mode) {
  switch (mode) {
    case InputMode::kPassword:
      // control_->setEchoMode(QLineEdit::Password);
      break;
  }
}

void VirtualInput::SetMaxLength(int size) {}

void VirtualInput::SetOnTextChange(std::function<void(std::string)> callback) {
  on_text_change_ = callback;
}

void VirtualInput::Show(const QString &value,
                        std::function<void(std::string)> callback) {
  SetOnTextChange(callback);
  setParentItem(SceneManager::GetInstance()->GetRenderer());
  control_->setPlainText(value);
  control_->show();
  control_->setFocus();
  visible_ = true;
}

void VirtualInput::Hide() {
  control_->hide();
  setParentItem(nullptr);
  visible_ = false;
}

bool VirtualInput::IsVisible() { return visible_; }

void VirtualInput::OnTextComplete() {
  auto text = control_->toPlainText();
  if (on_text_change_) {
    on_text_change_(text.toStdString());
  }
  Hide();
}

void CustomTextEdit::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    emit OnReturnPressed();
    return;
  }
  QPlainTextEdit::keyPressEvent(event);
}
