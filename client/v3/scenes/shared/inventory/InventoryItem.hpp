// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORYITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORYITEM_HPP_

#include "../../../core/Node.hpp"

namespace Scenes {
class InventoryItem : public Node {
 public:
  static InventoryItem* Create();

  void SetPixmap(const QPixmap pixmap);
  void SetText(const QString text);
  QString Text() const;
  void SetSelected(bool selected);
  bool IsSelected();

  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;

 private:
  bool is_selected_;
  QPixmap pixmap_;
  QString text_;

  InventoryItem();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_INVENTORYITEM_HPP_
