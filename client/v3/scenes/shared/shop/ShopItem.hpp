// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_SHOP_SHOPITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_SHOP_SHOPITEM_HPP_

#include "../../../ui/SelectableItem.hpp"
#include "../../../core/Sprite.hpp"

namespace Scenes {
class ShopItem : public UI::SelectableItem {
 public:
  ~ShopItem();
  static ShopItem *Create(uint16_t object_id, uint32_t quantity,
                          uint32_t price);
  std::string Name();
  uint16_t ObjectId();
  uint32_t Quantity();
  uint32_t Price();

  void DrawContent(QPainter *painter) override;
  void OnResize() override;

 private:
  uint16_t object_id_;
  uint32_t quantity_;
  uint32_t price_;

  std::string name_;

  ShopItem(uint16_t object_id, uint32_t quantity, uint32_t price);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_SHOP_SHOPITEM_HPP_
