// Copyright 2021 <CatchChallenger>
#include "InputDialog.hpp"

#include <QIntValidator>
#include <iostream>

#include "../Constants.hpp"
#include "../Globals.hpp"
#include "../core/SceneManager.hpp"

using UI::InputDialog;

InputDialog::InputDialog() {
  SetDialogSize(Constants::DialogSmallSize());

  message_ = UI::Label::Create(this);
  message_->SetAlignment(Qt::AlignCenter);

  input_ = UI::Input::Create(this);
  type_ = 0;

  accept_ = UI::Button::Create(":/CC/images/interface/validate.png", this);
  accept_->SetOnClick(std::bind(&InputDialog::OnAcceptClick, this));
  AddActionButton(accept_);

  newLanguage();
}

InputDialog::~InputDialog() {
  delete message_;
  message_ = nullptr;
}

InputDialog *InputDialog::Create() { return new (std::nothrow) InputDialog(); }

void InputDialog::Draw(QPainter *painter) { UI::Dialog::Draw(painter); }

void InputDialog::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto content = ContentBoundary();
  message_->SetPos(content.x(), content.y());
  message_->SetWidth(content.width());

  input_->SetPos(content.x(), content.y() + message_->Height());
  input_->SetSize(UI::Input::kSmall);
  input_->SetWidth(content.width());
}

void InputDialog::newLanguage() {}

void InputDialog::OnAcceptClick() {
  if (type_ == 1) {
    int value = input_->Value().toInt();
    if (value < min_ || value > max_) {
      Globals::GetAlertDialog()->Show(
          tr("The value must be between %1 and %2").arg(min_).arg(max_));
      return;
    }
  }
  callback_(input_->Value());
  Close();
}

void InputDialog::ShowInputNumber(const QString &message,
                                  std::function<void(QString)> callback,
                                  int min, int max,
                                  const QString &default_value) {
  ShowInputNumber(QString(), message, callback, min, max, default_value);
}

void InputDialog::ShowInputNumber(const QString &title, const QString &message,
                                  std::function<void(QString)> callback,
                                  int min, int max,
                                  const QString &default_value) {
  if (!title.isEmpty()) {
    SetTitle(title);
  }
  type_ = 1;
  min_ = min;
  max_ = max;

  input_->SetValidator(new QIntValidator(min, max, input_->GetProxy()));
  input_->SetValue(default_value);
  callback_ = callback;
  message_->SetText(message);
  SceneManager::GetInstance()->ShowOverlay(this);
}

void InputDialog::ShowInputText(const QString &message,
                                std::function<void(QString)> callback,
                                const QString &default_value) {
  ShowInputText(QString(), message, callback, default_value);
}

void InputDialog::ShowInputText(const QString &title, const QString &message,
                                std::function<void(QString)> callback,
                                const QString &default_value) {
  if (!title.isEmpty()) {
    SetTitle(title);
  }
  type_ = 2;
  input_->SetValidator(nullptr);
  input_->SetValue(default_value);
  callback_ = callback;
  message_->SetText(message);
  SceneManager::GetInstance()->ShowOverlay(this);
}
