// Copyright 2021 CatchChallenger
#include "Plant.hpp"

#include <math.h>

#include <QImage>
#include <QPainter>
#include <iostream>

#include "../../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../libraries/tiled/tiled_tile.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "../../../Constants.hpp"
#include "PlantItem.hpp"

using Scenes::Plant;

Plant::Plant() : Node() {
  lastItemSize = 0;
  lastItemSelected = -1;
  is_selection_mode_ = false;

  plantList = UI::ListView::Create(this);
  plantList->SetItemSpacing(5);

  connexionManager = ConnectionManager::GetInstance();

  select_ = UI::Button::Create(":/CC/images/interface/validate.png");
  select_->SetOnClick(std::bind(&Plant::inventoryUse_slot, this));

  NewLanguage();
}

Plant::~Plant() {
  select_->UnRegisterEvents();
  delete select_;
  select_ = nullptr;

  plantList->UnRegisterEvents();
  delete plantList;
  plantList = nullptr;
}

Plant *Plant::Create() { return new (std::nothrow) Plant(); }

void Plant::OnResize() {
  unsigned int space = 20;

  plantList->SetPos(10, 10);
  plantList->SetSize((Width() / 3) - 20, Height() - 20);
}

void Plant::NewLanguage() {}

void Plant::SetVar(const bool is_selection_mode, const int lastItemSelected) {
  is_selection_mode_ = is_selection_mode;
  this->lastItemSelected = lastItemSelected;
  updatePlant();
}

void Plant::updatePlant() {
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  plantList->Clear();
  auto i = playerInformations.items.begin();
  while (i != playerInformations.items.cend()) {
    const uint16_t id = i->first;
    if (QtDatapackClientLoader::GetInstance()->get_itemToPlants().find(id) !=
        QtDatapackClientLoader::GetInstance()->get_itemToPlants().cend()) {
      auto item = PlantItem::Create(id, i->second);
      item->SetData(99, id);
      item->SetOnClick(
          std::bind(&Plant::OnSelectItem, this, std::placeholders::_1));
      if (id == lastItemSelected) item->SetSelected(true);
      plantList->AddItem(item);
    }
    ++i;
  }
}

void Plant::inventoryUse_slot() {
  if (lastItemSelected == -1) return;
  on_use_item_(ObjectCategory::kSeed, lastItemSelected, 1, 0);
}

void Plant::OnSelectItem(Node *selected) {
  if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(selected->Data(
          99)) == QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend()) {
    return;
  }
  lastItemSelected = selected->Data(99);
  auto items = plantList->Items();
  for (auto item : items) {
    if (selected == item) continue;
    static_cast<PlantItem *>(item)->SetSelected(false);
  }
  ReDraw();
}

void Plant::RegisterEvents() {
  auto linked = static_cast<UI::LinkedDialog *>(Parent());
  linked->SetTitle(QObject::tr("PLANT"));

  if (is_selection_mode_) {
    linked->AddActionButton(select_);
  }
  linked->ShowNavigation(!is_selection_mode_);
  linked->RecalculateActionButtons();
  plantList->RegisterEvents();
  ReDraw();
}

void Plant::UnRegisterEvents() {
  auto linked = static_cast<UI::LinkedDialog *>(Parent());
  plantList->UnRegisterEvents();
  linked->RemoveActionButton(select_);
}

