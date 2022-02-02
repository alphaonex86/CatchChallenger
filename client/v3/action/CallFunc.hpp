// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_CALLFUNC_HPP_
#define CLIENT_QTOPENGL_ACTION_CALLFUNC_HPP_

#include <cstdint>
#include <functional>

#include "Action.hpp"

class CallFunc : public Action {
 public:
  ~CallFunc();

  static CallFunc *Create(std::function<void()> callback);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;
  std::function<void()> callback_;

  CallFunc();
};
#endif  // CLIENT_QTOPENGL_ACTION_CALLFUNC_HPP_
