// Copyright 2021 CatchChallenger
#include "MultiItem.hpp"

#include <QPainter>
#include <iostream>

#include "../../Constants.hpp"
#include "../../base/ConnectionInfo.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"

using Scenes::MultiItem;

MultiItem::MultiItem(const ConnectionInfo &connexionInfo)
    : UI::SelectableItem(), m_connexionInfo(connexionInfo) {
  SetSize(200, Constants::ItemMediumHeight());
  FetchName();
}

MultiItem::~MultiItem() {}

MultiItem *MultiItem::Create(const ConnectionInfo &connexionInfo) {
  return new (std::nothrow) MultiItem(connexionInfo);
}

void MultiItem::FetchName() {
  QString connexionInfoLabel;
  if (!m_connexionInfo.name.isEmpty()) {
    connexionInfoLabel = m_connexionInfo.name;
  } else {
#ifdef NOTCPSOCKET
    connexionInfoLabel = m_connexionInfo.ws;
#else
#ifdef NOWEBSOCKET
    connexionInfoLabel = m_connexionInfo.host;
#else
    if (!m_connexionInfo.host.isEmpty())
      connexionInfoLabel = m_connexionInfo.host;
    else
      connexionInfoLabel = m_connexionInfo.ws;
#endif
#endif
    if (connexionInfoLabel.size() > 32)
      connexionInfoLabel =
          connexionInfoLabel.left(15) + "..." + connexionInfoLabel.right(15);
  }
  name_ = connexionInfoLabel;
  ReDrawContent();
}

void MultiItem::DrawContent(QPainter *painter) {
  if (Width() == 0 || Height() == 0) return;

  auto text = UI::Label::Create(QColor(255, 255, 255), QColor(30, 25, 17));
  text->SetWidth(bounding_rect_.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(name_);
  text->SetY(Height() / 2 - text->Height() / 2);

  if (m_connexionInfo.connexionCounter > 0) {
    const QPixmap &p = *AssetsLoader::GetInstance()->GetImage(
        ":/CC/images/interface/star.png");
    const int size = Height() * 0.6;
    painter->drawPixmap(30, Height() / 2 - size / 2, size, size, p);
  }
  text->Render(painter);

  delete text;
}

ConnectionInfo MultiItem::Connection() const { return m_connexionInfo; }
