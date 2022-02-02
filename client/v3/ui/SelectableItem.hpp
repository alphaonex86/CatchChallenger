// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_UI_SELECTABLEITEM_HPP_
#define CLIENT_V3_UI_SELECTABLEITEM_HPP_

#include "../core/Node.hpp"

namespace UI {
class SelectableItem : public Node {
 public:
  ~SelectableItem();
  void SetSelected(bool is_selected);
  void SetDisabled(bool is_disabled);
  void ReDrawContent();
  virtual void DrawContent(QPainter *painter) = 0;

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;

 protected:
  QString bg_unselected_;
  QString bg_selected_;
  QString bg_disabled_;

  SelectableItem();

 private:
  bool is_selected_;
  bool is_disabled_;
  QPixmap *content_cache_;
  uint16_t strech_x_;
  uint16_t strech_y_;
  uint16_t border_size_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_SELECTABLEITEM_HPP_
