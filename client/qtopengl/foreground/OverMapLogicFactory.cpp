#include "OverMapLogic.hpp"
#include <iostream>
#include <QDebug>
#include "../../../general/base/CommonDatapack.hpp"

/*void OverMapLogic::on_factoryProducts_itemActivated(QListWidgetItem *item)
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
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
    {
        QMessageBox::warning(this,tr("No requirements"),tr("You don't meet the requirements"));
        return;
    }
    int quantityToBuy=1;
    if(playerInformations.cash>=(price*2) && quantity>1)
    {
        bool ok;
        quantityToBuy = QInputDialog::getInt(this, tr("Buy"),tr("Amount %1 to buy:")
                                             .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(id).name)),
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
        const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Product &product=industry.products.at(index);
            if(product.item==id)
            {
                item->setData(98,FacilityLib::getFactoryProductPrice(quantity,product,CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry)));
                break;
            }
            index++;
        }
        factoryToProductItem(item);
    }
    ItemToSellOrBuy itemToSellOrBuy;
    itemToSellOrBuy.quantity=quantityToBuy;
    itemToSellOrBuy.object=id;
    itemToSellOrBuy.price=quantityToBuy*price;
    itemsToBuy.push_back(itemToSellOrBuy);
    removeCash(itemToSellOrBuy.object);
    client->buyFactoryProduct(factoryId,id,quantityToBuy,price);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);
}

void OverMapLogic::on_factorySell_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->factoryResources->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_factoryResources_itemActivated(selectedItems.first());
}

void OverMapLogic::on_factoryResources_itemActivated(QListWidgetItem *item)
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
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
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
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemid).name)),
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
        const Industry &industry=CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
        unsigned int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            if(resource.item==itemid)
            {
                item->setData(98,FacilityLib::getFactoryResourcePrice(resource.quantity*industry.cycletobefull-quantity,resource,CommonDatapack::commonDatapack.industries.at(CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry)));
                break;
            }
            index++;
        }
        factoryToResourceItem(item);
    }
    ItemToSellOrBuy tempItem;
    tempItem.object=itemid;
    tempItem.quantity=i;
    tempItem.price=price;
    itemsToSell.push_back(tempItem);
    remove_to_inventory(itemid,i);
    client->sellFactoryResource(factoryId,itemid,i,price);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.at(factoryId).rewards.reputation);
}*/

void OverMapLogic::haveBuyFactoryObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice)
{
    const CatchChallenger::ItemToSellOrBuy &itemToSellOrBuy=itemsToBuy.front();
    std::unordered_map<uint16_t,uint32_t> items;
    switch(stat)
    {
        case CatchChallenger::BuyStat_Done:
            items[itemToSellOrBuy.object]=itemToSellOrBuy.quantity;
            if(industryStatus.products.find(itemToSellOrBuy.object)!=industryStatus.products.cend())
            {
                industryStatus.products[itemToSellOrBuy.object]-=itemToSellOrBuy.quantity;
                if(industryStatus.products.at(itemToSellOrBuy.object)==0)
                    industryStatus.products.erase(itemToSellOrBuy.object);
            }
            add_to_inventory(items);
        break;
        case CatchChallenger::BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveBuyFactoryObject() Can't buy at 0$!";
                return;
            }
            addCash(itemToSellOrBuy.price);
            removeCash(newPrice*itemToSellOrBuy.quantity);
            items[itemToSellOrBuy.object]=itemToSellOrBuy.quantity;
            if(industryStatus.products.find(itemToSellOrBuy.object)!=industryStatus.products.cend())
            {
                industryStatus.products[itemToSellOrBuy.object]-=itemToSellOrBuy.quantity;
                if(industryStatus.products.at(itemToSellOrBuy.object)==0)
                    industryStatus.products.erase(itemToSellOrBuy.object);
            }
            add_to_inventory(items);
        break;
        case CatchChallenger::BuyStat_HaveNotQuantity:
            addCash(itemToSellOrBuy.object);
            showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::BuyStat_PriceHaveChanged:
            addCash(itemToSellOrBuy.object);
            showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
            qDebug() << "haveBuyFactoryObject(stat) have unknow value";
        break;
    }
    switch(stat)
    {
        case CatchChallenger::BuyStat_Done:
        case CatchChallenger::BuyStat_BetterPrice:
        {
            if(factoryInProduction)
                break;
            const CatchChallenger::Industry &industry=CatchChallenger::CommonDatapack::commonDatapack.industries.at(CatchChallenger::CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
            industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000;
            updateFactoryStatProduction(industryStatus,industry);
        }
        break;
        default:
        break;
    }
    itemsToBuy.erase(itemsToBuy.cbegin());
}

