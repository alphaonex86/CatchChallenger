// Copyright 2021 CatchChallenger
#include "OverMap.hpp"

#include <QDesktopServices>
#include <iostream>
#include <utility>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libcatchchallenger/ChatParsing.hpp"

using Scenes::OverMap;

using std::placeholders::_1;

OverMap::OverMap() {
  connexionManager = ConnectionManager::GetInstance();
  monster_thumb_ = MonsterThumb::Create(this);

  player_portrait_ = PlayerPortrait::Create(this);
  player_portrait_->SetPos(0, 0);

  playersCountBack =
      Sprite::Create(":/CC/images/interface/multicount.png", this);
  playersCount = UI::Label::Create(this);

  bag = UI::Button::Create(":/CC/images/interface/bag.png", this);
  bagOver = UI::Label::Create(QColor(255, 255, 255), QColor(80, 71, 38), this);
  buy = UI::Button::Create(":/CC/images/interface/buy.png", this);
  {
    QLinearGradient gradient1(0, 0, 0, 100);
    gradient1.setColorAt(0, QColor(0, 0, 0));
    gradient1.setColorAt(1, QColor(0, 0, 0));
    QLinearGradient gradient2(0, 0, 0, 100);
    gradient2.setColorAt(0, QColor(255, 255, 255));
    gradient2.setColorAt(0.5, QColor(178, 242, 241));
    buyOver = UI::Label::Create(gradient1, gradient2, this);
  }

  crafting_btn_ = UI::Button::Create(":/CC/images/interface/gift.png", this);

  toast_ = Toast::Create(this);
  toast_->SetVisible(false);

  npc_talk_ = NpcTalk::Create();
  chat_ = Chat::Create(this);

  tip_ = Tip::Create(this);
  tip_->SetVisible(false);

  persistant_tipBack =
      Sprite::Create(":/CC/images/interface/chatBackground.png", this);
  persistant_tip = UI::Label::Create(persistant_tipBack);

  if (false) {
    {
      persistant_tipString = "persistant_tip";
      persistant_tip->SetText(persistant_tipString);
    }
  }

  newLanguage();

  buy->SetOnClick(std::bind(&OverMap::buyClicked, this));
}

OverMap::~OverMap() {
  delete tip_;
  tip_ = nullptr;
  delete player_portrait_;
  player_portrait_ = nullptr;
  delete chat_;
  chat_ = nullptr;
}

void OverMap::resetAll() {
  monster_thumb_->Reset();
}

void OverMap::setVar(CCMap *ccmap) {
  (void)ccmap;
  connect(connexionManager->client,
          &CatchChallenger::Api_protocol_Qt::Qtnumber_of_player, this,
          &OverMap::updatePlayerNumber, Qt::UniqueConnection);
  if (!connexionManager->client->getCaracterSelected()) {
    connect(connexionManager->client,
            &CatchChallenger::Api_protocol_Qt::Qthave_current_player_info, this,
            &OverMap::have_current_player_info, Qt::UniqueConnection);
  } else {
    const auto &info = connexionManager->client->get_player_informations_ro();
    have_current_player_info(info);
  }
}

void OverMap::have_current_player_info(
    const CatchChallenger::Player_private_and_public_informations
        &informations) {
  player_portrait_->SetVisible(true);
  player_portrait_->SetInformation(informations);

  monster_thumb_->LoadMonsters(informations);
  std::pair<uint16_t, uint16_t> t =
      connexionManager->client->getLast_number_of_player();
  playersCount->SetText(QString::number(t.first));
}

void OverMap::updatePlayerNumber(const uint16_t &number,
                                 const uint16_t &max_players) {
  Q_UNUSED(max_players);
  playersCount->SetText(QString::number(number));
}

void OverMap::newLanguage() {
  bagOver->SetText(tr("Bag"));
  buyOver->SetText(tr("Buy"));
}

