// Copyright 2021 <CatchChallenger>
#include "QuestItem.hpp"

#include <iostream>

#include "../../../ui/Label.hpp"

using Scenes::QuestItem;

QuestItem::QuestItem(QString text)
    : UI::SelectableItem() {
  text_ = text;
  SetSize(200, 50);
}

QuestItem::~QuestItem() {
}

QuestItem *QuestItem::Create(QString text) {
  return new (std::nothrow) QuestItem(text);
}

void QuestItem::DrawContent(QPainter *painter) {
  auto text = UI::Label::Create();
  text->SetWidth(bounding_rect_.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(text_);
  text->SetPos(0, Height() / 2 - text->Height() / 2);

  text->Render(painter);

  delete text;
}
