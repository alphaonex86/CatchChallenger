// Copyright 2021 CatchChallenger
#include "CallFunc.hpp"

#include <functional>

#include "../core/Node.hpp"
#include "Action.hpp"

CallFunc* CallFunc::Create(std::function<void()> callback) {
  CallFunc* instance = new (std::nothrow) CallFunc();

  instance->callback_ = callback;

  return instance;
}

CallFunc::CallFunc() : Action(), callback_(nullptr) { done_ = true; }

CallFunc::~CallFunc() { Action::~Action(); }

void CallFunc::Start() {
  done_ = false;
}

void CallFunc::Stop() {
  done_ = true;
}

void CallFunc::Step(unsigned int ellapsed) {
  (void) ellapsed;
  if (done_ || node_ == nullptr) return;

  callback_();
  OnFinish();
}

bool CallFunc::IsDone() { return done_; }
