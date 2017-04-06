#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../../general/base/CommonDatapack.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::on_toolButton_quit_shop_clicked()
{
    waitToSell=false;
    inSelection=false;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_shopItemList_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(!waitToSell)
    {
        if(playerInformations.cash<itemsIntoTheShop.value(shop_items_graphical.value(item)).price)
        {
            QMessageBox::information(this,tr("Buy"),tr("You have not the cash to buy this item"));
            return;
        }
        bool ok=true;
        ItemToSellOrBuy itemToSellOrBuy;
        if(playerInformations.cash/itemsIntoTheShop.value(shop_items_graphical.value(item)).price>1)
            itemToSellOrBuy.quantity=QInputDialog::getInt(this,tr("Buy"),tr("Quantity to buy"),1,1,playerInformations.cash/itemsIntoTheShop.value(shop_items_graphical.value(item)).price,1,&ok);
        else
            itemToSellOrBuy.quantity=1;
        if(!ok)
            return;
        itemToSellOrBuy.object=shop_items_graphical.value(item);
        client->buyObject(shopId,itemToSellOrBuy.object,itemToSellOrBuy.quantity,itemsIntoTheShop.value(itemToSellOrBuy.object).price);
        itemToSellOrBuy.price=itemsIntoTheShop.value(itemToSellOrBuy.object).price*itemToSellOrBuy.quantity;
        itemsToBuy << itemToSellOrBuy;
        removeCash(itemToSellOrBuy.price);
        showTip(tr("Buying the object..."));
    }
    else
    {
        if(playerInformations.items.find(shop_items_graphical.value(item))==playerInformations.items.cend())
            return;
        uint32_t objectItem=shop_items_graphical.value(item);
        bool ok=true;
        if(playerInformations.items.at(objectItem)>1)
            tempQuantityForSell=QInputDialog::getInt(this,tr("Sell"),tr("Quantity to sell"),1,1,playerInformations.items.at(objectItem),1,&ok);
        else
            tempQuantityForSell=1;
        if(!ok)
            return;
        if(playerInformations.items.find(objectItem)==playerInformations.items.cend())
            return;
        if(playerInformations.items.at(objectItem)<tempQuantityForSell)
            return;
        objectSelection(true,objectItem,tempQuantityForSell);
        showTip(tr("Selling the object..."));
        displaySellList();
    }
}

void BaseWindow::on_shopItemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        ui->shopName->setText("");
        ui->shopDescription->setText(tr("Select an object"));
        ui->shopBuy->setVisible(false);
        ui->shopImage->setPixmap(QPixmap(":/images/inventory/unknown-object.png"));
        return;
    }
    ui->shopBuy->setVisible(true);
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.itemsExtra.contains(shop_items_graphical.value(item)))
    {
        ui->shopImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->shopName->setText(tr("Unknown name"));
        ui->shopDescription->setText(tr("Unknown description"));
        return;
    }
    const DatapackClientLoader::ItemExtra &content=DatapackClientLoader::datapackLoader.itemsExtra.value(shop_items_graphical.value(item));

    ui->shopImage->setPixmap(content.image);
    ui->shopName->setText(content.name);
    ui->shopDescription->setText(content.description);
}

void BaseWindow::on_shopBuy_clicked()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_shopBuy_clicked() no correct selection";
        return;
    }
    on_shopItemList_itemActivated(items.first());
}

void BaseWindow::haveShopList(const QList<ItemToSellOrBuy> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveShopList()";
    #endif
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    int index=0;
    while(index<items.size())
    {
        itemsIntoTheShop[items.at(index).object]=items.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        shop_items_to_graphical[items.at(index).object]=item;
        shop_items_graphical[item]=items.at(index).object;
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(items.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(items.at(index).object).image);
            if(items.at(index).quantity==0)
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(items.at(index).object).name).arg(items.at(index).price));
            else
                item->setText(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(items.at(index).object).name).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(items.at(index).quantity==0)
                item->setText(tr("Item %1\nPrice: %2$").arg(items.at(index).object).arg(items.at(index).price));
            else
                item->setText(tr("Item %1 at %2$\nQuantity: %3").arg(items.at(index).object).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        if(items.at(index).price>playerInformations.cash)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->shopItemList->addItem(item);
        index++;
    }
    ui->shopBuy->setText(tr("Buy"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::displaySellList()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::displaySellList()";
    #endif
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while(i!=playerInformations.items.cend())
    {
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i->first) && CatchChallenger::CommonDatapack::commonDatapack.items.item.at(i->first).price>0)
        {
            QListWidgetItem *item=new QListWidgetItem();
            shop_items_to_graphical[i->first]=item;
            shop_items_graphical[item]=i->first;
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(i->first).image);
            if(i->second>1)
                item->setText(tr("%1\nPrice: %2$, quantity: %3")
                        .arg(DatapackClientLoader::datapackLoader.itemsExtra.value(i->first).name)
                        .arg(CatchChallenger::CommonDatapack::commonDatapack.items.item.at(i->first).price/2)
                        .arg(i->second)
                        );
            else
                item->setText(tr("%1\nPrice: %2$")
                        .arg(DatapackClientLoader::datapackLoader.itemsExtra.value(i->first).name)
                        .arg(CatchChallenger::CommonDatapack::commonDatapack.items.item.at(i->first).price/2)
                        );
            ui->shopItemList->addItem(item);
        }
        ++i;
    }
    ui->shopBuy->setText(tr("Sell"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::haveBuyObject(const BuyStat &stat,const uint32_t &newPrice)
{
    const ItemToSellOrBuy &itemToSellOrBuy=itemsToBuy.first();
    QHash<uint32_t,uint32_t> items;
    switch(stat)
    {
        case BuyStat_Done:
            add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() Can't buy at 0$!";
                return;
            }
            addCash(itemToSellOrBuy.price);
            removeCash(newPrice*itemToSellOrBuy.quantity);
            add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(itemToSellOrBuy.price);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(itemToSellOrBuy.price);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToBuy.removeFirst();
}

void BaseWindow::haveSellObject(const SoldStat &stat,const uint32_t &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            add_to_inventory(itemsToSell.first().object,itemsToSell.first().quantity,false);
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            add_to_inventory(itemsToSell.first().object,itemsToSell.first().quantity,false);
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
    displaySellList();
}
