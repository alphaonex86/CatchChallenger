// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTING_HPP_
#define CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTING_HPP_

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../../../general/base/GeneralStructuresXml.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Combo.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"
#include "../../../ui/ThemedButton.hpp"

namespace Scenes {
class Crafting : public UI::Dialog {
 public:
  static Crafting *Create();
  ~Crafting();

  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;

 private:
  UI::Label *material_;
  UI::Label *name_;
  UI::Label *details_;
  UI::Button *create_;
  Sprite *item_icon_;
  UI::ListView *craft_content_;
  UI::ListView *materials_content_;
  Node *selected_;
  std::vector<std::vector<std::pair<uint16_t, uint32_t>>>
      materialOfRecipeInUsing;
  std::vector<std::pair<uint16_t, uint32_t>> productOfRecipeInUsing;

  Crafting();
  void OnItemClick(Node *node);
  void OnCreateClick();
  void LoadMaterials();
  void LoadRecipes();
  void NewLanguage();
  void OnRecipeUsedSlot(const CatchChallenger::RecipeUsage &recipeUsage);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_CRAFTING_CRAFTING_HPP_
