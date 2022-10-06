// Copyright 2022 CatchChallenger
#include "NpcTalk.hpp"

using Scenes::NpcTalk;

void NpcTalk::Draw(QPainter *painter) {
  UI::Dialog::Draw(painter);
  auto inner = ContentPlainBoundary();
  auto b2 = Sprite::Create(":/CC/images/interface/b2.png");
  b2->Strech(24, 22, 20, 150, 150);
  b2->SetPos(inner.x(), inner.y());
  b2->Render(painter);

  int x = b2->X() + b2->Width() + 20;

  auto label = UI::Label::Create(QColor(255, 255, 255), QColor(30, 25, 17));
  label->SetText(title_);
  label->SetPos(x, inner.y());
  label->Render(painter);

  int y = label->Y() + label->Height();
  delete label;

  content_->SetY(y);
}

void NpcTalk::OnScreenResize() {
  SetDialogSize(500, 400);

  UI::Dialog::OnScreenResize();

  auto content = ContentPlainBoundary();

  content_->SetX(content.x() + 170);
  content_->SetSize(content.width() - 170, 300);
}
