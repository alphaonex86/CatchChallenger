// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_SEQUENCE_HPP_
#define CLIENT_QTOPENGL_ACTION_SEQUENCE_HPP_

#include <cstddef>
#include <vector>

#include "Action.hpp"

using std::vector;

class Sequence : public Action {
 public:
  ~Sequence();

  static Sequence* Create(const vector<Action*>& actions);
  static Sequence* Create(Action* action, ...);

  Action* ActionAt(size_t index);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  std::vector<Action*> actions_;
  size_t current_index_;
  Action* current_;
  bool done_;

  Sequence();
};
#endif  // CLIENT_QTOPENGL_ACTION_SEQUENCE_HPP_
