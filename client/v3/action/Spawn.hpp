// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_SPAWN_HPP_
#define CLIENT_QTOPENGL_ACTION_SPAWN_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Action.hpp"

using std::vector;

class Spawn : public Action {
 public:
  ~Spawn();

  static Spawn* Create(const vector<Action*>& actions);
  static Spawn* Create(Action* action, ...);

  Action* ActionAt(size_t index);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  std::vector<Action*> actions_;
  uint8_t done_count_;
  bool done_;

  Spawn();
};
#endif  // CLIENT_QTOPENGL_ACTION_SPAWN_HPP_
