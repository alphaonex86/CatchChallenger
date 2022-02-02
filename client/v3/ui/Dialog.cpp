// Copyright 2021 CatchChallenger
#include "Dialog.hpp"

#include <QPainter>
#include <iostream>

#include "../core/EventManager.hpp"
#include "../entities/Utils.hpp"

using std::placeholders::_1;
using UI::Dialog;

Dialog::Dialog(bool show_close) {
  on_close_ = nullptr;
  backdrop_ = Backdrop::Create(QColor(0, 0, 0, 120), this);

  background_ = Sprite::Create(":/CC/images/interface/message.png", this);
  title_background_ = Sprite::Create(this);

  x_ = -1;
  y_ = -1;
  dialog_size_ = QSize(900, 400);
  title_background_->SetPixmap(":/CC/images/interface/label.png");
  title_background_->SetVisible(false);

  QLinearGradient gradient1(0, 0, 0, 100);
  gradient1.setColorAt(0.25, QColor(230, 153, 0));
  gradient1.setColorAt(0.75, QColor(255, 255, 255));
  QLinearGradient gradient2(0, 0, 0, 100);
  gradient2.setColorAt(0, QColor(64, 28, 2));
  gradient2.setColorAt(1, QColor(64, 28, 2));
  title_ = UI::Label::Create(gradient2, gradient1, this);
  title_->SetAlignment(Qt::AlignCenter);
  title_->SetVisible(false);

  quit_ = Button::Create(":/CC/images/interface/closedialog.png");
  show_close_ = show_close;
  if (show_close_) {
    AddChild(quit_);
  }
  quit_->SetOnClick(std::bind(&Dialog::Close, this));

  actions_ = UI::Row::Create(this);
  actions_->SetItemSpacing(25);
  actions_->SetMode(Row::kDisplayNoneVisible);
}

Dialog::~Dialog() {
  delete background_;
  background_ = nullptr;

  delete title_;
  title_ = nullptr;

  delete title_background_;
  title_background_ = nullptr;

  quit_->UnRegisterEvents();
  delete quit_;
  quit_ = nullptr;

  delete actions_;
  actions_ = nullptr;

  delete backdrop_;
  backdrop_ = nullptr;
}

void Dialog::OnScreenResize() {
  bool forcedCenter = true;
  int idealW = dialog_size_.width();
  int idealH = dialog_size_.height();
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    idealW /= 2;
    idealH /= 2;
  }
  if (idealW > bounding_rect_.width()) idealW = bounding_rect_.width();
  int top = 36;
  int bottom = 94 / 2;
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    top /= 2;
    bottom /= 2;
  }
  if ((idealH + top + bottom) > bounding_rect_.height())
    idealH = bounding_rect_.height() - (top + bottom);

  if (x_ < 0 || y_ < 0 || forcedCenter) {
    x_ = bounding_rect_.width() / 2 - idealW / 2;
    y_ = bounding_rect_.height() / 2 - idealH / 2;
  } else {
    if ((x_ + idealW) > bounding_rect_.width())
      x_ = bounding_rect_.width() - idealW;
    if ((y_ + idealH + bottom) > bounding_rect_.height())
      y_ = bounding_rect_.height() - (idealH + bottom);
    if (y_ < top) y_ = top;
  }

  background_->SetPos(x_, y_);
  background_->Strech(42, 42, 30, idealW, idealH);

  backdrop_->SetSize(bounding_rect_.width(), bounding_rect_.height());

  unsigned int space = 30;
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    title_background_->SetPos(
        x_ + (idealW - title_background_->Width() / 2) / 2, y_ - 36 / 2);
    title_->SetPixelSize(30 / 2);
    space = 10;
  } else {
    title_background_->SetPos(x_ + (idealW - title_background_->Width()) / 2,
                              y_ - 36);
    title_->SetPixelSize(30);
  }

  title_->SetPos(x_, y_ - 30);
  title_->SetWidth(idealW);

  content_boundary_ = QRectF(x_ + 30, y_ + 60, idealW - 60, idealH - 120);
  content_plain_boundary_ = QRectF(x_ + 20, y_ + 20, idealW - 40, idealH - 40);

  quit_->SetSize(50, 56);
  quit_->SetPos((background_->X() + background_->Width()) - quit_->Width() / 2,
                background_->Y() - quit_->Height() / 2);
}

