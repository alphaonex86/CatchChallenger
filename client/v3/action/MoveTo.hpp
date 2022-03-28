// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ACTION_MOVETO_HPP_
#define CLIENT_V3_ACTION_MOVETO_HPP_

#include "Action.hpp"
#include <QPointF>

class MoveTo : public Action {
 public:
  ~MoveTo();

  static MoveTo* Create(int milliseconds, qreal x, qreal y);
  static MoveTo* Create(int milliseconds, QPointF point);

  void Step(unsigned int ellapsed) override;
  void Stop() override;
  void Start() override;
  bool IsDone() override;

 private:
  bool done_;

  qreal delta_x_;
  qreal delta_y_;
  qreal end_x_;
  qreal end_y_;
  int milliseconds_;
  int timeout_;
  int ellapsed_;

  bool finish_x_;
  bool finish_y_;

  MoveTo();
  void CalculateDelta();
};
#endif  // CLIENT_V3_ACTION_MOVETO_HPP_
