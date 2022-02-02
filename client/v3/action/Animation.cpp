// Copyright 2021 CatchChallenger
#include "Animation.hpp"

#include <QPainter>
#include <iostream>

#include "../core/Node.hpp"
#include "../core/Sprite.hpp"
#include "Action.hpp"

Animation* Animation::Create(std::vector<QPixmap> frames, int delay,
                             unsigned int loop) {
  Animation* instance = new Animation();

  instance->frames_ = frames;
  instance->delay_ = delay;
  instance->loop_ = loop;

  return instance;
}

Animation::Animation() : Action() { done_ = true; }

Animation::~Animation() { Action::~Action(); }

void Animation::Start() {
  if (frames_.empty()) {
    return;
  }
  ellapsed_ = 0;
  index_ = 0;
  done_ = false;
}

void Animation::Stop() { done_ = true; }

void Animation::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;
  ellapsed_ += ellapsed;

  if (ellapsed_ < delay_) return;

  ellapsed_ -= delay_;

  (static_cast<Sprite*>(node_))->SetPixmap(frames_.at(index_), false);
  index_++;
  if (frames_.size() == index_) {
    OnFinish();
  }
}

bool Animation::IsDone() { return done_; }

void Animation::SetFrames(std::vector<QPixmap> frames) { frames_ = frames; }
