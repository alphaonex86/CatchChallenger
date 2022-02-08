// Copyright 2021 CatchChallenger
#include "Inventory.hpp"

#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "InventoryItem.hpp"

using Scenes::Inventory;
using Scenes::InventoryItem;
using std::placeholders::_1;

Inventory::Inventory() : Node(nullptr) {
  lastItemSelected = -1;
  lastItemSize = 0;
  itemCount = 0;
  object_filter_ = Inventory::ObjectFilter::ObjectFilter_All;
  inSelection = false;
  connexionManager = ConnectionManager::GetInstance();

  inventory = UI::GridView::Create(this);
  inventory->SetItemSpacing(5);

  descriptionBack = Sprite::Create(this);
  inventory_description = UI::Label::Create(this);
  inventoryDestroy =
      UI::Button::Create(":/CC/images/interface/delete.png", this);
  inventoryUse = UI::Button::Create(":/CC/images/interface/validate.png", this);

  inventoryUse->SetOnClick(std::bind(&Inventory::inventoryUse_slot, this));
  inventoryDestroy->SetOnClick(
      std::bind(&Inventory::inventoryDestroy_slot, this));

  QObject::connect(PlayerInfo::GetInstance(), &PlayerInfo::OnUpdateInfo,
                   std::bind(&Inventory::OnUpdateInfo, this, _1));
}

Inventory::~Inventory() {
  delete inventory;
  inventory = nullptr;
  delete descriptionBack;
  descriptionBack = nullptr;
}

Inventory *Inventory::Create() { return new (std::nothrow) Inventory(); }

void Inventory::Draw(QPainter *painter) { (void)painter; }

void Inventory::SetVar(const Inventory::ObjectFilter &filter,
                       const bool inSelection, const int lastItemSelected) {
  bool force = object_filter_ != filter;
  this->inSelection = inSelection;
  object_filter_ = filter;
  this->lastItemSelected = lastItemSelected;

  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  itemCount = playerInformations.items.size();
  UpdateInventory(64, force);
}

