// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_TEST_TESTITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_TEST_TESTITEM_HPP_

#include "../../core/Node.hpp"

namespace Scenes {
class TestItem : public Node {
 public:
  static TestItem* Create();

  void SetPixmap(const QPixmap pixmap);
  void SetText(const QString text);
  QString Text() const;
  void SetSelected(bool selected);
  bool IsSelected();

  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void OnResize() override;

 private:
  bool is_selected_;
  QPixmap pixmap_;
  QString text_;

  TestItem();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_TEST_TESTITEM_HPP_
