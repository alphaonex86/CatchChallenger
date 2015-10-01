#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/CommonDatapack.h"

#include <QInputDialog>

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
    uint32_t id=item->data(99).toUInt();
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();
    if(cash<price)
    {
        QMessageBox::warning(this,tr("No cash"),tr("No bash to buy this item"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        QMessageBox::warning(this,tr("No requirements"),tr("You don't meet the requirements"));
        return;
    }
    int i=1;
    if(cash>=(price*2) && quantity>1)
    {
        bool ok;
        i = QInputDialog::getInt(this, tr("Buy"),tr("Amount %1 to buy:").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(id).name), 0, 0, quantity, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
    {
        const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
        int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Product &product=industry.products.at(index);
            if(product.item==id)
            {
                item->setData(98,FacilityLib::getFactoryProductPrice(quantity,product,CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry)));
                break;
            }
            index++;
        }
        factoryToProductItem(item);
    }
    ItemToSellOrBuy itemToSellOrBuy;
    itemToSellOrBuy.quantity=quantity;
    itemToSellOrBuy.object=id;
    itemToSellOrBuy.price=i*price;
    itemsToBuy << itemToSellOrBuy;
    removeCash(itemToSellOrBuy.object);
    CatchChallenger::Api_client_real::client->buyFactoryProduct(factoryId,id,i,price);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.value(factoryId).rewards.reputation);
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
    uint32_t id=item->data(99).toUInt();
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();
    if(!items.contains(id))
    {
        QMessageBox::warning(this,tr("No item"),tr("You have not the item to sell"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        QMessageBox::warning(this,tr("No requirements"),tr("You don't meet the requirements"));
        return;
    }
    int i=1;
    if(items.value(id)>1 && quantity>1)
    {
        uint32_t quantityToSell=quantity;
        if(items.value(id)<quantityToSell)
            quantityToSell=items.value(id);
        bool ok;
        i = QInputDialog::getInt(this, tr("Sell"),tr("Amount %1 to sell:").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(id).name), 0, 0, quantityToSell, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
    {
        const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
        int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            if(resource.item==id)
            {
                item->setData(98,FacilityLib::getFactoryResourcePrice(resource.quantity*industry.cycletobefull-quantity,resource,CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry)));
                break;
            }
            index++;
        }
        factoryToResourceItem(item);
    }
    ItemToSellOrBuy tempItem;
    tempItem.object=id;
    tempItem.quantity=i;
    tempItem.price=price;
    itemsToSell << tempItem;
    remove_to_inventory(id,i);
    CatchChallenger::Api_client_real::client->sellFactoryResource(factoryId,id,i,price);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.value(factoryId).rewards.reputation);
}

void BaseWindow::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice)
{
    const ItemToSellOrBuy &itemToSellOrBuy=itemsToBuy.first();
    QHash<uint16_t,uint32_t> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[itemToSellOrBuy.object]=itemToSellOrBuy.quantity;
            if(industryStatus.products.contains(itemToSellOrBuy.object))
            {
                industryStatus.products[itemToSellOrBuy.object]-=itemToSellOrBuy.quantity;
                if(industryStatus.products.value(itemToSellOrBuy.object)==0)
                    industryStatus.products.remove(itemToSellOrBuy.object);
            }
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveBuyFactoryObject() Can't buy at 0$!";
                return;
            }
            addCash(itemToSellOrBuy.price);
            removeCash(newPrice*itemToSellOrBuy.quantity);
            items[itemToSellOrBuy.object]=itemToSellOrBuy.quantity;
            if(industryStatus.products.contains(itemToSellOrBuy.object))
            {
                industryStatus.products[itemToSellOrBuy.object]-=itemToSellOrBuy.quantity;
                if(industryStatus.products.value(itemToSellOrBuy.object)==0)
                    industryStatus.products.remove(itemToSellOrBuy.object);
            }
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(itemToSellOrBuy.object);
            QMessageBox::information(this,tr("Information"),tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(itemToSellOrBuy.object);
            QMessageBox::information(this,tr("Information"),tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyFactoryObject(stat) have unknow value";
        break;
    }
    switch(stat)
    {
        case BuyStat_Done:
        case BuyStat_BetterPrice:
        {
            if(factoryInProduction)
                break;
            const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
            industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000;
            updateFactoryStatProduction(industryStatus,industry);
        }
        break;
        default:
        break;
    }
    itemsToBuy.removeFirst();
}

