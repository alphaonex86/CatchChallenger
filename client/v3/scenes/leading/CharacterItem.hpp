// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_LEADING_CHARACTERITEM_HPP_
#define CLIENT_V3_SCENES_LEADING_CHARACTERITEM_HPP_

#include "../../ui/SelectableItem.hpp"

namespace Scenes {
class CharacterItem : public UI::SelectableItem {
 public:
  static CharacterItem *Create();

  void OnResize() override;
  void DrawContent(QPainter *painter) override;
  void SetIcon(const QString &icon);
  void SetTitle(const QString &text);
  void SetSubTitle(const QString &text);

 private:
  explicit CharacterItem();

  QString icon_;
  QString title_;
  QString subtitle_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_LEADING_CHARACTERITEM_HPP_
