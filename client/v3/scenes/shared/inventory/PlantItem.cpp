// Copyright 2021 <CatchChallenger>
#include "PlantItem.hpp"

#include <math.h>

#include <QPainter>
#include <iostream>

#include "../../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/Label.hpp"

using Scenes::PlantItem;

PlantItem::PlantItem(uint16_t plant_id, uint32_t quantity)
    : UI::SelectableItem() {
  plant_id_ = plant_id;
  quantity_ = quantity;
  SetSize(200, 60);
}

PlantItem::~PlantItem() {}

PlantItem *PlantItem::Create(uint16_t plant_id, uint32_t quantity) {
  return new (std::nothrow) PlantItem(plant_id, quantity);
}

void PlantItem::DrawContent(QPainter *painter) {
  auto icon = Sprite::Create();
  auto text = UI::Label::Create();

  if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(
          plant_id_) !=
      QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend()) {
    const DatapackClientLoader::ItemExtra &itemExtra =
        QtDatapackClientLoader::GetInstance()->get_itemsExtra().at(plant_id_);
    QPixmap p =
        QtDatapackClientLoader::GetInstance()->getItemExtra(plant_id_).image;
    p = p.scaledToWidth(p.width() * 2);
    icon->SetPixmap(p);
    icon->SetPos(10, Height() / 2 - icon->Height() / 2);
    icon->Render(painter);

    text->SetPixelSize(12);
    text->SetText(QString::fromStdString(itemExtra.name) + "\n" +
                  QObject::tr("Quantity: ") + QString::number(quantity_) + " ");
    text->SetPos(Width() / 2 - text->Width() / 2, 5);
    text->Render(painter);
  } else {
    icon->SetPixmap(
        QtDatapackClientLoader::GetInstance()->defaultInventoryImage());
    icon->SetPos(10, 10);
    icon->Render(painter);

    text->SetPixelSize(12);
    text->SetPos(20, 10);
    if (quantity_ > 1)
      text->SetText(
          QStringLiteral("id: %1 (x%2)").arg(plant_id_).arg(quantity_));
    else
      text->SetText(QStringLiteral("id: %1").arg(plant_id_));
    text->Render(painter);
  }

  delete icon;
  delete text;
}

void PlantItem::OnResize() { ReDraw(); }

uint16_t PlantItem::PlantId() { return plant_id_; }
