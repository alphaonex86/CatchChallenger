#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../../general/base/FacilityLib.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libqtcatchchallenger/Ultimate.hpp"

#include <QInputDialog>
#include <iostream>

using namespace CatchChallenger;

void BaseWindow::on_factoryBuy_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->factoryProducts->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_factoryProducts_itemActivated(selectedItems.first());
}

void BaseWindow::on_factoryProducts_itemActivated(QListWidgetItem *item)
{
    uint16_t id=static_cast<uint16_t>(item->data(99).toUInt());
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(playerInformations.cash<price)
    {
        QMessageBox::warning(this,tr("No cash"),tr("No bash to buy this item"));
        return;
    }
    if(!client->haveReputationRequirements(factoryIndustry().requirements))
    {
        QMessageBox::warning(this,tr("No requirements"),tr("You don't meet the requirements"));
        return;
    }
    int quantityToBuy=1;
    if(playerInformations.cash>=(price*2) && quantity>1)
    {
        bool ok;
        quantityToBuy = QInputDialog::getInt(this, tr("Buy"),tr("Amount %1 to buy:")
                                             .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(id).name)),
                                             0, 0, static_cast<int>(quantity), 1, &ok);
        if(!ok || quantityToBuy<=0)
            return;
    }
    quantity-=quantityToBuy;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
    {
        uint32_t newPrice=0;
        if(FactoryTrade::productPrice(factoryIndustry(),id,quantity,newPrice))
            item->setData(98,newPrice);
        factoryToProductItem(item);
    }
    removeCash(factoryTrade.buy(id,quantityToBuy,price));
    client->buyFactoryProduct(id,quantityToBuy,price);
}

void BaseWindow::on_factorySell_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->factoryResources->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_factoryResources_itemActivated(selectedItems.first());
}

void BaseWindow::on_factoryResources_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    uint16_t itemid=static_cast<uint16_t>(item->data(99).toUInt());
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();
    if(playerInformations.items.find(itemid)==playerInformations.items.cend())
    {
        QMessageBox::warning(this,tr("No item"),tr("You have not the item to sell"));
        return;
    }
    if(!client->haveReputationRequirements(factoryIndustry().requirements))
    {
        QMessageBox::warning(this,tr("No requirements"),tr("You don't meet the requirements"));
        return;
    }
    int i=1;
    if(playerInformations.items.at(itemid)>1 && quantity>1)
    {
        uint32_t quantityToSell=quantity;
        if(playerInformations.items.at(itemid)<quantityToSell)
            quantityToSell=playerInformations.items.at(itemid);
        bool ok;
        i = QInputDialog::getInt(this, tr("Sell"),tr("Amount %1 to sell:")
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(itemid).name)),
                                 0, 0, quantityToSell, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
    {
        uint32_t newPrice=0;
        if(FactoryTrade::resourcePrice(factoryIndustry(),itemid,quantity,newPrice))
            item->setData(98,newPrice);
        factoryToResourceItem(item);
    }
    factoryTrade.sell(itemid,i,price);
    remove_to_inventory(itemid,i);
    client->sellFactoryResource(itemid,i,price);
}

const Industry &BaseWindow::factoryIndustry() const
{
    return QtDatapackClientLoader::datapackLoader->getMap(factoryMapId).industries.at(factoryId);
}

IndustryStatus &BaseWindow::factoryIndustryStatus()
{
    return FactoryTrade::industryStatus(client->get_player_informations(),factoryMapId,factoryId);
}

