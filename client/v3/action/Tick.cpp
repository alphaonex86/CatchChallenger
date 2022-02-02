// Copyright 2021 CatchChallenger
#include "Tick.hpp"

#include <iostream>

#include "../core/Node.hpp"
#include "Action.hpp"

Tick* Tick::Create(int milliseconds, const std::function<bool()>& callback) {
  Tick* instance = new Tick();

  instance->timeout_ = milliseconds;
  instance->on_tick_ = callback;

  return instance;
}

Tick::Tick() : Action() { done_ = true; }

Tick::~Tick() { Action::~Action(); }

void Tick::Start() {
  ellapsed_ = 0;
  done_ = false;
}

void Tick::Stop() {
  done_ = true;
  ellapsed_ = 0;
}

void Tick::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  ellapsed_ += ellapsed;
  if (ellapsed_ >= timeout_) {
    OnFinish();
  }
}

bool Tick::IsDone() { return done_; }

void Tick::OnFinish() {
  auto should_continue = on_tick_();
  if (should_continue) {
    Start();
    return;
  }
  Action::OnFinish();
}
