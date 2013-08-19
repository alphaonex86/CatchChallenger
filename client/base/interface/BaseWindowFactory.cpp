#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"

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
    quint32 quantity=1;
    quint32 id=item->data(99).toUInt();
    quint32 price=item->data(98).toUInt();
    if(!item->text().isEmpty())
        quantity=item->text().toUInt();
    if(cash<price)
    {
        QMessageBox::warning(this,tr("No cash"),tr("No bash to buy this item"));
        return;
    }
    int i=1;
    if(cash>=(price*2) && quantity>1)
    {
        bool ok;
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    if(quantity<1)
        delete item;
    else if(quantity==1)
        item->setText(QString());
    else
        item->setText(QString::number(quantity));
    tempQuantityForBuy=quantity;
    tempItemForBuy=id;
    tempCashForBuy=i*price;
    removeCash(tempCashForBuy);
    CatchChallenger::Api_client_real::client->buyFactoryObject(factoryId,id,i,price);
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
    quint32 quantity=1;
    quint32 id=item->data(99).toUInt();
    quint32 price=item->data(98).toUInt();
    if(!item->text().isEmpty())
        quantity=item->text().toUInt();
    if(!items.contains(id))
    {
        QMessageBox::warning(this,tr("No item"),tr("You have not the item to sell"));
        return;
    }
    int i=1;
    if(items[id]>1 && quantity>1)
    {
        bool ok;
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    if(quantity<1)
        delete item;
    else if(quantity==1)
        item->setText(QString());
    else
        item->setText(QString::number(quantity));
    ItemToSellOrBuy tempItem;
    tempItem.object=id;
    tempItem.quantity=i;
    tempItem.price=price;
    itemsToSell << tempItem;
    items[id]-=i;
    if(items[id]==0)
        items.remove(id);
    CatchChallenger::Api_client_real::client->sellFactoryObject(factoryId,id,i,price);
}

void BaseWindow::haveBuyFactoryObject(const BuyStat &stat,const quint32 &newPrice)
{
    QHash<quint32,quint32> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveBuyFactoryObject() Can't buy at 0$!";
                return;
            }
            addCash(tempCashForBuy);
            removeCash(newPrice*tempQuantityForBuy);
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyFactoryObject(stat) have unknow value";
        break;
    }
}

void BaseWindow::haveSellFactoryObject(const SoldStat &stat,const quint32 &newPrice)
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
                qDebug() << "haveSellFactoryObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveSellFactoryObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
}

void BaseWindow::haveFactoryList(const QList<ItemToSellOrBuy> &resources,const QList<ItemToSellOrBuy> &products)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveFactoryList()";
    #endif
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    int index;
    ui->factoryResources->clear();
    index=0;
    while(index<resources.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,resources.at(index).object);
        item->setData(98,resources.at(index).price);
        if(resources.at(index).quantity>1)
            item->setText(QString::number(resources.at(index).quantity));
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(resources.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].image);
            if(resources.at(index).quantity==0)
                item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].name).arg(resources.at(index).price));
            else
                item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].name).arg(resources.at(index).price).arg(resources.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(resources.at(index).quantity==0)
                item->setToolTip(tr("Item %1\nPrice: %2$").arg(resources.at(index).object).arg(resources.at(index).price));
            else
                item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(resources.at(index).object).arg(resources.at(index).price).arg(resources.at(index).quantity));
        }
        if(!items.contains(resources.at(index).object))
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
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
        if(products.at(index).quantity>1)
            item->setText(QString::number(products.at(index).quantity));
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(products.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].image);
            if(products.at(index).quantity==0)
                item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].name).arg(products.at(index).price));
            else
                item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].name).arg(products.at(index).price).arg(products.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(products.at(index).quantity==0)
                item->setToolTip(tr("Item %1\nPrice: %2$").arg(products.at(index).object).arg(products.at(index).price));
            else
                item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(products.at(index).object).arg(products.at(index).price).arg(products.at(index).quantity));
        }
        if(products.at(index).price>cash)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->factoryProducts->addItem(item);
        index++;
    }
    ui->factoryStatus->setText(tr("Have the factory list"));
}

void BaseWindow::on_factoryQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}
