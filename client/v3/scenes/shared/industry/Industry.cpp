// Copyright 2021 CatchChallenger
#include "Industry.hpp"

#include <QDateTime>
#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../../general/base/FacilityLib.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../Ultimate.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"
#include "../../../ui/ThemedButton.hpp"

using CatchChallenger::CommonDatapack;
using CatchChallenger::FacilityLib;
using Scenes::Industry;
using Scenes::IndustryItem;
using std::placeholders::_1;
using std::placeholders::_2;

Industry::Industry() {
  SetDialogSize(1200, 600);
  selected_product_ = nullptr;
  selected_resource_ = nullptr;

  products_title_ = UI::Label::Create(this);
  resources_title_ = UI::Label::Create(this);

  products_content_ = UI::ListView::Create(this);
  resources_content_ = UI::ListView::Create(this);

  products_buy_ = UI::YellowButton::Create(this);
  products_buy_->SetOnClick(std::bind(&Industry::OnBuyClick, this));
  resources_sell_ = UI::YellowButton::Create(this);
  resources_sell_->SetOnClick(std::bind(&Industry::OnSellClick, this));

  factoryInProduction = false;

  npc_icon_ = Sprite::Create(this);
  TranslateLabels();
}

Industry::~Industry() {}

Industry *Industry::Create() { return new (std::nothrow) Industry(); }

void Industry::TranslateLabels() {
  SetTitle(tr("Industry"));
  products_title_->SetText(tr("Products"));
  resources_title_->SetText(tr("Resources"));
  products_buy_->SetText(tr("Buy"));
  resources_sell_->SetText(tr("Sell"));
}

void Industry::OnEnter() {
  UI::Dialog::OnEnter();
  products_content_->RegisterEvents();
  resources_content_->RegisterEvents();
  resources_sell_->RegisterEvents();
  products_buy_->RegisterEvents();

  auto client = ConnectionManager::GetInstance()->client;

  if (!connect(client, &CatchChallenger::Api_client_real::QthaveFactoryList,
               this, &Industry::HaveFactoryListSlot))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QthaveSellFactoryObject, this,
               &Industry::HaveSellFactoryObjectSlot))
    abort();
  if (!connect(client,
               &CatchChallenger::Api_client_real::QthaveBuyFactoryObject, this,
               &Industry::HaveBuyFactoryObjectSlot))
    abort();

  OnScreenResize();
}

void Industry::OnExit() {
  products_content_->UnRegisterEvents();
  resources_content_->UnRegisterEvents();
  resources_sell_->UnRegisterEvents();
  products_buy_->UnRegisterEvents();

  auto client = ConnectionManager::GetInstance()->client;
  disconnect(client, &CatchChallenger::Api_client_real::QthaveFactoryList, this,
             &Industry::HaveFactoryListSlot);
  disconnect(client, &CatchChallenger::Api_client_real::QthaveSellFactoryObject,
             this, &Industry::HaveSellFactoryObjectSlot);
  disconnect(client, &CatchChallenger::Api_client_real::QthaveBuyFactoryObject,
             this, &Industry::HaveBuyFactoryObjectSlot);

  UI::Dialog::OnExit();
}