void Inventory::UpdateInventory(uint8_t targetSize, bool force) {
  if (!force && lastItemSize == targetSize) return;
  lastItemSize = targetSize;
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  inventory_description->SetEnabled(false);
  inventoryUse->SetEnabled(false);
  inventoryDestroy->SetEnabled(false);
  inventory->Clear();
  inventory->SetItemSize(targetSize, targetSize);
  itemCount = playerInformations.items.size();
  auto i = playerInformations.items.begin();
  while (i != playerInformations.items.cend()) {
    const uint16_t id = i->first;
    bool show = !inSelection;
    if (inSelection) {
      switch (object_filter_) {
        case ObjectFilter_Seed:
          // reputation requierement control is into load_plant_inventory() NOT:
          // on_listPlantList_itemSelectionChanged()
          if (QtDatapackClientLoader::datapackLoader->itemToPlants.find(id) !=
              QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
            show = true;
          break;
        case ObjectFilter_UseInFight:
          if (connexionManager->client->isInFightWithWild() &&
              CatchChallenger::CommonDatapack::commonDatapack.items.trap.find(
                  id) != CatchChallenger::CommonDatapack::commonDatapack.items
                             .trap.cend())
            show = true;
          else if (CatchChallenger::CommonDatapack::commonDatapack.items
                       .monsterItemEffect.find(id) !=
                   CatchChallenger::CommonDatapack::commonDatapack.items
                       .monsterItemEffect.cend())
            show = true;
          else
            show = false;
          break;
        default:
          qDebug() << "object_filter_ is unknow into load_inventory()";
          break;
      }
    }
    if (show) {
      const DatapackClientLoader::ItemExtra &itemExtra =
          QtDatapackClientLoader::datapackLoader->itemsExtra.at(id);
      auto item = InventoryItem::Create();
      if (QtDatapackClientLoader::datapackLoader->itemsExtra.find(id) !=
          QtDatapackClientLoader::datapackLoader->itemsExtra.cend()) {
        QPixmap p =
            QtDatapackClientLoader::datapackLoader->getItemExtra(id).image;
        p = p.scaledToHeight(targetSize);
        item->SetPixmap(p);
        if (i->second > 1) item->SetText(QString::number(i->second) + " ");
        item->SetText(item->Text() + QString::fromStdString(itemExtra.name));
        item->SetToolTip(QString::fromStdString(itemExtra.name));
      } else {
        item->SetPixmap(
            QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        if (i->second > 1)
          item->SetText(QStringLiteral("id: %1 (x%2)").arg(id).arg(i->second));
        else
          item->SetText(QStringLiteral("id: %1").arg(id));
      }
      item->SetData(99, id);
      item->SetOnClick(std::bind(&Inventory::OnItemClick, this, _1));
      inventory->AddItem(item);
      if (id == lastItemSelected) {
        item->SetSelected(true);
        on_inventory_itemSelectionChanged();
      }
    }
    ++i;
  }
}

void Inventory::inventoryUse_slot() {
  if (lastItemSelected < 0) return;
  if (on_use_item_) {
    bool isRecipe = CatchChallenger::CommonDatapack::commonDatapack
                        .itemToCraftingRecipes.find(lastItemSelected) !=
                    CatchChallenger::CommonDatapack::commonDatapack
                        .itemToCraftingRecipes.cend();
    if (isRecipe) {
      on_use_item_(ObjectType::kRecipe, lastItemSelected, 1);
    } else {
      on_use_item_(ObjectType::kSeed, lastItemSelected, 1);
    }
  }
}

void Inventory::inventoryDestroy_slot() {
  if (lastItemSelected < 0) return;
  if (on_delete_item_) {
    on_delete_item_(lastItemSelected);
  }
}

void Inventory::on_inventory_itemSelectionChanged() {
  if (QtDatapackClientLoader::datapackLoader->itemsExtra.find(
          lastItemSelected) ==
      QtDatapackClientLoader::datapackLoader->itemsExtra.cend()) {
    inventory_description->SetVisible(false);
    inventoryUse->SetEnabled(false);
    inventoryDestroy->SetEnabled(!inSelection);
    inventory_description->SetText(QObject::tr("Unknown description"));
    return;
  }
  const QtDatapackClientLoader::ItemExtra &content =
      QtDatapackClientLoader::datapackLoader->itemsExtra.at(lastItemSelected);
  inventoryDestroy->SetEnabled(!inSelection);
  inventory_description->SetVisible(true);
  inventory_description->SetText(QString::fromStdString(content.description));

  bool isRecipe = false;
  {
    /* is a recipe */
    isRecipe = CatchChallenger::CommonDatapack::commonDatapack
                   .itemToCraftingRecipes.find(lastItemSelected) !=
               CatchChallenger::CommonDatapack::commonDatapack
                   .itemToCraftingRecipes.cend();
    if (isRecipe) {
      const uint16_t &recipeId =
          CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes
              .at(lastItemSelected);
      const CatchChallenger::CraftingRecipe &recipe =
          CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(
              recipeId);
      if (!connexionManager->client->haveReputationRequirements(
              recipe.requirements.reputation)) {
        std::string string;
        unsigned int index = 0;
        while (index < recipe.requirements.reputation.size()) {
          string +=
              CatchChallenger::FacilityLibClient::reputationRequirementsToText(
                  recipe.requirements.reputation.at(index));
          index++;
        }
        inventory_description->SetText(
            inventory_description->Text() + "\n" +
            QObject::tr(
                "<span style=\"color:#D50000\">Don't meet the requirements: "
                "%1</span>")
                .arg(QString::fromStdString(string)));
        isRecipe = false;
      }
    }
  }
  inventoryUse->SetEnabled(
      inSelection || isRecipe ||
      /* is a repel */
      CatchChallenger::CommonDatapack::commonDatapack.items.repel.find(
          lastItemSelected) !=
          CatchChallenger::CommonDatapack::commonDatapack.items.repel.cend() ||
      /* is a item with monster effect */
      CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect
              .find(lastItemSelected) !=
          CatchChallenger::CommonDatapack::commonDatapack.items
              .monsterItemEffect.cend() ||
      /* is a item with monster effect out of fight */
      (CatchChallenger::CommonDatapack::commonDatapack.items
               .monsterItemEffectOutOfFight.find(lastItemSelected) !=
           CatchChallenger::CommonDatapack::commonDatapack.items
               .monsterItemEffectOutOfFight.cend() &&
       !connexionManager->client->isInFight()) ||
      /* is a evolution item */
      CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(
          lastItemSelected) != CatchChallenger::CommonDatapack::commonDatapack
                                   .items.evolutionItem.cend() ||
      /* is a evolution item */
      (CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.find(
           lastItemSelected) != CatchChallenger::CommonDatapack::commonDatapack
                                    .items.itemToLearn.cend() &&
       !connexionManager->client->isInFight()));
}

void Inventory::OnItemClick(Node *node) {
  lastItemSelected = node->Data(99);
  on_inventory_itemSelectionChanged();
  auto items = inventory->Items();
  for (auto item : items) {
    if (node == item) continue;
    static_cast<InventoryItem *>(item)->SetSelected(false);
  }
}

void Inventory::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(QObject::tr(
      "Bag", "You can translate by inventory if the word is shorter"));
  inventory->RegisterEvents();
  auto items = inventory->Items();
  for (auto item : items) {
    static_cast<InventoryItem *>(item)->SetSelected(false);
  }
  inventoryDestroy->RegisterEvents();
  inventoryUse->RegisterEvents();
}

void Inventory::UnRegisterEvents() {
  inventory->UnRegisterEvents();
  inventoryDestroy->UnRegisterEvents();
  inventoryUse->UnRegisterEvents();
}

void Inventory::OnResize() {
  auto font = inventory_description->GetFont();
  int space = 30;

  inventoryDestroy->SetSize(60, 72);
  inventoryUse->SetSize(60, 72);
  font.setPixelSize(18);

  auto bounds = BoundingRect();
  int yTemp = bounds.height() - inventoryDestroy->Height() - 5;
  int xTemp = 0;

  inventory->SetPos(0, 0);
  inventory->SetSize(bounds.width(), yTemp);

  inventoryDestroy->SetPos(xTemp, yTemp);
  xTemp += inventoryDestroy->Width() + space / 2;
  descriptionBack->SetPos(xTemp, yTemp);
  inventory_description->SetPos(xTemp + 10, yTemp + 10);
  inventory_description->SetPixelSize(12);
  inventoryUse->SetPos(bounds.width() - inventoryUse->Width(), yTemp);
  int pos = inventoryUse->X() - space -
            (inventoryDestroy->X() + inventoryDestroy->Width());
  descriptionBack->SetPixmap(":/CC/images/interface/b1-trimmed.png");
  descriptionBack->Strech(18, 16, 10, pos, inventoryDestroy->Height());
  inventory_description->SetWidth(descriptionBack->Width());
}

void Inventory::SetOnUseItem(
    std::function<void(ObjectType, uint16_t, uint32_t)> callback) {
  on_use_item_ = callback;
}

void Inventory::SetOnDeleteItem(std::function<void(uint16_t)> callback) {
  on_delete_item_ = callback;
}

void Inventory::OnUpdateInfo(PlayerInfo *info) { lastItemSize = 0; }