void BaseWindow::applyFactoryAnswer(const FactoryTrade::Answer &answer)
{
    if(answer.cashToAdd>0)
        addCash(answer.cashToAdd);
    if(answer.cashToRemove>0)
        removeCash(answer.cashToRemove);
    if(answer.itemsToAdd>0)
    {
        add_to_inventory(answer.item,answer.itemsToAdd,answer.done);
        if(!answer.done)
        {
            load_inventory();
            load_plant_inventory();
        }
    }
    switch(answer.result)
    {
        case FactoryTrade::Result_Sold:
            showTip(tr("Item sold").toStdString());
        break;
        case FactoryTrade::Result_SoldBetterPrice:
            showTip(tr("Item sold at better price").toStdString());
        break;
        case FactoryTrade::Result_NotTheQuantity:
            QMessageBox::information(this,tr("Information"),tr("Sorry but have not the quantity of this item"));
        break;
        case FactoryTrade::Result_PriceWorse:
            QMessageBox::information(this,tr("Information"),tr("Sorry but now the price is worse"));
        break;
        default:
        break;
    }
    if(answer.done)
    {
        appendReputationRewards(factoryIndustry().rewards);
        updateFactoryStatProduction();
    }
}

void BaseWindow::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice)
{
    applyFactoryAnswer(factoryTrade.buyAnswer(stat,newPrice,factoryIndustryStatus(),factoryIndustry(),QDateTime::currentSecsSinceEpoch()));
}

void BaseWindow::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice)
{
    applyFactoryAnswer(factoryTrade.sellAnswer(stat,newPrice,factoryIndustryStatus(),factoryIndustry(),QDateTime::currentSecsSinceEpoch()));
}

void BaseWindow::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{
    FactoryTrade::statusFromServerList(factoryIndustryStatus(),factoryIndustry(),remainingProductionTime,QDateTime::currentSecsSinceEpoch(),resources,products);
    unsigned int index;
    ui->factoryResources->clear();
    index=0;
    while(index<resources.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,resources.at(index).object);
        item->setData(98,resources.at(index).price);
        item->setData(97,resources.at(index).quantity);
        factoryToResourceItem(item);
        ui->factoryResources->addItem(item);
        index++;
    }
    ui->factoryProducts->clear();
    index=0;
    while(index<products.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,products.at(index).object);
        item->setData(98,products.at(index).price);
        item->setData(97,products.at(index).quantity);
        factoryToProductItem(item);
        ui->factoryProducts->addItem(item);
        index++;
    }
    ui->factoryStatus->setText(tr("Have the factory list"));
    updateFactoryStatProduction();
}

void BaseWindow::updateFactoryStatProduction()
{
    const IndustryStatus &industryStatus=factoryIndustryStatus();
    const Industry &industry=factoryIndustry();
    if(FacilityLib::factoryProductionStarted(industryStatus,industry))
    {
        if(Ultimate::ultimate.isUltimate())
        {
            std::string productionTime;
            const uint32_t remainingProductionTime=FactoryTrade::remainingProductionTime(industryStatus,industry,QDateTime::currentSecsSinceEpoch());
            if(remainingProductionTime>0)
            {
                productionTime=tr("Remaining time:").toStdString()+"<br />";
                if(remainingProductionTime<60)
                    productionTime+=tr("Less than a minute").toStdString();
                else
                    productionTime+=tr("%n minute(s)","",static_cast<int>(remainingProductionTime/60)).toStdString();
            }
            ui->factoryStatText->setText(tr("In production")+"<br />"+QString::fromStdString(productionTime));
        }
        else
            ui->factoryStatText->setText(tr("In production"));
    }
    else
        ui->factoryStatText->setText(tr("Production stopped"));
}

void BaseWindow::factoryToResourceItem(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->getItemExtra(itemId).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("Item %1\nPrice: %2$").arg(itemId).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(itemId).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    if(playerInformations.items.find(static_cast<uint16_t>(item->data(99).toUInt()))==playerInformations.items.cend() || !client->haveReputationRequirements(factoryIndustry().requirements))
    {
        item->setFont(MissingQuantity);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void BaseWindow::factoryToProductItem(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->getItemExtra(itemId).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("Item %1\nPrice: %2$").arg(itemId).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(itemId).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    if(item->data(98).toUInt()>playerInformations.cash)
    {
        item->setFont(MissingQuantity);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void BaseWindow::on_factoryQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}
