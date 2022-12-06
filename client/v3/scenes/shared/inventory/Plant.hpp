// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_INVENTORY_PLANT_HPP_
#define CLIENT_V3_SCENES_SHARED_INVENTORY_PLANT_HPP_

#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/CommonTypes.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/Backdrop.hpp"
#include "Inventory.hpp"

namespace Scenes {
class Plant : public Node {
 public:
  static Plant *Create();
  ~Plant();

  void OnResize() override;
  void Draw(QPainter *painter) override;
  void SetVar(const bool inSelection, const int lastItemSelected = -1);
  void SetOnUseItem(
          std::function<void(ObjectCategory, uint16_t, uint32_t, uint8_t)> callback);
  void RegisterEvents();
  void UnRegisterEvents();

 private:
  UI::ListView *plantList;

  Sprite *descriptionBack;
  UI::Label *inventory_description;
  UI::Button *inventoryUse;
  UI::Button *select_;
  UI::Backdrop *backdrop_;

  ConnectionManager *connexionManager;
  bool is_selection_mode_;
  uint8_t lastItemSize;
  int lastItemSelected;
  std::function<void(ObjectCategory, uint16_t, uint32_t, uint8_t)> on_use_item_;

  Plant();
  void NewLanguage();
  void updatePlant();
  void inventoryUse_slot();
  void OnSelectItem(Node *item);
  QPixmap SetCanvas(const QPixmap &image, unsigned int size);
  QPixmap SetCanvas(const QImage &image, unsigned int size);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_INVENTORY_PLANT_HPP_