void Industry::OnBuyClick() {
  auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  auto player_info = PlayerInfo::GetInstance();
  auto item = static_cast<IndustryItem *>(selected_product_);

  uint16_t id = item->ObjectId();
  uint32_t price = item->Price();
  uint32_t quantity = item->Quantity();
  if (info.cash < price) {
    Globals::GetAlertDialog()->Show(tr("No cash"),
                                    tr("No cash to buy this item"));
    return;
  }
  if (!player_info->HaveReputationRequirements(
          CommonDatapack::commonDatapack.get_industriesLink()
              .at(industry_id_)
              .requirements.reputation)) {
    Globals::GetAlertDialog()->Show(tr("No requirements"),
                                    tr("You don't meet the requirements"));
    return;
  }
  int quantityToBuy = 1;
  if (info.cash >= (price * 2) && quantity > 1) {
    bool ok;
    // quantityToBuy = QInputDialog::getInt(this, tr("Buy"),tr("Amount %1 to
    // buy:")
    //.arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(id).name)),
    // 0, 0, static_cast<int>(quantity),
    // 1, &ok); if(!ok || quantityToBuy<=0) return;
  }
  quantity -= quantityToBuy;
  item->SetQuantity(quantity);
  if (quantity < 1) {
    products_content_->RemoveItem(item);
    selected_product_ = nullptr;
  } else {
    const CatchChallenger::Industry &industry =
        CommonDatapack::commonDatapack.get_industries().at(
            CommonDatapack::commonDatapack.get_industriesLink()
                .at(industry_id_)
                .industry);
    unsigned int index = 0;
    while (index < industry.resources.size()) {
      const CatchChallenger::Industry::Product &product =
          industry.products.at(index);
      if (product.item == id) {
        item->SetPrice(FacilityLib::getFactoryProductPrice(
            quantity, product,
            CommonDatapack::commonDatapack.get_industries().at((
                CommonDatapack::commonDatapack.get_industriesLink()
                    .at(industry_id_)
                    .industry))));
        break;
      }
      index++;
    }
  }
  products_content_->ReDraw();
  CatchChallenger::ItemToSellOrBuy itemToSellOrBuy;
  itemToSellOrBuy.quantity = quantityToBuy;
  itemToSellOrBuy.object = id;
  itemToSellOrBuy.price = quantityToBuy * price;
  items_bought_.push_back(itemToSellOrBuy);
  player_info->AddCash(-itemToSellOrBuy.price);
  ConnectionManager::GetInstance()->client->buyFactoryProduct(
      industry_id_, id, quantityToBuy, price);
  player_info->AppendReputationRewards(
      CommonDatapack::commonDatapack.get_industriesLink()
          .at(industry_id_)
          .rewards.reputation);
}

void Industry::OnSellClick() {
  if (selected_resource_ == nullptr) return;
  auto tmp = static_cast<IndustryItem *>(selected_resource_);
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  uint16_t itemid = tmp->ObjectId();
  uint32_t price = tmp->Price();
  uint32_t quantity = tmp->Quantity();
  if (info.items.find(itemid) == info.items.cend()) {
    Globals::GetAlertDialog()->Show(tr("No item"),
                                    tr("You have not the item to sell"));
    return;
  }
  if (!PlayerInfo::GetInstance()->HaveReputationRequirements(
          CommonDatapack::commonDatapack.get_industriesLink()
              .at(industry_id_)
              .requirements.reputation)) {
    Globals::GetAlertDialog()->Show(tr("No requirements"),
                                    tr("You don't meet the requirements"));
    return;
  }
  int i = 1;
  if (info.items.at(itemid) > 1 && quantity > 1) {
    uint32_t quantityToSell = quantity;
    if (info.items.at(itemid) < quantityToSell)
      quantityToSell = info.items.at(itemid);
    bool ok;
    // Agregar en un rato
    // i = QInputDialog::getInt(
    // this, tr("Sell"),
    // tr("Amount %1 to sell:")
    //.arg(QString::fromStdString(
    // QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemid)
    //.name)),
    // 0, 0, quantityToSell, 1, &ok);
    // if (!ok || i <= 0) return;
  }
  quantity -= i;
  tmp->SetQuantity(quantity);
  if (quantity < 1) {
    resources_content_->RemoveItem(tmp);
  } else {
    const CatchChallenger::Industry &industry =
        CommonDatapack::commonDatapack.get_industries().at(
            CommonDatapack::commonDatapack.get_industriesLink()
                .at(industry_id_)
                .industry);
    unsigned int index = 0;
    while (index < industry.resources.size()) {
      const CatchChallenger::Industry::Resource &resource =
          industry.resources.at(index);
      if (resource.item == itemid) {
        tmp->SetPrice(FacilityLib::getFactoryResourcePrice(
            resource.quantity * industry.cycletobefull - quantity, resource,
            CommonDatapack::commonDatapack.get_industries().at(
                CommonDatapack::commonDatapack.get_industriesLink()
                    .at(industry_id_)
                    .industry)));
        break;
      }
      index++;
    }
  }
  resources_content_->ReDraw();
  CatchChallenger::ItemToSellOrBuy tempItem;
  tempItem.object = itemid;
  tempItem.quantity = i;
  tempItem.price = price;
  items_sold_.push_back(tempItem);

  auto player_info = PlayerInfo::GetInstance();
  player_info->RemoveInventory(itemid, i);
  ConnectionManager::GetInstance()->client->sellFactoryResource(
      industry_id_, itemid, i, price);
  player_info->AppendReputationRewards(
      CommonDatapack::commonDatapack.get_industriesLink()
          .at(industry_id_)
          .rewards.reputation);
}

