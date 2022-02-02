// Copyright 2021 CatchChallenger
#include "Spawn.hpp"

#include <cstdarg>
#include <iostream>
#include <vector>
#include <cstdint>

#include "../core/Node.hpp"
#include "Action.hpp"

using std::vector;

Spawn* Spawn::Create(const vector<Action*>& actions) {
  Spawn* instance = new (std::nothrow) Spawn();

  instance->actions_ = actions;

  return instance;
}

Spawn* Spawn::Create(Action* action, ...) {
  Spawn* instance = new (std::nothrow) Spawn();
  va_list params;
  va_start(params, action);
  Action* item;

  instance->actions_.push_back(action);
  while (action) {
    item = va_arg(params, Action*);
    if (!item) break;
    instance->actions_.push_back(item);
  }

  va_end(params);

  return instance;
}

Spawn::Spawn() : Action() {
  done_ = true;
  done_count_ = 0;
}

Spawn::~Spawn() {
  Action::~Action();

  for (auto action : actions_) {
    delete action;
  }
}

Action* Spawn::ActionAt(size_t index) { return actions_.at(index); }

void Spawn::Start() {
  done_ = false;
  done_count_ = 0;
  for (auto action : actions_) {
    action->SetTarget(node_);
    action->Start();
  }
}

void Spawn::Stop() {
  done_ = true;
  done_count_ = 0;
  for (auto action : actions_) {
    action->Stop();
  }
}

void Spawn::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  for (auto action : actions_) {
    if (action->IsDone()) continue;

    action->Step(ellapsed);
    if (action->IsDone()) done_count_++;
  }

  if (done_count_ >= actions_.size()) {
    OnFinish();
  }
}

bool Spawn::IsDone() { return done_; }
