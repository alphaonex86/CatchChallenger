// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_DELAY_HPP_
#define CLIENT_QTOPENGL_ACTION_DELAY_HPP_

#include "Action.hpp"

class Delay : public Action {
 public:
  ~Delay();

  static Delay* Create(int milliseconds);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;

  unsigned int ellapsed_;
  unsigned int timeout_;

  Delay();
};
#endif  // CLIENT_QTOPENGL_ACTION_DELAY_HPP_