void BaseWindow::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            if(!industryStatus.resources.contains(itemsToSell.first().object))
                industryStatus.resources[itemsToSell.first().object]=0;
            industryStatus.resources[itemsToSell.first().object]+=itemsToSell.first().quantity;
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellFactoryObject() the price 0$ can't be better price!";
                QMessageBox::information(this,tr("Information"),tr("Bug into returned price"));
                return;
            }
            if(!industryStatus.resources.contains(itemsToSell.first().object))
                industryStatus.resources[itemsToSell.first().object]=0;
            industryStatus.resources[itemsToSell.first().object]+=itemsToSell.first().quantity;
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            add_to_inventory(itemsToSell.first().object,itemsToSell.first().quantity,false);
            load_inventory();
            load_plant_inventory();
            QMessageBox::information(this,tr("Information"),tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            add_to_inventory(itemsToSell.first().object,itemsToSell.first().quantity,false);
            load_inventory();
            load_plant_inventory();
            QMessageBox::information(this,tr("Information"),tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveSellFactoryObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
    switch(stat)
    {
        case BuyStat_Done:
        case BuyStat_BetterPrice:
        {
            if(factoryInProduction)
                break;
            const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
            industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000;
            updateFactoryStatProduction(industryStatus,industry);
        }
        break;
        default:
        break;
    }
}

void BaseWindow::haveFactoryList(const uint32_t &remainingProductionTime,const QList<ItemToSellOrBuy> &resources,const QList<ItemToSellOrBuy> &products)
{
    industryStatus.products.clear();
    industryStatus.resources.clear();
    const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
    industryStatus.last_update=QDateTime::currentMSecsSinceEpoch()/1000+remainingProductionTime-industry.time;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveFactoryList()";
    #endif
    int index;
    ui->factoryResources->clear();
    index=0;
    while(index<resources.size())
    {
        int sub_index=0;
        while(sub_index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(sub_index);
            if(resource.item==resources.at(index).object)
            {
                industryStatus.resources[resources.at(index).object]=industry.cycletobefull*resource.quantity-resources.at(index).quantity;
                break;
            }
            sub_index++;
        }
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
    updateFactoryStatProduction(industryStatus,industry);
}

void BaseWindow::updateFactoryStatProduction(const IndustryStatus &industryStatus,const Industry &industry)
{
    if(FacilityLib::factoryProductionStarted(industryStatus,industry))
    {
        factoryInProduction=true;
        #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
        QString productionTime;
        uint32_t remainingProductionTime=0;
        if((industryStatus.last_update+industry.time)>(QDateTime::currentMSecsSinceEpoch()/1000))
            remainingProductionTime=(industryStatus.last_update+industry.time)-(QDateTime::currentMSecsSinceEpoch()/1000);
        else if((industryStatus.last_update+industry.time)<(QDateTime::currentMSecsSinceEpoch()/1000))
            remainingProductionTime=((QDateTime::currentMSecsSinceEpoch()/1000)-industryStatus.last_update)%industry.time;
        else
            remainingProductionTime=industry.time;
        if(remainingProductionTime>0)
        {
            productionTime=tr("Remaining time:")+"<br />";
            if(remainingProductionTime<60)
                productionTime+=tr("Less than a minute");
            else
                productionTime+=tr("%n minute(s)","",remainingProductionTime/60);
        }
        ui->factoryStatText->setText(QStringLiteral("%1<br />%2").arg(tr("In production")).arg(productionTime));
        #else
        ui->factoryStatText->setText(tr("In production"));
        #endif
    }
    else
    {
        factoryInProduction=false;
        ui->factoryStatText->setText(tr("Production stopped"));
    }
}

void BaseWindow::factoryToResourceItem(QListWidgetItem *item)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(item->data(99).toUInt()))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).name).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).name).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    else
    {
        item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("Item %1\nPrice: %2$").arg(item->data(99).toUInt()).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(item->data(99).toUInt()).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    if(!items.contains(item->data(99).toUInt()) || !haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        item->setFont(MissingQuantity);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void BaseWindow::factoryToProductItem(QListWidgetItem *item)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(item->data(99).toUInt()))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).image);
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).name).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item->data(99).toUInt()).name).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    else
    {
        item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        if(item->data(97).toUInt()==0)
            item->setToolTip(tr("Item %1\nPrice: %2$").arg(item->data(99).toUInt()).arg(item->data(98).toUInt()));
        else
            item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(item->data(99).toUInt()).arg(item->data(98).toUInt()).arg(item->data(97).toUInt()));
    }
    if(item->data(98).toUInt()>cash)
    {
        item->setFont(MissingQuantity);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void BaseWindow::on_factoryQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}