void OverMapLogic::haveSellFactoryObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case CatchChallenger::SoldStat_Done:
            if(industryStatus.resources.find(itemsToSell.front().object)==industryStatus.resources.cend())
                industryStatus.resources[itemsToSell.front().object]=0;
            industryStatus.resources[itemsToSell.front().object]+=itemsToSell.front().quantity;
            addCash(itemsToSell.front().price*itemsToSell.front().quantity);
            showTip(tr("Item sold").toStdString());
        break;
        case CatchChallenger::SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellFactoryObject() the price 0$ can't be better price!";
                showTip(tr("Bug into returned price").toStdString());
                return;
            }
            if(industryStatus.resources.find(itemsToSell.front().object)==industryStatus.resources.cend())
                industryStatus.resources[itemsToSell.front().object]=0;
            industryStatus.resources[itemsToSell.front().object]+=itemsToSell.front().quantity;
            addCash(newPrice*itemsToSell.front().quantity);
            showTip(tr("Item sold at better price").toStdString());
        break;
        case CatchChallenger::SoldStat_WrongQuantity:
            add_to_inventory(itemsToSell.front().object,itemsToSell.front().quantity,false);
            showTip(tr("Sorry but have not the quantity of this item").toStdString());
            /*load_inventory();
            load_plant_inventory();*/
            abort();
        break;
        case CatchChallenger::SoldStat_PriceHaveChanged:
            add_to_inventory(itemsToSell.front().object,itemsToSell.front().quantity,false);
            showTip(tr("Sorry but now the price is worse").toStdString());
            /*            load_inventory();
            load_plant_inventory();*/
            abort();
        break;
        default:
            qDebug() << "haveSellFactoryObject(stat) have unknow value";
        break;
    }
    itemsToSell.erase(itemsToSell.cbegin());
    switch(stat)
    {
        case CatchChallenger::SoldStat_Done:
        case CatchChallenger::SoldStat_BetterPrice:
        {
            if(factoryInProduction)
                break;
            const CatchChallenger::IndustryLink &industryLink=CatchChallenger::CommonDatapack::commonDatapack.industriesLink.at(factoryId);
            const CatchChallenger::Industry &industry=CatchChallenger::CommonDatapack::commonDatapack.industries.at(industryLink.industry);
            industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000;
            updateFactoryStatProduction(industryStatus,industry);
        }
        break;
        default:
        break;
    }
}

