// Copyright 2021 CatchChallenger
#include "RepeatForever.hpp"

#include <iostream>

#include "../core/Node.hpp"
#include "Action.hpp"

RepeatForever* RepeatForever::Create(Action* action) {
  RepeatForever* instance = new (std::nothrow) RepeatForever();

  instance->action_ = action;

  return instance;
}

RepeatForever::RepeatForever() : Action() { done_ = true; }

RepeatForever::~RepeatForever() {
  Action::~Action();
  if (action_) delete action_;
}

void RepeatForever::Start() {
  done_ = false;
  action_->SetTarget(node_);
  action_->Start();
}

void RepeatForever::Stop() {
  done_ = true;
  if (action_) action_->Stop();
}

void RepeatForever::Step(unsigned int ellapsed) {
  if (done_ || node_ == nullptr) return;

  action_->Step(ellapsed);

  if (action_->IsDone()) {
    action_->Start();
  }
}

bool RepeatForever::IsDone() { return done_; }
