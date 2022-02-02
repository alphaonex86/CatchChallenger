// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_REPEATFOREVER_HPP_
#define CLIENT_QTOPENGL_ACTION_REPEATFOREVER_HPP_

#include "Action.hpp"

class RepeatForever : public Action {
 public:
  ~RepeatForever();

  static RepeatForever *Create(Action *action);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;
  Action *action_;

  RepeatForever();
};
#endif  // CLIENT_QTOPENGL_ACTION_REPEATFOREVER_HPP_