void OverMapLogic::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<CatchChallenger::ItemToSellOrBuy> &resources,const std::vector<CatchChallenger::ItemToSellOrBuy> &products)
{
    abort();
    /*
    industryStatus.products.clear();
    industryStatus.resources.clear();
    const CatchChallenger::Industry &industry=CatchChallenger::CommonDatapack::commonDatapack.industries.at(CatchChallenger::CommonDatapack::commonDatapack.industriesLink.at(factoryId).industry);
    industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000+remainingProductionTime-industry.time;
    unsigned int index;
    ui->factoryResources->clear();
    index=0;
    while(index<resources.size())
    {
        unsigned int sub_index=0;
        while(sub_index<industry.resources.size())
        {
            const CatchChallenger::Industry::Resource &resource=industry.resources.at(sub_index);
            if(resource.item==resources.at(index).object)
            {
                industryStatus.resources[resources.at(index).object]=industry.cycletobefull*resource.quantity-resources.at(index).quantity;
                break;
            }
            sub_index++;
        }
        if(sub_index==industry.resources.size())
            std::cerr << "sub_index==industry.resources.size()" << std::endl;
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,resources.at(index).object);
        item->setData(98,resources.at(index).price);
        item->setData(97,resources.at(index).quantity);
        factoryToResourceItem(item);
        ui->factoryResources->addItem(item);
        index++;
    }
    {
        unsigned int sub_index=0;
        while(sub_index<industry.resources.size())
        {
            const CatchChallenger::Industry::Resource &resource=industry.resources.at(sub_index);
            if(industryStatus.resources.find(resource.item)==industryStatus.resources.cend())
            {
                std::cerr << "Ressource " << resource.item << " not returned, consider as full: " << industry.cycletobefull*resource.quantity << std::endl;
                industryStatus.resources[resource.item]=industry.cycletobefull*resource.quantity;
            }
            sub_index++;
        }
    }

    ui->factoryProducts->clear();
    index=0;
    while(index<products.size())
    {
        industryStatus.products[products.at(index).object]=products.at(index).quantity;
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,products.at(index).object);
        item->setData(98,products.at(index).price);
        item->setData(97,products.at(index).quantity);
        factoryToProductItem(item);
        ui->factoryProducts->addItem(item);
        index++;
    }
    ui->factoryStatus->setText(tr("Have the factory list"));
    updateFactoryStatProduction(industryStatus,industry);*/
}

void OverMapLogic::updateFactoryStatProduction(const CatchChallenger::IndustryStatus &industryStatus,const CatchChallenger::Industry &industry)
{
    abort();
    /*
    if(FacilityLib::factoryProductionStarted(industryStatus,industry))
    {
        factoryInProduction=true;
        if(Ultimate::ultimate.isUltimate())
        {
            std::string productionTime;
            uint64_t remainingProductionTime=0;
            if((uint64_t)(industryStatus.last_update+industry.time)>(uint64_t)(QDateTime::currentMSecsSinceEpoch()/1000))
                remainingProductionTime=static_cast<uint32_t>((industryStatus.last_update+industry.time)-(QDateTime::currentMSecsSinceEpoch()/1000));
            else if((uint64_t)(industryStatus.last_update+industry.time)<(uint64_t)(QDateTime::currentMSecsSinceEpoch()/1000))
                remainingProductionTime=((QDateTime::currentMSecsSinceEpoch()/1000)-industryStatus.last_update)%industry.time;
            else
                remainingProductionTime=industry.time;
            if(remainingProductionTime>0)
            {
                productionTime=tr("Remaining time:").toStdString()+"<br />";
                if(remainingProductionTime<60)
                    productionTime+=tr("Less than a minute").toStdString();
                else
                    productionTime+=tr("%n minute(s)","",static_cast<uint32_t>(remainingProductionTime/60)).toStdString();
            }
            ui->factoryStatText->setText(tr("In production")+"<br />"+QString::fromStdString(productionTime));
        }
        else
            ui->factoryStatText->setText(tr("In production"));
    }
    else
    {
        factoryInProduction=false;
        ui->factoryStatText->setText(tr("Production stopped"));
    }*/
}

/*void OverMapLogic::factoryToResourceItem(QListWidgetItem *item)
{
    abort();
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(itemId)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(itemId).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("Item %1\nPrice: %2$").arg(itemId).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(itemId).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    if(playerInformations.items.find(static_cast<uint16_t>(item->data(99).toUInt()))==playerInformations.items.cend() || !haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.at(factoryId).requirements.reputation))
    {
        item->setFont(MissingQuantity);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}*/

/*void OverMapLogic::factoryToProductItem(QListWidgetItem *item)
{
    abort();
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(itemId)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(itemId).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
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
}*/

/*void OverMapLogic::on_factoryQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}*/
