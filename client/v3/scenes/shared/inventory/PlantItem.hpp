// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_PLANTITEM_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_PLANTITEM_HPP_

#include "../../../ui/SelectableItem.hpp"

namespace Scenes {
class PlantItem : public UI::SelectableItem {
 public:
  ~PlantItem();
  static PlantItem *Create(uint16_t plant_id, uint32_t quantity);
  uint16_t PlantId();

  void DrawContent(QPainter *painter) override;
  void OnResize() override;

 private:
  uint16_t plant_id_;
  uint32_t quantity_;

  PlantItem(uint16_t plant_id, uint32_t quantity);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_PLANTITEM_HPP_