void Plant::Draw(QPainter *painter) {
  auto b2 = Sprite::Create(":/CC/images/interface/b2.png");

  b2->Strech(24, 22, 20, plantList->Width() + 10, plantList->Height() + 10);
  b2->SetPos(plantList->X() - 5, plantList->Y() - 5);
  b2->Render(painter);

  delete b2;

  if (lastItemSelected == -1) return;
  int item_id = lastItemSelected;

  auto icon = Sprite::Create();
  auto text = UI::Label::Create();

  int x_offset = (Width() / 3) + 10;
  int y_offset = 10;
  

  const DatapackClientLoader::ItemExtra &itemExtra =
      QtDatapackClientLoader::GetInstance()->get_itemsExtra().at(item_id);
  const uint8_t plant_id =
      QtDatapackClientLoader::GetInstance()->get_itemToPlants().at(item_id);
  const CatchChallenger::Plant &plant =
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant_id);
  const QtDatapackClientLoader::QtPlantExtra &contentExtra =
      QtDatapackClientLoader::GetInstance()->getPlantExtra(plant_id);

  const int icon_size = Constants::ButtonMediumHeight();

  if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(item_id) !=
      QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend()) {
    text->SetPixelSize(Constants::TextMediumSize());
    text->SetPos(x_offset, y_offset);
    text->SetWidth(Width() - x_offset);
    text->SetAlignment(Qt::AlignCenter);
    text->SetText(itemExtra.name);
    text->Render(painter);
    y_offset += text->Height();

    text->SetPixelSize(Constants::TextSmallSize());
    text->SetPos(x_offset, y_offset);
    text->SetText(itemExtra.description);
    text->Render(painter);

    y_offset += text->Height();

    text->SetAlignment(Qt::AlignLeft);
    text->SetText(
        QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(
            plant.fruits_seconds)));
    text->SetPos(x_offset, y_offset);
    text->Render(painter);

    text->SetAlignment(Qt::AlignRight);
    text->SetText(
        QObject::tr("Quantity: %1")
            .arg((double)plant.fix_quantity +
                 ((double)plant.random_quantity) / RANDOM_FLOAT_PART_DIVIDER));
    text->SetPos(x_offset, y_offset);
    text->Render(painter);
    y_offset += text->Height() + 20;

    if (contentExtra.tileset != nullptr) {
      uint8_t index = 0;

      while (index < 6) {
        if (contentExtra.tileset->tileAt(index) != nullptr) {
          icon->SetPixmap(SetCanvas(
              contentExtra.tileset->tileAt(index)->image(), icon_size));
          icon->SetPos(x_offset + icon_size * index, y_offset);
          icon->Render(painter);
        }

        index++;
      }
      // icon->SetPixmap(
      // SetCanvas(contentExtra.tileset->tileAt(0)->image(), icon_size));
      // icon->SetPos(x_offset, y_offset);
      // icon->Render(painter);

      // icon->SetPixmap(
      // SetCanvas(contentExtra.tileset->tileAt(1)->image(), icon_size));
      // icon->SetPos(x_offset + icon_size, y_offset);
      // icon->Render(painter);

      // icon->SetPixmap(
      // SetCanvas(contentExtra.tileset->tileAt(2)->image(), icon_size));
      // icon->SetPos(x_offset + icon_size * 2, y_offset);
      // icon->Render(painter);

      // icon->SetPixmap(
      // SetCanvas(contentExtra.tileset->tileAt(3)->image(), icon_size));
      // icon->SetPos(x_offset + icon_size * 3, y_offset);
      // icon->Render(painter);

      // icon->SetPixmap(
      // SetCanvas(contentExtra.tileset->tileAt(4)->image(), icon_size));
      // icon->SetPos(x_offset + icon_size * 4, y_offset);
      // icon->Render(painter);

      // icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(5)->image(),
      // 32)); icon->SetPos(240, 10); icon->Paint(painter, nullptr, nullptr);
    }
  } else {
    icon->SetPixmap(
        QtDatapackClientLoader::GetInstance()->defaultInventoryImage());
    icon->SetPos(x_offset + 10, 10);
    icon->Render(painter);

    text->SetPixelSize(Constants::TextSmallSize());
    text->SetPos(x_offset + 20, 10);
    text->Render(painter);

    if (contentExtra.tileset != nullptr) {
      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(0)->image(), 48));
      icon->ScaledToHeight(Constants::ButtonSmallHeight());
      icon->SetPos(x_offset + 40, 10);
      icon->Render(painter);

      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(1)->image(), 32));
      icon->SetPos(x_offset + 80, 10);
      icon->Render(painter);

      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(2)->image(), 32));
      icon->SetPos(x_offset + 120, 10);
      icon->Render(painter);

      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(3)->image(), 32));
      icon->SetPos(x_offset + 160, 10);
      icon->Render(painter);

      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(4)->image(), 32));
      icon->SetPos(x_offset + 200, 10);
      icon->Render(painter);

      icon->SetPixmap(SetCanvas(contentExtra.tileset->tileAt(5)->image(), 32));
      icon->SetPos(x_offset + 240, 10);
      icon->Render(painter);
    }
    text->SetText(
        QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(
            plant.fruits_seconds)));
    text->SetPos(x_offset + 280, 10);
    text->Render(painter);

    text->SetText(
        QObject::tr("Quantity: %1")
            .arg((double)plant.fix_quantity +
                 ((double)plant.random_quantity) / RANDOM_FLOAT_PART_DIVIDER));
    text->SetPos(x_offset + 320, 10);
    text->Render(painter);
  }

  delete icon;
  delete text;
}

QPixmap Plant::SetCanvas(const QPixmap &image, unsigned int size) {
  int wratio = floor((float)size / (float)image.width());
  int hratio = floor((float)size / (float)image.height());
  if (wratio < 1) wratio = 1;
  if (hratio < 1) hratio = 1;
  int minratio = wratio;
  if (minratio > hratio) minratio = hratio;
  QPixmap i = image.scaledToWidth(image.width() * minratio);
  QImage temp(size, size, QImage::Format_ARGB32_Premultiplied);
  temp.fill(Qt::transparent);
  QPainter paint;
  if (i.isNull()) abort();
  paint.begin(&temp);
  paint.drawPixmap((size - i.width()) / 2, (size - i.height()) / 2, i.width(),
                   i.height(), i);
  return QPixmap::fromImage(temp);
}

QPixmap Plant::SetCanvas(const QImage &image, unsigned int size) {
  int wratio = floor((float)size / (float)image.width());
  int hratio = floor((float)size / (float)image.height());
  if (wratio < 1) wratio = 1;
  if (hratio < 1) hratio = 1;
  int minratio = wratio;
  if (minratio > hratio) minratio = hratio;
  QImage i = image.scaledToWidth(image.width() * minratio);
  QImage temp(size, size, QImage::Format_ARGB32_Premultiplied);
  temp.fill(Qt::transparent);
  QPainter paint;
  if (i.isNull()) abort();
  paint.begin(&temp);
  paint.drawImage((size - i.width()) / 2, (size - i.height()) / 2, i, 0, 0,
                  i.width(), i.height());
  return QPixmap::fromImage(temp);
}

void Plant::SetOnUseItem(
    std::function<void(ObjectCategory, uint16_t, uint32_t, uint8_t)> callback) {
  on_use_item_ = callback;
}
