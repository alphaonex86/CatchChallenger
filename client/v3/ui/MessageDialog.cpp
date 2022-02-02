// Copyright 2021 <CatchChallenger>
#include "MessageDialog.hpp"

#include <iostream>

#include "../core/SceneManager.hpp"

using UI::MessageDialog;

MessageDialog::MessageDialog() {
  SetDialogSize(600, 300);

  message_ = UI::Label::Create(this);
  message_->SetAlignment(Qt::AlignCenter);

  newLanguage();
}

MessageDialog::~MessageDialog() {
  delete message_;
  message_ = nullptr;
}

MessageDialog *MessageDialog::Create() {
  return new (std::nothrow) MessageDialog();
}

void MessageDialog::Draw(QPainter *painter) { UI::Dialog::Draw(painter); }

void MessageDialog::SetMessage(const QString &text) { message_->SetText(text); }

void MessageDialog::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto content = ContentBoundary();
  message_->SetPos(content.x(), content.y());
  message_->SetWidth(content.width());
}

void MessageDialog::newLanguage() { SetTitle(tr("Alert")); }

void MessageDialog::Show(const QString &message) { Show(QString(), message); }

void MessageDialog::Show(const QString &title, const QString &message) {
  if (!title.isEmpty()) {
    SetTitle(title);
  }

  message_->SetText(message);
  SceneManager::GetInstance()->ShowOverlay(this);
}