void Industry::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentBoundary();
  qreal content_width = (content.width() / 2) - 10;
  qreal block_1 = content.x();
  qreal block_2 = content.x() + content.width() - content_width;

  products_title_->SetAlignment(Qt::AlignCenter);
  products_title_->SetPixelSize(14);
  products_title_->SetWidth(content_width);
  products_title_->SetPos(block_1, content.y());

  resources_title_->SetAlignment(Qt::AlignCenter);
  resources_title_->SetPixelSize(14);
  resources_title_->SetWidth(content_width);
  resources_title_->SetPos(block_2, content.y());

  products_content_->SetPos(block_1,
                            content.y() + products_title_->Height() + 5);
  products_content_->SetSize(content_width,
                             content.height() - products_title_->Height() - 25);
  products_content_->SetItemSpacing(5);

  resources_content_->SetPos(block_2, products_content_->Y());
  resources_content_->SetSize(content_width, products_content_->Height());
  resources_content_->SetItemSpacing(5);

  products_buy_->SetSize(150, 40);
  products_buy_->SetPixelSize(14);
  products_buy_->SetPos(
      block_1 + content_width / 2 - products_buy_->Width() / 2,
      products_content_->Bottom() + 5);

  resources_sell_->SetSize(150, 40);
  resources_sell_->SetPixelSize(14);
  resources_sell_->SetPos(
      block_2 + content_width / 2 - resources_sell_->Width() / 2,
      resources_content_->Bottom() + 5);
}

void Industry::OnResourceClick(Node *node) {
  selected_resource_ = node;
  auto items = resources_content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<IndustryItem *>(item)->SetSelected(false);
  }
}

void Industry::OnProductClick(Node *node) {
  selected_product_ = node;
  auto items = products_content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    static_cast<IndustryItem *>(item)->SetSelected(false);
  }
}

void Industry::UpdateContent() {
  products_content_->Clear();
  resources_content_->Clear();
}

void Industry::SetBot(const CatchChallenger::Bot &bot, uint16_t industry_id) {
  industry_id_ = industry_id;
  if (bot.properties.find("skin") != bot.properties.cend()) {
    QPixmap skin = QString::fromStdString(
        QtDatapackClientLoader::datapackLoader->getFrontSkinPath(
            std::stoi(bot.properties.at("skin"))));
    if (!skin.isNull()) {
      npc_icon_->SetPixmap(skin.scaled(80, 80));
      npc_icon_->SetVisible(true);
    } else {
      npc_icon_->SetVisible(false);
    }
  } else {
    npc_icon_->SetVisible(false);
  }
  ConnectionManager::GetInstance()->client->getFactoryList(industry_id);
}

void Industry::HaveFactoryListSlot(
    const uint32_t &remainingProductionTime,
    const std::vector<CatchChallenger::ItemToSellOrBuy> &resources,
    const std::vector<CatchChallenger::ItemToSellOrBuy> &products) {
  const auto &player_info = PlayerInfo::GetInstance();
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();

  products_content_->Clear();
  resources_content_->Clear();

  industryStatus.products.clear();
  industryStatus.resources.clear();
  const CatchChallenger::Industry &industry =
      CatchChallenger::CommonDatapack::commonDatapack.get_industries().at(
          CatchChallenger::CommonDatapack::commonDatapack.get_industriesLink()
              .at(industry_id_)
              .industry);
  industryStatus.last_update = QDateTime::currentMSecsSinceEpoch() / 1000 +
                               remainingProductionTime - industry.time;
  unsigned int index = 0;
  while (index < resources.size()) {
    unsigned int sub_index = 0;
    while (sub_index < industry.resources.size()) {
      const CatchChallenger::Industry::Resource &resource =
          industry.resources.at(sub_index);
      if (resource.item == resources.at(index).object) {
        industryStatus.resources[resources.at(index).object] =
            industry.cycletobefull * resource.quantity -
            resources.at(index).quantity;
        break;
      }
      sub_index++;
    }
    if (sub_index == industry.resources.size()) {
      std::cerr << "sub_index==industry.resources.size()" << std::endl;
    }
    auto *item = IndustryItem::Create(resources.at(index));
    item->SetOnClick(
        std::bind(&Industry::OnResourceClick, this, std::placeholders::_1));
    if (info.items.find(resources.at(index).object) == info.items.cend() ||
        !player_info->HaveReputationRequirements(
            CommonDatapack::commonDatapack.get_industriesLink()
                .at(industry_id_)
                .requirements.reputation)) {
      item->SetDisabled(true);
    }
    resources_content_->AddItem(item);
    index++;
  }
  {
    unsigned int sub_index = 0;
    while (sub_index < industry.resources.size()) {
      const CatchChallenger::Industry::Resource &resource =
          industry.resources.at(sub_index);
      if (industryStatus.resources.find(resource.item) ==
          industryStatus.resources.cend()) {
        std::cerr << "Ressource " << resource.item
                  << " not returned, consider as full: "
                  << industry.cycletobefull * resource.quantity << std::endl;
        industryStatus.resources[resource.item] =
            industry.cycletobefull * resource.quantity;
      }
      sub_index++;
    }
  }

  index = 0;
  while (index < products.size()) {
    industryStatus.products[products.at(index).object] =
        products.at(index).quantity;
    auto *item = IndustryItem::Create(products.at(index));
    item->SetOnClick(
        std::bind(&Industry::OnProductClick, this, std::placeholders::_1));
    if (products.at(index).price > info.cash) {
      item->SetDisabled(true);
    }
    products_content_->AddItem(item);
    index++;
  }
  // ui->factoryStatus->setText(tr("Have the factory list"));
  UpdateFactoryStatProduction(industryStatus, industry);
}

