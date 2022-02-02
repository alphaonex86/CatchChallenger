// Copyright 2021 CatchChallenger
#include "Sequence.hpp"

#include <cstdarg>
#include <iostream>
#include <vector>

#include "../core/Node.hpp"
#include "Action.hpp"

using std::vector;

Sequence* Sequence::Create(const vector<Action*>& actions) {
  Sequence* instance = new (std::nothrow) Sequence();

  instance->actions_ = actions;

  return instance;
}

Sequence* Sequence::Create(Action* action, ...) {
  Sequence* instance = new (std::nothrow) Sequence();
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

Sequence::Sequence() : Action() {
  done_ = true;
  current_ = 0;
}

Sequence::~Sequence() {
  Action::~Action();

  for (auto action : actions_) {
    delete action;
  }
}

Action* Sequence::ActionAt(size_t index) { return actions_.at(index); }

void Sequence::Start() {
  done_ = false;
  current_index_ = 0;
  current_ = actions_.at(current_index_);
  current_->SetTarget(node_);
  current_->Start();
}

void Sequence::Stop() {
  done_ = true;
  current_index_ = 0;
  if (current_) current_->Stop();
  current_ = nullptr;
}

void Sequence::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr || current_ == nullptr) return;

  current_->Step(ellapsed);

  if (current_ && current_->IsDone()) {
    current_index_++;
    if (current_index_ < actions_.size()) {
      current_ = actions_.at(current_index_);
      current_->SetTarget(node_);
      current_->Start();
    } else {
      OnFinish();
    }
  }
}

bool Sequence::IsDone() { return done_; }
