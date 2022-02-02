// Copyright 2021 CatchChallenger
#include "Delay.hpp"

#include <iostream>

#include "../core/Node.hpp"
#include "Action.hpp"

Delay* Delay::Create(int milliseconds) {
  Delay* instance = new Delay();

  instance->timeout_ = milliseconds;

  return instance;
}

Delay::Delay() : Action() { done_ = true; }

Delay::~Delay() { Action::~Action(); }

void Delay::Start() {
  ellapsed_ = 0;
  done_ = false;
}

void Delay::Stop() {
  done_ = true;
  ellapsed_ = 0;
}

void Delay::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  ellapsed_ += ellapsed;

  if (ellapsed_ >= timeout_) {
    OnFinish();
  }
}

bool Delay::IsDone() { return done_; }
