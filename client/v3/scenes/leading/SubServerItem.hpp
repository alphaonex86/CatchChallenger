// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_LEADING_SUBSERVERITEM_HPP_
#define CLIENT_V3_SCENES_LEADING_SUBSERVERITEM_HPP_

#include "../../ui/SelectableItem.hpp"

namespace Scenes {
class SubServerItem : public UI::SelectableItem {
 public:
  static SubServerItem *Create();

  void OnResize() override;
  void DrawContent(QPainter *painter) override;
  void SetIcon(const QString &icon);
  void SetTitle(const QString &text);
  void SetSubTitle(const QString &text);
  void SetStat(const QString &text);
  void SetStar(const QString &text);

 private:
  explicit SubServerItem();

  QString icon_;
  QString title_;
  QString subtitle_;
  QString stat_;
  QString star_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_LEADING_SUBSERVERITEM_HPP_
