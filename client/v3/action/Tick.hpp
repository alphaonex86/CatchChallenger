// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_TICK_HPP_
#define CLIENT_QTOPENGL_ACTION_TICK_HPP_

#include <cstdint>
#include <functional>

#include "Action.hpp"

class Tick : public Action {
 public:
  ~Tick();

  static Tick *Create(int milliseconds, const std::function<bool()> &callback);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;
  void SetOnTick(const std::function<bool()> &callback);

 private:
  bool done_;

  unsigned int ellapsed_;
  unsigned int timeout_;
  std::function<bool()> on_tick_;

  Tick();
  void OnFinish();
};
#endif  // CLIENT_QTOPENGL_ACTION_TICK_HPP_
