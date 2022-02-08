// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_THEMEDITEM_HPP_
#define CLIENT_V3_UI_THEMEDITEM_HPP_

#include "SelectableItem.hpp"

namespace UI {
class DotItem: public SelectableItem {
 public:
  static DotItem *Create();

  void OnResize() override;
  void DrawContent(QPainter *painter) override;
  void SetText(const QString &text);
  void SetPixelSize(uint8_t size);

 private:
  explicit DotItem();

  QString text_;
  uint8_t text_size_;
};

class IconItem: public SelectableItem {
 public:
  static IconItem *Create();

  void OnResize() override;
  void DrawContent(QPainter *painter) override;
  void SetText(const QString &text);
  void SetPixelSize(uint8_t size);
  void SetIcon(const QPixmap &icon);

 private:
  explicit IconItem();

  QString text_;
  QPixmap icon_;
  uint8_t text_size_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_THEMEDITEM_HPP_
