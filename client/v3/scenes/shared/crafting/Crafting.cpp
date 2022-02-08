// Copyright 2021 CatchChallenger
#include "Crafting.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"
#include "../../../ui/ThemedItem.hpp"
#include "CraftingItem.hpp"

using Scenes::Crafting;
using std::placeholders::_1;
using std::placeholders::_2;
using UI::IconItem;

Crafting::Crafting() {
  SetDialogSize(1200, 600);
  selected_ = nullptr;

  material_ = UI::Label::Create(this);
  name_ = UI::Label::Create(this);
  details_ = UI::Label::Create(this);
  material_->SetAlignment(Qt::AlignCenter);
  item_icon_ = Sprite::Create(this);

  craft_content_ = UI::ListView::Create(this);
  materials_content_ = UI::ListView::Create(this);

  create_ = UI::Button::Create(":/CC/images/interface/button.png", this);
  create_->SetOnClick(std::bind(&Crafting::OnCreateClick, this));

  QObject::connect(ConnectionManager::GetInstance()->client,
                   &CatchChallenger::Api_client_real::QtrecipeUsed,
                   std::bind(&Crafting::OnRecipeUsedSlot, this, _1));

  NewLanguage();
}

Crafting::~Crafting() {}

Crafting *Crafting::Create() { return new (std::nothrow) Crafting(); }

void Crafting::NewLanguage() {
  SetTitle(tr("Crafting"));
  create_->SetText(tr("Create"));
  material_->SetText(tr("Materials"));
}

void Crafting::OnEnter() {
  UI::Dialog::OnEnter();
  craft_content_->RegisterEvents();
  materials_content_->RegisterEvents();
  create_->RegisterEvents();

  LoadRecipes();

  OnScreenResize();
}

void Crafting::OnExit() {
  craft_content_->UnRegisterEvents();
  materials_content_->UnRegisterEvents();
  create_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void Crafting::OnCreateClick() {
  if (selected_ == nullptr) return;

  auto current = static_cast<CraftingItem *>(selected_);

  const CatchChallenger::CraftingRecipe &content =
      CatchChallenger::CommonDatapack::commonDatapack
          .craftingRecipes[current->Recipe()];

  QStringList mIngredients;
  // load the materials
  std::vector<std::pair<uint16_t, uint32_t>> recipeUsage;

  uint16_t index = 0;
  while (index < content.materials.size()) {
    std::pair<uint16_t, uint32_t> pair;
    pair.first = content.materials.at(index).item;
    pair.second = content.materials.at(index).quantity;
    PlayerInfo::GetInstance()->RemoveInventory(pair.first, pair.second);
    recipeUsage.push_back(pair);
    index++;
  }
  materialOfRecipeInUsing.push_back(recipeUsage);

  // the product do
  std::pair<uint16_t, uint32_t> pair;
  pair.first = content.doItemId;
  pair.second = content.quantity;
  productOfRecipeInUsing.push_back(pair);

  // update the UI
  LoadMaterials();

  // send to the network
  ConnectionManager::GetInstance()->client->useRecipe(current->Recipe());
  PlayerInfo::GetInstance()->AppendReputationPoints(
      CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes
          .at(current->Recipe())
          .rewards.reputation);
}

void Crafting::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();
  qreal width = content.width() / 3;
  qreal block_1 = content.x();
  qreal block_2 = content.x() + width + 20;

  craft_content_->SetPos(block_1, content.y());
  craft_content_->SetSize(width, content.height());

  item_icon_->SetPos(block_2, content.y());
  item_icon_->SetSize(60, 60);

  details_->SetPixelSize(12);
  name_->SetPos(item_icon_->Right() + 10, content.y());
  details_->SetPos(name_->X(), name_->Bottom() + 10);

  material_->SetPos(block_2, details_->Y() + 50);

  create_->SetSize(150, 45);
  create_->SetPixelSize(14);
  create_->SetPos(block_2 + width - create_->Width() / 2,
                  content.bottom() - create_->Height());

  materials_content_->SetPos(block_2, material_->Bottom() + 10);
  materials_content_->SetSize((width * 2) - 20,
                              content.height() -
                                  (materials_content_->Y() - content.y()) -
                                  create_->Height() - 20);
}

void Crafting::OnItemClick(Node *node) {
  selected_ = node;
  auto items = craft_content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<CraftingItem *>(item)->SetSelected(false);
  }
  LoadMaterials();
}