void Dialog::Draw(QPainter *painter) {
  //painter->fillRect(0, 0, bounding_rect_.width(), bounding_rect_.height(),
                    //QColor(0, 0, 0, 120));
  //background_->Paint(painter, nullptr, nullptr);

  //if (title_->Text().size() > 0) {
    //title_background_->Paint(painter, nullptr, nullptr);
    //title_->Paint(painter, nullptr, nullptr);
  //}
}

void Dialog::OnScreenSD() {}

void Dialog::OnScreenHD() {}

void Dialog::OnScreenHDR() {}

void Dialog::SetOnClose(std::function<void()> callback) {
  on_close_ = callback;
}

void Dialog::OnEnter() {
  Scene::OnEnter();
  EventManager::GetInstance()->Lock();

  if (show_close_) {
    quit_->RegisterEvents();
  }
  auto items = actions_->Children();
  for (auto it = begin(items); it != end(items); it++) {
    (*it)->RegisterEvents();
  }

  RecalculateActionButtons();
}

void Dialog::OnExit() {
  auto items = actions_->Children();
  for (auto it = begin(items); it != end(items); it++) {
    (*it)->UnRegisterEvents();
  }

  if (show_close_) {
    quit_->UnRegisterEvents();
  }
  EventManager::GetInstance()->UnLock();
}

void Dialog::AddActionButton(Button *action) {
  actions_->AddChild(action);

  if (Utils::IsActiveInScene(this)) {
    action->RegisterEvents();
  }
  RecalculateActionButtons();
}

void Dialog::InsertActionButton(Button *action, int pos) {
  actions_->InsertChild(action, pos);

  if (Utils::IsActiveInScene(this)) {
    action->RegisterEvents();
  }
  RecalculateActionButtons();
}

void Dialog::RemoveActionButton(Button *action) {
  action->UnRegisterEvents();
  actions_->RemoveChild(action);

  RecalculateActionButtons();
}

void Dialog::RecalculateActionButtons() {
  if (actions_->Children().empty()) return;

  unsigned int width = 83;
  unsigned int height = 94;
  auto content = bounding_rect_;
  if (content.width() < 600 || content.height() < 480) {
    width /= 2;
    height /= 2;
  }

  auto items = actions_->Children();
  for (auto it = begin(items); it != end(items); it++) {
    (*it)->SetSize(width, height);
  }
  actions_->ReCalculateSize();

  auto content_b = ContentBoundary();
  actions_->SetPos(content.width() / 2 - actions_->Width() / 2,
                   content_b.y() + content_b.height());
}

void Dialog::SetTitle(const QString &text) {
  title_->SetText(text.toUpper());

  bool isVisible = !text.isEmpty();
  title_->SetVisible(isVisible);
  title_background_->SetVisible(isVisible);
}

QString Dialog::Title() const { return title_->Text(); }

void Dialog::Close() {
  if (on_close_) {
    on_close_();
  }
  auto parent = Parent();
  if (parent != nullptr) {
    parent->RemoveChild(this);
  }
}

QRectF Dialog::ContentBoundary() const { return content_boundary_; }

QRectF Dialog::ContentPlainBoundary() const { return content_plain_boundary_; }

void Dialog::SetDialogSize(int w, int h) {
  dialog_size_.setHeight(h);
  dialog_size_.setWidth(w);
}

void Dialog::ShowClose(bool show) {
  show_close_ = show;
  quit_->SetVisible(show);
}