void Industry::HaveBuyFactoryObjectSlot(const CatchChallenger::BuyStat &stat,
                                        const uint32_t &newPrice) {
  auto player_info = PlayerInfo::GetInstance();
  const CatchChallenger::ItemToSellOrBuy &itemToSellOrBuy =
      items_bought_.front();
  std::unordered_map<uint16_t, uint32_t> items;
  switch (stat) {
    case CatchChallenger::BuyStat_Done:
      items[itemToSellOrBuy.object] = itemToSellOrBuy.quantity;
      if (industryStatus.products.find(itemToSellOrBuy.object) !=
          industryStatus.products.cend()) {
        industryStatus.products[itemToSellOrBuy.object] -=
            itemToSellOrBuy.quantity;
        if (industryStatus.products.at(itemToSellOrBuy.object) == 0)
          industryStatus.products.erase(itemToSellOrBuy.object);
      }
      player_info->AddInventory(items);
      break;
    case CatchChallenger::BuyStat_BetterPrice:
      if (newPrice == 0) {
        qDebug() << "haveBuyFactoryObject() Can't buy at 0$!";
        return;
      }
      player_info->AddCash(itemToSellOrBuy.price);
      player_info->AddCash(newPrice * -itemToSellOrBuy.quantity);
      items[itemToSellOrBuy.object] = itemToSellOrBuy.quantity;
      if (industryStatus.products.find(itemToSellOrBuy.object) !=
          industryStatus.products.cend()) {
        industryStatus.products[itemToSellOrBuy.object] -=
            itemToSellOrBuy.quantity;
        if (industryStatus.products.at(itemToSellOrBuy.object) == 0)
          industryStatus.products.erase(itemToSellOrBuy.object);
      }
      player_info->AddInventory(items);
      break;
    case CatchChallenger::BuyStat_HaveNotQuantity:
      player_info->AddCash(itemToSellOrBuy.price);
      // showTip(tr("Sorry but have not the quantity of this
      // item").toStdString());
      break;
    case CatchChallenger::BuyStat_PriceHaveChanged:
      player_info->AddCash(itemToSellOrBuy.price);
      // showTip(tr("Sorry but now the price is worse").toStdString());
      break;
    default:
      qDebug() << "haveBuyFactoryObject(stat) have unknow value";
      break;
  }
  switch (stat) {
    case CatchChallenger::BuyStat_Done:
    case CatchChallenger::BuyStat_BetterPrice: {
      if (factoryInProduction) break;
      const CatchChallenger::Industry &industry =
          CatchChallenger::CommonDatapack::commonDatapack.get_industries().at(
              CatchChallenger::CommonDatapack::commonDatapack
                  .get_industriesLink()
                  .at(industry_id_)
                  .industry);
      industryStatus.last_update = QDateTime::currentMSecsSinceEpoch() / 1000;
      UpdateFactoryStatProduction(industryStatus, industry);
    } break;
    default:
      break;
  }
  player_info->Notify();
  items_bought_.erase(items_bought_.cbegin());
}

