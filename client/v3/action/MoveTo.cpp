// Copyright 2021 CatchChallenger
#include "MoveTo.hpp"

#include <cmath>
#include <iostream>

#include "../Options.hpp"
#include "../core/Node.hpp"
#include "Action.hpp"

MoveTo* MoveTo::Create(int milliseconds, qreal x, qreal y) {
  MoveTo* instance = new MoveTo();

  instance->milliseconds_ = milliseconds;
  instance->end_x_ = x;
  instance->end_y_ = y;

  return instance;
}

MoveTo* MoveTo::Create(int milliseconds, QPointF point) {
  MoveTo* instance = new MoveTo();

  instance->milliseconds_ = milliseconds;
  instance->end_x_ = point.x();
  instance->end_y_ = point.y();

  return instance;
}

MoveTo::MoveTo() : Action() {
  delta_x_ = 0;
  delta_y_ = 0;
  done_ = true;
  timeout_ = 0;
  ellapsed_ = 0;
}

MoveTo::~MoveTo() { Action::~Action(); }

void MoveTo::Start() {
  ellapsed_ = 0;
  delta_x_ = 0;
  delta_y_ = 0;
  finish_x_ = false;
  finish_y_ = false;
  CalculateDelta();
  done_ = false;
}

void MoveTo::Stop() {
  done_ = true;
  delta_x_ = 0;
  delta_y_ = 0;
  ellapsed_ = 0;
}

void MoveTo::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  ellapsed_ += ellapsed;
  if (ellapsed_ >= timeout_) {
    ellapsed_ = 0;

    if (!finish_x_) {
      if (std::abs(node_->X() - end_x_) <= std::abs(delta_x_)) {
        finish_x_ = true;
      }
    }

    if (!finish_y_) {
      if (std::abs(node_->Y() - end_y_) <= std::abs(delta_y_)) {
        finish_y_ = true;
      }
    }

    if (finish_x_ && finish_y_) {
      node_->SetPos(end_x_, end_y_);
      OnFinish();
    } else {
      if (!finish_x_) node_->SetX(node_->X() + delta_x_);
      if (!finish_y_) node_->SetY(node_->Y() + delta_y_);
    }
  }
}

bool MoveTo::IsDone() { return done_; }

void MoveTo::CalculateDelta() {
  auto fps = Options::GetInstance()->getFPS();
  qreal frames = (milliseconds_ / 1000) * fps;

  timeout_ = 1000 / fps;
  if (end_x_ == node_->X()) {
    finish_x_ = true;
  } else {
    delta_x_ = (end_x_ - node_->X()) / frames;
  }

  if (end_y_ == node_->Y()) {
    finish_y_ = true;
  } else {
    delta_y_ = (end_y_ - node_->Y()) / frames;
  }
}