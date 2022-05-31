// Copyright 2021 CatchChallenger
#include "Shop.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../Globals.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"
#include "ShopItem.hpp"

using Scenes::Shop;
using std::placeholders::_1;
using std::placeholders::_2;

Shop::Shop() {
  on_transaction_finish_ = nullptr;
  selected_item_ = nullptr;
  SetDialogSize(600, 500);
  portrait_ = Sprite::Create(this);
  seller_name_ = UI::Label::Create(this);
  buy_ = UI::Button::Create(":/CC/images/interface/buy.png", this);
  buy_->SetOnClick(std::bind(&Shop::OnActionClick, this, _1));
  content_ = UI::ListView::Create(this);
  connexionManager = ConnectionManager::GetInstance();

  if (!QObject::connect(connexionManager->client,
                        &CatchChallenger::Api_client_real::QthaveShopList,
                        std::bind(&Shop::HaveShopList, this, _1)))
    abort();
  if (!QObject::connect(connexionManager->client,
                        &CatchChallenger::Api_client_real::QthaveSellObject,
                        std::bind(&Shop::HaveSellObject, this, _1, _2)))
    abort();
  if (!QObject::connect(connexionManager->client,
                        &CatchChallenger::Api_client_real::QthaveBuyObject,
                        std::bind(&Shop::HaveBuyObject, this, _1, _2)))
    abort();

  is_buy_mode_ = true;

  AddActionButton(buy_);
}

Shop::~Shop() {
  delete portrait_;
  portrait_ = nullptr;
  delete content_;
  content_ = nullptr;
}

Shop *Shop::Create() { return new (std::nothrow) Shop(); }

void Shop::Draw(QPainter *painter) {
  UI::Dialog::Draw(painter);
  auto inner = ContentBoundary();
  auto b2 = Sprite::Create(":/CC/images/interface/b2.png");
  b2->Strech(24, 22, 20, 160, 160);
  b2->SetPos(inner.x(), inner.y());
  b2->Render(painter);

  int x = b2->X() + b2->Width() + 20;

  b2->Strech(24, 22, 20, inner.width() + inner.x() - x, inner.height());
  b2->SetPos(x, inner.y());
  b2->Render(painter);

  delete b2;
}

void Shop::OnEnter() {
  UI::Dialog::OnEnter();
  content_->RegisterEvents();
}

void Shop::OnExit() {
  content_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void Shop::SetSeller(const CatchChallenger::Bot &bot, uint16_t shop_id) {
  shop_id_ = shop_id;
  SetTitle(tr("Buy"));
  is_buy_mode_ = true;
  content_->Clear();
  if (bot.properties.find("skin") != bot.properties.cend()) {
    QPixmap skin(QString::fromStdString(
        QtDatapackClientLoader::GetInstance()->getSkinPath(
            bot.properties.at("skin"), "front")));
    if (!skin.isNull()) {
      portrait_->SetPixmap(skin.scaled(160, 160));
      portrait_->SetVisible(true);
    } else {
      portrait_->SetVisible(false);
    }
  } else {
    portrait_->SetVisible(false);
  }
  content_->Clear();
#ifndef CATCHCHALLENGER_CLIENT_INSTANT_SHOP
  seller_name_->SetText(tr("Waiting the shop content"));
  connexionManager->client->getShopList(shop_id);
#else
  {
    // std::vector<ItemToSellOrBuy> items;
    const auto &shop =
        CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId);
    std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
              << (int)shop.size() << std::endl;
    // unsigned int index = 0;
    // while (index < shop.items.size()) {
    // if (shop.prices.at(index) > 0) {
    // ItemToSellOrBuy newItem;
    // newItem.object = shop.items.at(index);
    // newItem.price = shop.prices.at(index);
    // newItem.quantity = 0;
    // items << newItem;
    //}
    // index++;
    //}
    // haveShopList(items);
  }
#endif
}

void Shop::SetMeAsSeller(uint16_t shop_id) {
  shop_id_ = shop_id;
  content_->Clear();
  SetTitle(tr("Sell"));
  is_buy_mode_ = false;
  const CatchChallenger::Player_private_and_public_informations &player_info =
      connexionManager->client->get_player_informations_ro();
  auto i = player_info.items.begin();
  while (i != player_info.items.cend()) {
    if (QtDatapackClientLoader::GetInstance()->get_itemsExtra().find(i->first) !=
            QtDatapackClientLoader::GetInstance()->get_itemsExtra().cend() &&
        CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(i->first)
                .price > 0) {
      int price = CatchChallenger::CommonDatapack::commonDatapack.get_items().item
                      .at(i->first)
                      .price /
                  2;
      auto node = ShopItem::Create(i->first, i->second, price);

      node->SetOnClick(std::bind(&Shop::OnItemClick, this, _1));
      content_->AddItem(node);
    }
    ++i;
  }
}

