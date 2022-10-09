// Copyright 2021 CatchChallenger
#include "OverMap.hpp"

#include <QDesktopServices>
#include <iostream>
#include <utility>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libcatchchallenger/ChatParsing.hpp"
#include "../../Constants.hpp"

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
  unsigned int space = Constants::ItemMediumSpacing();
  auto text_size = Constants::TextMediumSize();

  player_portrait_->SetPos(space, space);
  monster_thumb_->SetPos(player_portrait_->Right(), space);

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
    buy->SetSize(UI::Button::kRoundMedium);
    buyOver->SetVisible(physicalDpiX < 200);

    unsigned int buyX = xRight - buy->Width();
    buy->SetPos(buyX, bounding_rect_.height() - space - buy->Height());
    xRight -= buy->Width() + space;
    if (buyOver->IsVisible()) {
      buyOver->SetPixelSize(text_size);
      buyOver->SetPos(buyX + buy->Width() / 2 - buyOver->Width() / 2,
                      bounding_rect_.height() - space - buyOver->Height());
    }
  }

  {
    bagOver->SetVisible(physicalDpiX < 200);
    bag->SetSize(UI::Button::kRoundMedium);
    crafting_btn_->SetSize(UI::Button::kRoundMedium);

    unsigned int bagX = xRight - bag->Width();
    bag->SetPos(bagX, bounding_rect_.height() - space - bag->Height());
    xRight -= bag->Width() + space;
    if (bagOver->IsVisible()) {
      bagOver->SetPixelSize(text_size);
      bagOver->SetPos(bagX + bag->Width() / 2 - bagOver->Width() / 2,
                      bounding_rect_.height() - space - bagOver->Height());
    }

    crafting_btn_->SetPos(xRight - crafting_btn_->Width(),
                      bounding_rect_.height() - space - crafting_btn_->Height());
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
