// Copyright 2021 CatchChallenger
#include "Combo.hpp"

#include <math.h>

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

#include "../core/AssetsLoader.hpp"
#include "../core/EventManager.hpp"

using UI::Combo;

Combo::Combo(QString pix, Node *parent) : Node(parent) {
  background_ = Sprite::Create();
  auto pixmap = AssetsLoader::GetInstance()->GetImage(pix);
  background_->SetPixmap(pixmap->copy(0, 0, 223, 92));

  label_ = Label::Create(this);
  label_->SetAlignment(Qt::AlignCenter);
  last_z_ = ZValue();

  is_menu_open_ = false;
}

Combo *Combo::Create(QString pix, Node *parent) {
  Combo *instance = new (std::nothrow) Combo(pix, parent);
  return instance;
}

Combo *Combo::Create(Node *parent) {
  Combo *instance =
      new (std::nothrow) Combo(":/CC/images/interface/button.png", parent);
  return instance;
}

Combo::~Combo() { 
  UnRegisterEvents();
  delete label_; 
  delete background_;
}

void Combo::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;

  qreal init_height = 92;
  auto inner = inner_bounding_rect_;
  qreal scale = inner.height() / init_height;

  background_->Draw(painter);

  if (is_menu_open_) {
    auto label = UI::Label::Create();
    label->SetFont(label_->GetFont());
    label->SetAlignment(Qt::AlignCenter);
    label->SetWidth(inner.width());
    label->SetX(0);

    auto check = AssetsLoader::GetInstance()
                     ->GetImage(":/CC/images/interface/check.png")
                     ->scaledToHeight(scale * 46, Qt::SmoothTransformation);
    auto items = items_;
    qreal offset_y = inner.height();
    uint8_t index = 0;
    for (auto item : items) {
      label->SetY(offset_y);
      label->SetText(item);
      label->Render(painter);
      if (index == current_index_) {
        painter->drawPixmap(
            5, offset_y + (label_->Height() / 2) - (check.height() / 2), check);
      }
      offset_y += label_->Height();
      index++;
    }

    delete label;
  }
  auto arrow = AssetsLoader::GetInstance()
                   ->GetImage(":/CC/images/interface/arrow-down.png")
                   ->scaledToHeight(scale * 27, Qt::SmoothTransformation);
  painter->drawPixmap(inner.width() - arrow.width() - 10,
                      inner.height() / 2 - (arrow.height() / 2) - 3, arrow);
}

void Combo::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) {
    if (is_menu_open_) {
      CloseMenu();
    }
    return;
  }
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(p)) {
    press_validated = true;
  } else {
    if (is_menu_open_) {
      CloseMenu();
    }
  }
}

void Combo::MouseReleaseEvent(const QPointF &p, bool &prev_validated) {
  if (prev_validated) {
    // SetPressed(false);
    return;
  }
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  // SetPressed(false);
  if (!prev_validated && IsVisible()) {
    if (t.contains(p)) {
      if (!is_menu_open_) {
        OpenMenu();
      } else {
        auto y = p.y() - (t.y() + inner_bounding_rect_.height());
        auto index = y / label_->Height();
        if (index >= 0) {
          OnClick(index);
          CloseMenu();
        }
      }
    }
  }
}

void Combo::MouseMoveEvent(const QPointF &point) { (void)point; }

void Combo::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Combo::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Combo::SetCurrentIndex(uint8_t index) {
  current_index_ = index;
  label_->SetText(items_.at(index));
}

uint8_t Combo::CurrentIndex() { return current_index_; }

void Combo::AddItem(const QString &item) {
  items_.push_back(item);
  if (items_.size() == 1) {
    current_index_ = 0;
    label_->SetText(item);
    label_->SetY(BoundingRect().height() / 2 - label_->Height() / 2);
  }
}

void Combo::OnClick(uint8_t selected) {
  current_index_ = selected;
  label_->SetText(items_.at(current_index_));

  if (on_select_change_) {
    on_select_change_(current_index_);
  }
  ReDraw();
}

void Combo::SetOnSelectChange(std::function<void(uint8_t)> callback) {
  on_select_change_ = callback;
}

void Combo::SetSize(QSizeF& size) {
  Combo::SetSize(size.width(), size.height());
}

void Combo::SetWidth(qreal width) {
  Combo::SetSize(width, Height());
}

void Combo::SetSize(qreal width, qreal height) {
  auto bounding = BoundingRect();
  if (width == bounding.width() && height == bounding.height()) return;
  if (!is_menu_open_) {
    inner_bounding_rect_ = QRectF(0, 0, width, height);
  }
  Node::SetSize(width, height);
}

void Combo::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Combo::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

size_t Combo::Count() const { return items_.size(); }

void Combo::Clear() { items_.clear(); }

int Combo::ItemData(uint8_t index, int role) { return item_data_[index][role]; }

void Combo::SetItemData(uint8_t index, int role, int value) {
  std::unordered_map<int, int> item;
  item[role] = value;
  item_data_[index] = item;
}

void Combo::OnResize() {
  label_->SetWidth(inner_bounding_rect_.width());
  label_->SetY(inner_bounding_rect_.height() / 2 - label_->Height() / 2);
    background_->Strech(25, 25, 25, BoundingRect().width(), BoundingRect().height());
}

void Combo::OpenMenu() {
  is_menu_open_ = true;
  last_z_ = ZValue();
  SetZValue(10);

  SetHeight(Height() + (items_.size()) * label_->Height() + Height() * 0.2);
  ReDraw();
}

void Combo::CloseMenu() {
  is_menu_open_ = false;
  SetZValue(last_z_);
  SetHeight(inner_bounding_rect_.height());
  ReDraw();
}
