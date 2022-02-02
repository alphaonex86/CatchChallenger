// Copyright 2021 CatchChallenger
#include "Blink.hpp"

#include "../core/Node.hpp"
#include "Action.hpp"

Blink* Blink::Create(int milliseconds, unsigned int times) {
  Blink* instance = new Blink();

  instance->timeout_ = milliseconds;
  instance->times_ = times;

  return instance;
}

Blink::Blink() : Action() { done_ = true; }

Blink::~Blink() { Action::~Action(); }

void Blink::Start() {
  ellapsed_ = 0;
  index_ = 0;
  done_ = false;
}

void Blink::Stop() {
  done_ = true;
  ellapsed_ = 0;
}

void Blink::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  ellapsed_ += ellapsed;
  if (ellapsed_ >= timeout_) {
    ellapsed_ -= timeout_;
    index_++;

    node_->SetVisible(!node_->IsVisible());

    if (index_ >= times_) {
      OnFinish();
    }
  }
}

bool Blink::IsDone() { return done_; }