void Industry::HaveSellFactoryObjectSlot(const CatchChallenger::SoldStat &stat,
                                         const uint32_t &newPrice) {
  auto info = PlayerInfo::GetInstance();
  switch (stat) {
    case CatchChallenger::SoldStat_Done:
      if (industryStatus.resources.find(items_sold_.front().object) ==
          industryStatus.resources.cend())
        industryStatus.resources[items_sold_.front().object] = 0;
      industryStatus.resources[items_sold_.front().object] +=
          items_sold_.front().quantity;
      info->AddCash(items_sold_.front().price * items_sold_.front().quantity);
      // showtip(tr("item sold").tostdstring());
      break;
    case CatchChallenger::SoldStat_BetterPrice:
      if (newPrice == 0) {
        std::cout
            << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << "havesellfactoryobject() the price 0$ can't be better price!"
            << std::endl;
        // showtip(tr("bug into returned price").tostdstring());
        return;
      }
      if (industryStatus.resources.find(items_sold_.front().object) ==
          industryStatus.resources.cend())
        industryStatus.resources[items_sold_.front().object] = 0;
      industryStatus.resources[items_sold_.front().object] +=
          items_sold_.front().quantity;
      info->AddCash(newPrice * items_sold_.front().quantity);
      // showtip(tr("item sold at better price").tostdstring());
      break;
    case CatchChallenger::SoldStat_WrongQuantity:
      info->AddInventory(items_sold_.front().object,
                         items_sold_.front().quantity);
      // showtip(tr("sorry but have not the quantity of this
      // item").tostdstring());
      break;
    case CatchChallenger::SoldStat_PriceHaveChanged:
      info->AddInventory(items_sold_.front().object,
                         items_sold_.front().quantity);
      // showtip(tr("sorry but now the price is worse").tostdstring());
      break;
    default:
      std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
                << "havesellfactoryobject(stat) have unknow value" << std::endl;
      break;
  }
  items_sold_.erase(items_sold_.cbegin());
  switch (stat) {
    case CatchChallenger::SoldStat_Done:
    case CatchChallenger::SoldStat_BetterPrice: {
      if (factoryInProduction) break;
      const CatchChallenger::IndustryLink &industrylink =
          CatchChallenger::CommonDatapack::commonDatapack.get_industriesLink()
              .at(industry_id_);
      const CatchChallenger::Industry &industry =
          CatchChallenger::CommonDatapack::commonDatapack.get_industries().at(
              industrylink.industry);
      industryStatus.last_update = QDateTime::currentMSecsSinceEpoch() / 1000;
      UpdateFactoryStatProduction(industryStatus, industry);
    } break;
    default:
      break;
  }
  info->Notify();
}

void Industry::UpdateFactoryStatProduction(
    const CatchChallenger::IndustryStatus &industryStatus,
    const CatchChallenger::Industry &industry) {
  if (FacilityLib::factoryProductionStarted(industryStatus, industry)) {
    factoryInProduction = true;
    if (Ultimate::ultimate.isUltimate()) {
      std::string productionTime;
      uint64_t remainingProductionTime = 0;
      if ((uint64_t)(industryStatus.last_update + industry.time) >
          (uint64_t)(QDateTime::currentMSecsSinceEpoch() / 1000))
        remainingProductionTime =
            static_cast<uint32_t>((industryStatus.last_update + industry.time) -
                                  (QDateTime::currentMSecsSinceEpoch() / 1000));
      else if ((uint64_t)(industryStatus.last_update + industry.time) <
               (uint64_t)(QDateTime::currentMSecsSinceEpoch() / 1000))
        remainingProductionTime =
            ((QDateTime::currentMSecsSinceEpoch() / 1000) -
             industryStatus.last_update) %
            industry.time;
      else
        remainingProductionTime = industry.time;
      if (remainingProductionTime > 0) {
        productionTime = tr("Remaining time:").toStdString() + "<br />";
        if (remainingProductionTime < 60)
          productionTime += tr("Less than a minute").toStdString();
        else
          productionTime +=
              tr("%n minute(s)", "",
                 static_cast<uint32_t>(remainingProductionTime / 60))
                  .toStdString();
      }
          //ui->factoryStatText->setText(tr("In production")+"<br/>"+QString::fromStdString(productionTime));
    } else {
      //ui->factoryStatText->setText(tr("In production"));
    }
  } else {
    factoryInProduction = false;
    //ui->factoryStatText->setText(tr("Production stopped"));
  }
}