void Shop::OnActionClick(Node *node) {
  if (node == buy_) {
    if (selected_item_ != nullptr) {
      ShopItem *item = static_cast<ShopItem *>(selected_item_);
      if (is_buy_mode_) {
        connexionManager->client->buyObject(shop_id_, item->ObjectId(), 1,
                                            item->Price());
      } else {
        connexionManager->client->sellObject(shop_id_, item->ObjectId(), 1,
                                             item->Price());
      }
    }
  }
}

void Shop::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();

  content_->SetPos(content.x() + 190, content.y() + 15);
  content_->SetSize(content.width() - 200, content.height() - 25);
  content_->SetItemSpacing(5);
  portrait_->SetPos(content.x(), content.y());
  seller_name_->SetPos(content.x(), content.y() + 180);
}

void Shop::Close() { Parent()->RemoveChild(this); }

void Shop::OnItemClick(Node *node) {
  selected_item_ = node;
  auto items = content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<ShopItem *>(item)->SetSelected(false);
  }
}

void Shop::HaveShopList(
    const std::vector<CatchChallenger::ItemToSellOrBuy> &items) {
  seller_name_->SetVisible(false);

  for (auto it = begin(items); it != end(items); it++) {
    auto node = ShopItem::Create((*it).object, 99, (*it).price);

    node->SetOnClick(std::bind(&Shop::OnItemClick, this, _1));
    content_->AddItem(node);
  }
}

void Shop::HaveBuyObject(const CatchChallenger::BuyStat &stat,
                         const uint32_t &new_price) {
  QString message;
  ShopItem *item = static_cast<ShopItem *>(selected_item_);
  auto info = PlayerInfo::GetInstance();
  int quantity = 1;

  switch (stat) {
    case CatchChallenger::BuyStat_Done:
      info->AddCash(new_price * quantity * -1);
      info->AddInventory(item->ObjectId(), quantity);
      message = QStringLiteral("You buy\n%1 x %2")
                    .arg(quantity)
                    .arg(QString::fromStdString(item->Name()));
      break;
    case CatchChallenger::BuyStat_BetterPrice:
      if (new_price == 0) {
        Logger::Log(QString("haveSellObject() Can't buy at 0$!"));
        return;
      }
      info->AddCash(new_price * quantity * -1);
      info->AddInventory(item->ObjectId(), quantity);
      break;
    case CatchChallenger::BuyStat_HaveNotQuantity:
      // addCash(itemToSellOrBuy.price);
      message = tr("Sorry but have not the quantity of this item");
      break;
    case CatchChallenger::BuyStat_PriceHaveChanged:
      // addCash(itemToSellOrBuy.price);
      message = tr("Sorry but now the price is worse");
      break;
    default:
      Logger::Log(QString("haveBuyObject(stat) have unknow value"));
      break;
  }

  info->Notify();
  if (on_transaction_finish_) {
    on_transaction_finish_(message.toStdString());
  }

  Close();
}

void Shop::HaveSellObject(const CatchChallenger::SoldStat &stat,
                          const uint32_t &new_price) {
  QString message;
  ShopItem *item = static_cast<ShopItem *>(selected_item_);
  auto info = PlayerInfo::GetInstance();
  int quantity = 1;

  switch (stat) {
    case CatchChallenger::SoldStat_Done:
      info->AddCash(item->Price() * quantity);
      info->RemoveInventory(item->ObjectId(), quantity);
      message = tr("Item sold");
      break;
    case CatchChallenger::SoldStat_BetterPrice:
      if (new_price == 0) {
        Logger::Log(
            QString("haveSellObject() the price 0$ can't be better price!"));
        return;
      }
      info->AddCash(new_price * quantity);
      info->RemoveInventory(item->ObjectId(), quantity);
      message = tr("Item sold at better price");
      break;
    case CatchChallenger::SoldStat_WrongQuantity:
      // add_to_inventory(itemsToSell.front().object,
      // itemsToSell.front().quantity, false);
      message = tr("Sorry but have not the quantity of this item");
      break;
    case CatchChallenger::SoldStat_PriceHaveChanged:
      // add_to_inventory(itemsToSell.front().object,
      // itemsToSell.front().quantity, false);
      message = tr("Sorry but now the price is worse");
      break;
    default:
      Logger::Log(QString("haveBuyObject(stat) have unknow value"));
      break;
  }

  info->Notify();
  if (on_transaction_finish_) {
    on_transaction_finish_(message.toStdString());
  }

  Close();
}

void Shop::SetOnTransactionFinish(std::function<void(std::string)> callback) {
  on_transaction_finish_ = callback;
}
