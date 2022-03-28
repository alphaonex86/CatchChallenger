// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_SLIMPROGRESSBAR_HPP_
#define CLIENT_V3_UI_SLIMPROGRESSBAR_HPP_

#include <QGraphicsWidget>

#include "../core/Node.hpp"
#include "../action/Tick.hpp"
#include "Label.hpp"

namespace UI {
class SlimProgressbar : public Node {
 public:
  static SlimProgressbar *Create(Node *parent = nullptr);
  virtual ~SlimProgressbar();

  void SetSize(int width, int height);
  void SetMaximum(const int &value);
  void SetMinimum(const int &value);
  void SetValue(const int &value);
  void IncrementValue(const int &delta, const bool &animate);
  int Maximum();
  int Minimum();
  int Value();
  void SetOnIncrementDone(const std::function<void()> &callback);

  void Draw(QPainter *painter) override;

 private:
  int value_;
  int min_;
  int max_;
  float delta_value_;
  float internal_value_;
  unsigned int increment_times_;
  std::function<void()> on_increment_done_;

  QColor background_color_;
  QColor foreground_color_;

  Tick *tick_;

  SlimProgressbar(Node *parent = nullptr);

  bool OnTick();
};
}  // namespace UI

#endif  // CLIENT_V3_UI_SLIMPROGRESSBAR_HPP_