void Crafting::LoadMaterials() {
  materials_content_->Clear();
  if (selected_ == nullptr) {
    name_->SetVisible(false);
    details_->SetVisible(false);
    item_icon_->SetVisible(false);
    create_->SetEnabled(false);
    return;
  }
  name_->SetVisible(true);
  details_->SetVisible(true);
  item_icon_->SetVisible(true);

  auto craft = static_cast<CraftingItem *>(selected_);
  uint16_t item_id = craft->ItemId();

  if (QtDatapackClientLoader::datapackLoader->itemsExtra.find(item_id) ==
      QtDatapackClientLoader::datapackLoader->itemsExtra.cend()) {
    details_->SetText(tr("Unknown description"));
    create_->SetEnabled(false);
    return;
  }

  name_->SetText(craft->Name());
  details_->SetText(craft->Description());
  item_icon_->SetPixmap(craft->Icon());

  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  materials_content_->Clear();

  const CatchChallenger::CraftingRecipe &content =
      CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(
          craft->Recipe());

  // load the materials
  bool haveMaterials = true;
  unsigned int index = 0;
  QString nameMaterials;
  uint32_t quantity;
  while (index < content.materials.size()) {
    // load the material item
    auto item = IconItem::Create();
    if (QtDatapackClientLoader::datapackLoader->itemsExtra.find(
            content.materials.at(index).item) !=
        QtDatapackClientLoader::datapackLoader->itemsExtra.cend()) {
      nameMaterials = QString::fromStdString(
          QtDatapackClientLoader::datapackLoader
              ->itemsExtra[content.materials.at(index).item]
              .name);
      item->SetIcon(QtDatapackClientLoader::datapackLoader
                        ->getItemExtra(content.materials.at(index).item)
                        .image);
    } else {
      nameMaterials =
          tr("Unknow item (%1)").arg(content.materials.at(index).item);
      item->SetIcon(
          QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
    }

    // load the quantity into the inventory
    quantity = 0;
    if (info.items.find(content.materials.at(index).item) !=
        info.items.cend()) {
      quantity = info.items.at(content.materials.at(index).item);
    }

    // load the display
    item->SetText(tr("Needed: %1 %2\nIn the inventory: %3 %4")
                      .arg(content.materials.at(index).quantity)
                      .arg(nameMaterials)
                      .arg(quantity)
                      .arg(nameMaterials));
    if (quantity < content.materials.at(index).quantity) {
      item->SetDisabled(true);
    }

    if (quantity < content.materials.at(index).quantity) haveMaterials = false;

    materials_content_->AddItem(item);
    index++;
  }
  create_->SetEnabled(haveMaterials);
  OnScreenResize();
}

void Crafting::LoadRecipes() {
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  craft_content_->Clear();
  uint16_t index = 0;
  while (index <=
         CatchChallenger::CommonDatapack::commonDatapack.craftingRecipesMaxId) {
    uint16_t recipe = index;
    if (info.recipes[recipe / 8] & (1 << (7 - recipe % 8))) {
      // load the material item
      if (CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.find(
              recipe) != CatchChallenger::CommonDatapack::commonDatapack
                             .craftingRecipes.cend()) {
        const uint16_t &itemId = CatchChallenger::CommonDatapack::commonDatapack
                                     .craftingRecipes[recipe]
                                     .doItemId;
        auto item = CraftingItem::Create();
        if (QtDatapackClientLoader::datapackLoader->itemsExtra.find(itemId) !=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend()) {
          item->SetIcon(
              QtDatapackClientLoader::datapackLoader->getItemExtra(itemId)
                  .image);
          item->SetName(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->itemsExtra[itemId].name));
        } else {
          item->SetIcon(
              QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
          item->SetName(tr("Unknow item: %1").arg(itemId));
        }
        item->SetRecipe(recipe);
        item->SetItemId(itemId);
        item->SetOnClick(std::bind(&Crafting::OnItemClick, this, _1));
        craft_content_->AddItem(item);
      }
    }
    ++index;
  }

  OnItemClick(nullptr);
}

void Crafting::OnRecipeUsedSlot(
    const CatchChallenger::RecipeUsage &recipeUsage) {
  switch (recipeUsage) {
    case CatchChallenger::RecipeUsage_ok:
      materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());

      PlayerInfo::GetInstance()->AddInventory(
          productOfRecipeInUsing.front().first,
          productOfRecipeInUsing.front().second);
      productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
      // update the UI
      LoadRecipes();
      break;
    case CatchChallenger::RecipeUsage_impossible: {
      unsigned int index = 0;
      while (index < materialOfRecipeInUsing.front().size()) {
        PlayerInfo::GetInstance()->AddInventory(
            materialOfRecipeInUsing.front().at(index).first,
            materialOfRecipeInUsing.front().at(index).first);
        index++;
      }
      materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
      productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
      // update the UI
      LoadRecipes();
    } break;
    case CatchChallenger::RecipeUsage_failed:
      materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
      productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
      break;
    default:
      qDebug() << "recipeUsed() unknow code";
      return;
  }
}
