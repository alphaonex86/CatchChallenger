// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_MOVETO_HPP_
#define CLIENT_QTOPENGL_ACTION_MOVETO_HPP_

#include "Action.hpp"

class MoveTo : public Action {
 public:
  ~MoveTo();

  static MoveTo* Create(int milliseconds, int x, int y);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;

  int delta_x_;
  int delta_y_;
  int end_x_;
  int end_y_;
  int milliseconds_;
  int timeout_;
  int ellapsed_;

  bool finish_x_;
  bool finish_y_;

  MoveTo();
  void CalculateDelta();
};
#endif  // CLIENT_QTOPENGL_ACTION_MOVETO_HPP_
