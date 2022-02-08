// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTINGITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTINGITEM_HPP_

#include "../../../ui/SelectableItem.hpp"

namespace Scenes {
class CraftingItem : public UI::SelectableItem {
 public:
  static CraftingItem *Create();
  ~CraftingItem();

  void DrawContent(QPainter *painter) override;
  void OnResize() override;

  void SetIcon(const QPixmap &pixmap);
  void SetName(const QString &text);
  void SetItemId(const uint16_t &item_id);
  void SetRecipe(const uint16_t &recipe);

  uint16_t ItemId() const;
  uint16_t Recipe() const;
  QString Name() const;
  QString Description() const;
  QPixmap Icon() const;

 private:
  QString text_;
  QString description_;
  uint16_t item_id_;
  uint16_t recipe_;
  QPixmap icon_;

  CraftingItem();
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTINGITEM_HPP_