void OverMap::OnScreenResize() {
  unsigned int space = 10;

  player_portrait_->SetPos(space, space);

  int tempx = player_portrait_->X() + player_portrait_->Width();

  monster_thumb_->SetPos(tempx, space);

  playersCountBack->SetPos(
      bounding_rect_.width() - space - playersCountBack->Width(), space);
  playersCount->SetPos(bounding_rect_.width() - space - 80, space + 15);

  // TODO(lanstat): verificar como obtener este valor
  // int physicalDpiX = w->physicalDpiX();
  int physicalDpiX = 300;

  unsigned int xLeft = space;

  // compose from right to left
  unsigned int xRight = bounding_rect_.width() - space;
  if (buy->IsVisible()) {
    if (bounding_rect_.width() < 800 || bounding_rect_.height() < 600) {
      buy->SetSize(84 / 2, 93 / 2);
      buyOver->SetVisible(false);
    } else {
      buy->SetSize(84, 93);
      buyOver->SetVisible(physicalDpiX < 200);
    }
    unsigned int buyX = xRight - buy->Width();
    buy->SetPos(buyX, bounding_rect_.height() - space - buy->Height());
    xRight -= buy->Width() + space;
    if (buyOver->IsVisible()) {
      buyOver->SetPixelSize(18);
      buyOver->SetPos(buyX + buy->Width() / 2 - buyOver->Width() / 2,
                      bounding_rect_.height() - space - buyOver->Height());
    }
  }
  {
    if (bounding_rect_.width() < 800 || bounding_rect_.height() < 600) {
      bag->SetSize(84 / 2, 93 / 2);
      bagOver->SetVisible(false);
      crafting_btn_->SetSize(84 / 2, 93 / 2);
    } else {
      bag->SetSize(84, 93);
      bagOver->SetVisible(physicalDpiX < 200);
      crafting_btn_->SetSize(84, 93);
    }
    unsigned int bagX = xRight - bag->Width();
    bag->SetPos(bagX, bounding_rect_.height() - space - bag->Height());
    xRight -= bag->Width() + space;
    if (bagOver->IsVisible()) {
      bagOver->SetPixelSize(18);
      bagOver->SetPos(bagX + bag->Width() / 2 - bagOver->Width() / 2,
                      bounding_rect_.height() - space - bagOver->Height());
    }

    crafting_btn_->SetPos(xRight - crafting_btn_->Width(),
                      bounding_rect_.height() - space - crafting_btn_->Height());
  }
  Q_UNUSED(xRight);

  int yMiddle = bounding_rect_.height() - space;

  persistant_tipBack->SetVisible(!persistant_tipString.isEmpty());
  persistant_tip->SetVisible(persistant_tipBack->IsVisible());
  if (persistant_tip->IsVisible()) {
    const QRectF &f = persistant_tip->BoundingRect();
    persistant_tipBack->SetSize(f.width() + space * 2, f.height() + space * 2);
    persistant_tip->SetPos(space, space);
    persistant_tipBack->SetPos(
        bounding_rect_.width() / 2 - persistant_tipBack->Width() / 2,
        yMiddle - persistant_tipBack->Height());
    yMiddle -= (f.height() + space * 2 + space);
  }
  tip_->SetY(150);
  toast_->SetY(150);

  chat_->SetPos(10, Height() - 210);
  chat_->SetSize(400, 200);
}

void OverMap::OnEnter() {
  Scene::OnEnter();
  buy->RegisterEvents();
  tip_->RegisterEvents();
  toast_->RegisterEvents();
  chat_->RegisterEvents();
  monster_thumb_->RegisterEvents();
  
  if (player_portrait_->IsVisible()) {
    have_current_player_info(PlayerInfo::GetInstance()->GetInformationRO());
  }
}

void OverMap::OnExit() {
  buy->UnRegisterEvents();
  tip_->UnRegisterEvents();
  toast_->UnRegisterEvents();
  chat_->UnRegisterEvents();
  monster_thumb_->UnRegisterEvents();
}

void OverMap::buyClicked() {
  QDesktopServices::openUrl(tr("https://shop.first-world.info/en/") +
                            "#CatchChallenger");
}
