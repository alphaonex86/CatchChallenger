// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_BLINK_HPP_
#define CLIENT_QTOPENGL_ACTION_BLINK_HPP_

#include <cstdint>

#include "Action.hpp"

class Blink : public Action {
 public:
  ~Blink();

  static Blink *Create(int milliseconds, unsigned int times);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;

  unsigned int ellapsed_;
  unsigned int timeout_;
  unsigned int times_;
  unsigned int index_;

  Blink();
};
#endif  // CLIENT_QTOPENGL_ACTION_BLINK_HPP_
