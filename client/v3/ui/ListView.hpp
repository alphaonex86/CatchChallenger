// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_UI_LISTVIEW_HPP_
#define CLIENT_QTOPENGL_UI_LISTVIEW_HPP_

#include <QGraphicsWidget>
#include <vector>

#include "../core/Node.hpp"

namespace UI {
class ListView : public Node {
 public:
  static ListView *Create(Node *parent = nullptr);
  ~ListView();

  /** Add an item to the listview */
  void AddItem(Node *node);
  void AddItems(std::vector<Node *> nodes);
  Node *GetItem(uint8_t index);
  void RemoveItem(Node *node);
  void RemoveItem(uint8_t index);
  uint8_t Count();
  Node *GetSelectedItem();
  void SetSize(int width, int height);
  std::vector<Node *> Items();
  /** Release all items in the list */
  void Clear();
  void SetHorizontal(bool horizontal);
  void SetItemSpacing(int value);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnChildResize(Node *node) override;

 protected:
  QRectF slider_rect_;
  int spacing_;
  QPixmap slider_rail_;
  QPixmap slider_;
  bool show_slider_;
  qreal content_scale_;
  QRectF content_rect_;
  QPointF last_point_;
  QPointF initial_point_;
  QPixmap content_;
  std::vector<Node *> items_;
  bool is_horizontal_;
  bool is_drawable_;

  explicit ListView(Node *parent = nullptr);
  virtual void ReCalculateSize();

 private:
  bool is_slider_pressed_;

  Node *selected_item_;
  bool is_processing_resize_;
  bool has_dirty_items_;
};
}  // namespace UI
#endif  // CLIENT_QTOPENGL_UI_LISTVIEW_HPP_
