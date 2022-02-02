// Copyright 2021 CatchChallenger
#include "Progressbar.hpp"

#include <QPainter>
#include <cmath>
#include <iostream>

#include "../action/Tick.hpp"
#include "../core/AssetsLoader.hpp"
#include "../core/Node.hpp"

using UI::Progressbar;

Progressbar::Progressbar(Node *parent)
    : Node(parent), on_increment_done_(nullptr) {
  bounding_rect_ = QRectF(0.0, 0.0, 200.0, 82.0);
  value_ = 0;
  internal_value_ = 0;
  min_ = 0;
  max_ = 0;

  text_ = UI::Label::Create(QColor(255, 255, 255), QColor(5, 40, 0), this);
  text_->SetText(QString::fromStdString("0"));
  text_->SetPixelSize(11);
  text_->SetAlignment(Qt::AlignCenter);
  text_->SetWidth(bounding_rect_.width());
  text_->SetY(bounding_rect_.height() / 2 - text_->Height() / 2);

  tick_ = Tick::Create(50, std::bind(&Progressbar::OnTick, this));
}

Progressbar::~Progressbar() {
  delete text_;
  if (tick_ != nullptr) {
    delete tick_;
    tick_ = nullptr;
  }
}

Progressbar *Progressbar::Create(Node *parent) {
  Progressbar *instance = new (std::nothrow) Progressbar(parent);
  return instance;
}

void Progressbar::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;
  QImage image(bounding_rect_.width(), bounding_rect_.height(),
               QImage::Format_ARGB32);
  if (background_left_.isNull() ||
      background_left_.height() != bounding_rect_.height()) {
    auto asset_loader = AssetsLoader::GetInstance();
    QPixmap background =
        *asset_loader->GetImage(":/CC/images/interface/Pbarbackground.png");
    if (background.isNull()) abort();
    QPixmap bar =
        *asset_loader->GetImage(":/CC/images/interface/Pbarforeground.png");
    if (bar.isNull()) abort();
    if (bounding_rect_.height() == background.height()) {
      background_left_ = background.copy(0, 0, 41, 82);
      background_middle_ = background.copy(41, 0, 824, 82);
      background_right_ = background.copy(865, 0, 41, 82);
      bar_left_ = bar.copy(0, 0, 26, 82);
      bar_middle_ = bar.copy(26, 0, 620, 82);
      bar_right_ = bar.copy(646, 0, 26, 82);
    } else {
      background_left_ = background.copy(0, 0, 41, 82)
                             .scaledToHeight(bounding_rect_.height(),
                                             Qt::SmoothTransformation);
      background_middle_ = background.copy(41, 0, 824, 82)
                               .scaledToHeight(bounding_rect_.height(),
                                               Qt::SmoothTransformation);
      background_right_ = background.copy(865, 0, 41, 82)
                              .scaledToHeight(bounding_rect_.height(),
                                              Qt::SmoothTransformation);
      bar_left_ = bar.copy(0, 0, 26, 82)
                      .scaledToHeight(bounding_rect_.height(),
                                      Qt::SmoothTransformation);
      bar_middle_ = bar.copy(26, 0, 620, 82)
                        .scaledToHeight(bounding_rect_.height(),
                                        Qt::SmoothTransformation);
      bar_right_ = bar.copy(646, 0, 26, 82)
                       .scaledToHeight(bounding_rect_.height(),
                                       Qt::SmoothTransformation);
    }
  }
  painter->drawPixmap(0, 0, background_left_.width(), background_left_.height(),
                      background_left_);
  painter->drawPixmap(background_left_.width(), 0,
                      bounding_rect_.width() - background_left_.width() -
                          background_right_.width(),
                      background_left_.height(), background_middle_);
  painter->drawPixmap(bounding_rect_.width() - background_right_.width(), 0,
                      background_right_.width(), background_right_.height(),
                      background_right_);

  int startX = 18 * bounding_rect_.height() / 82;
  int size = bounding_rect_.width() - startX - startX;
  if (Maximum() == 0) return;
  float inpixel = internal_value_ * size / Maximum();
  if (inpixel < (bar_left_.width() + bar_right_.width())) {
    if (inpixel > 0) {
      const int tempW = inpixel / 2;
      if (tempW > 0) {
        QPixmap bar_left_C = bar_left_.copy(0, 0, tempW, bar_left_.height());
        painter->drawPixmap(startX, 0, bar_left_C.width(), bar_left_C.height(),
                            bar_left_C);
        const float pixelremaining = inpixel - bar_left_C.width();
        if (pixelremaining > 0) {
          QPixmap bar_right_C =
              bar_right_.copy(bar_right_.width() - pixelremaining, 0,
                              pixelremaining, bar_right_.height());
          painter->drawPixmap(startX + bar_left_C.width(), 0,
                              bar_right_C.width(), bar_right_C.height(),
                              bar_right_C);
        }
      }
    }
  } else {
    painter->drawPixmap(startX, 0, bar_left_.width(), bar_left_.height(),
                        bar_left_);
    const unsigned int pixelremaining =
        inpixel - (bar_left_.width() + bar_right_.width());
    if (pixelremaining > 0)
      painter->drawPixmap(startX + bar_left_.width(), 0, pixelremaining,
                          bar_middle_.height(), bar_middle_);
    painter->drawPixmap(startX + bar_left_.width() + pixelremaining, 0,
                        bar_right_.width(), bar_right_.height(), bar_right_);
  }
}

void Progressbar::SetMaximum(const int &value) {
  if (this->max_ == value) return;
  this->max_ = value;
  ReDraw();
}

void Progressbar::SetMinimum(const int &value) {
  if (min_ == value) return;
  min_ = value;
  ReDraw();
}

void Progressbar::SetValue(const int &value) {
  if (value_ == value) return;
  if (value > Maximum()) {
    value_ = Maximum();
  } else {
    value_ = value;
  }
  text_->SetText(QString::number(value_));
  internal_value_ = value;
  ReDraw();
}

void Progressbar::IncrementValue(const int &delta, const bool &animate) {
  int tmp = delta;
  if (value_ + tmp > Maximum()) {
    tmp = Maximum() - value_;
  }
  value_ = value_ + tmp;
  if (animate) {
    increment_times_ = 0;
    delta_value_ = tmp / 10.0f;

    RunAction(tick_);
  }
  text_->SetText(QString::number(value_));
  ReDraw();
}

bool Progressbar::OnTick() {
  bool should_continue;
  if (increment_times_ < 9) {
    internal_value_ += delta_value_;
    increment_times_++;
    should_continue = true;
  } else {
    internal_value_ = value_;
    should_continue = false;
    if (on_increment_done_) {
      on_increment_done_();
    }
  }
  ReDraw();
  return should_continue;
}

int Progressbar::Maximum() { return max_; }

int Progressbar::Minimum() { return min_; }

int Progressbar::Value() { return value_; }

void Progressbar::SetSize(int width, int height) {
  if (bounding_rect_.width() == width && bounding_rect_.height() == height)
    return;
  Node::SetSize(width, height);
  text_->SetWidth(width);
  text_->SetY(height / 2 - text_->Height() / 2);

  ReDraw();
}

void Progressbar::SetOnIncrementDone(const std::function<void()> &callback) {
  on_increment_done_ = callback;
}
