// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_BACKDROP_HPP_
#define CLIENT_V3_UI_BACKDROP_HPP_

#include <vector>

#include "../core/Node.hpp"

namespace UI {
class Backdrop : public Node {
 public:
  static Backdrop *Create(QColor color, Node *parent = nullptr);
  static Backdrop *Create(const std::function<void(QPainter *)> &on_draw, Node *parent = nullptr);
  ~Backdrop();

  void Draw(QPainter *painter) override;

  void SetOnDraw(const std::function<void(QPainter *)> &on_draw);

 protected:

  explicit Backdrop(QColor color, const std::function<void(QPainter *)> &on_draw, Node *parent = nullptr);

 private:
  QColor color_;
  std::function<void(QPainter *)> on_draw_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_BACKDROP_HPP_
