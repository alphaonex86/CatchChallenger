// Copyright 2021 CatchChallenger
#include "SlimProgressbar.hpp"

#include <QPainter>
#include <cmath>
#include <iostream>

#include "../action/Tick.hpp"
#include "../core/AssetsLoader.hpp"
#include "../core/Node.hpp"

using UI::SlimProgressbar;

SlimProgressbar::SlimProgressbar(Node *parent)
    : Node(parent), on_increment_done_(nullptr) {
  bounding_rect_ = QRectF(0.0, 0.0, 200.0, 40.0);
  value_ = 0;
  internal_value_ = 0;
  min_ = 0;
  max_ = 0;

  background_color_ = QColor(99, 99, 101);
  foreground_color_ = QColor(0, 221, 6);

  tick_ = Tick::Create(100, std::bind(&SlimProgressbar::OnTick, this));
}

SlimProgressbar::~SlimProgressbar() {
  if (tick_ != nullptr) {
    delete tick_;
    tick_ = nullptr;
  }
}

SlimProgressbar *SlimProgressbar::Create(Node *parent) {
  SlimProgressbar *instance = new (std::nothrow) SlimProgressbar(parent);
  return instance;
}

void SlimProgressbar::Draw(QPainter *painter) {
  if (bounding_rect_.isEmpty()) return;

  painter->setPen(Qt::NoPen);
  painter->setBrush(background_color_);
  painter->drawRect(bounding_rect_);
  painter->setBrush(foreground_color_);
  painter->drawRect(0, 0, bounding_rect_.width() * internal_value_,
                    bounding_rect_.height());
}

void SlimProgressbar::SetMaximum(const int &value) {
  if (this->max_ == value) return;
  this->max_ = value;
  ReDraw();
}

void SlimProgressbar::SetMinimum(const int &value) {
  if (min_ == value) return;
  min_ = value;
  ReDraw();
}

void SlimProgressbar::SetValue(const int &value) {
  if (value_ == value) return;
  if (value > Maximum()) {
    value_ = Maximum();
  } else {
    value_ = value;
  }
  internal_value_ = (float)value / Maximum();
  ReDraw();
}

void SlimProgressbar::IncrementValue(const int &delta, const bool &animate) {
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
  ReDraw();
}

bool SlimProgressbar::OnTick() {
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

int SlimProgressbar::Maximum() { return max_; }

int SlimProgressbar::Minimum() { return min_; }

int SlimProgressbar::Value() { return value_; }

void SlimProgressbar::SetSize(int width, int height) {
  if (bounding_rect_.width() == width && bounding_rect_.height() == height)
    return;
  Node::SetSize(width, height);

  ReDraw();
}

void SlimProgressbar::SetOnIncrementDone(const std::function<void()> &callback) {
  on_increment_done_ = callback;
}

void SlimProgressbar::SetForegroundColor(const QColor &color) {
  foreground_color_ = color;
  ReDraw();
}
