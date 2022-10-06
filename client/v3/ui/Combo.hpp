// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_COMBO_HPP_
#define CLIENT_V3_UI_COMBO_HPP_

#include <QGraphicsItem>
#include <QPointF>
#include <unordered_map>
#include <vector>

#include "../core/Node.hpp"
#include "../core/Sprite.hpp"
#include "Label.hpp"

namespace UI {
class Combo : public Node {
 public:
  static Combo *Create(QString pix, Node *parent = nullptr);
  static Combo *Create(Node *parent = nullptr);
  ~Combo();

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;
  void SetCurrentIndex(uint8_t index);
  void SetOnSelectChange(std::function<void(uint8_t)> callback);
  uint8_t CurrentIndex();
  void AddItem(const QString &item);
  int ItemData(uint8_t index, int role);
  void SetItemData(uint8_t index, int role, int value);
  void SetSize(qreal width, qreal height);
  void SetSize(QSizeF& size);
  void SetWidth(qreal width);
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  size_t Count() const;
  void Clear();

  void Draw(QPainter *painter) override;
  void OnResize() override;

 protected:
  explicit Combo(QString pix, Node *parent = nullptr);

 private:
  Sprite *background_;
  uint8_t current_index_;
  std::vector<QString> items_;
  Label *label_;
  std::function<void(uint8_t)> on_select_change_;
  std::unordered_map<uint8_t, std::unordered_map<int, int>> item_data_;
  qreal last_z_;
  bool is_menu_open_;
  QRectF inner_bounding_rect_;

  void OnClick(uint8_t selected);
  void OpenMenu();
  void CloseMenu();
};
}  // namespace UI

#endif  // CLIENT_V3_UI_COMBO_HPP_
