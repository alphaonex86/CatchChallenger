#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonSettings.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::marketList(const quint64 &price,const QList<MarketObject> &marketObjectList,const QList<MarketMonster> &marketMonsterList,const QList<MarketObject> &marketOwnObjectList,const QList<MarketMonster> &marketOwnMonsterList)
{
    ui->marketWithdraw->setVisible(true);
    ui->marketStat->setText(tr("Cash to withdraw: %1$").arg(price));
    int index;
    //the object list
    ui->marketObject->clear();
    index=0;
    while(index<marketObjectList.size())
    {
        const MarketObject &marketObject=marketObjectList.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        updateMarketObject(item,marketObject);
        ui->marketObject->addItem(item);
    }
    //the monster list
    ui->marketMonster->clear();
    index=0;
    while(index<marketMonsterList.size())
    {
        const MarketMonster &marketMonster=marketMonsterList.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,marketMonster.monsterId);
        item->setData(98,marketMonster.price);
        item->setData(96,marketMonster.level);
        QString price;
        if(marketMonster.price>0)
            price=tr("Price: %1$").arg(marketMonster.price);
        else
            price=tr("Price: Free");
        if(DatapackClientLoader::datapackLoader.monsterExtra.contains(marketMonster.monster))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).thumb);
            item->setText(QStringLiteral("%1 level %2\n%3").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).name).arg(marketMonster.level).arg(price));
            item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).description);
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            item->setText(QStringLiteral("Unknown item with id %1 level %2\n%3").arg(marketMonster.monster).arg(marketMonster.level).arg(price));
        }
        ui->marketMonster->addItem(item);
    }
    //the object own list
    ui->marketOwnObject->clear();
    index=0;
    while(index<marketOwnObjectList.size())
    {
        const MarketObject &marketObject=marketOwnObjectList.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        updateMarketObject(item,marketObject);
        ui->marketOwnObject->addItem(item);
        index++;
    }
    //the object list
    ui->marketOwnMonster->clear();
    index=0;
    while(index<marketOwnMonsterList.size())
    {
        addOwnMonster(marketOwnMonsterList.at(index));
        index++;
    }
}

void BaseWindow::addOwnMonster(const MarketMonster &marketMonster)
{
    QListWidgetItem *item=new QListWidgetItem();
    item->setData(99,marketMonster.monsterId);
    item->setData(98,marketMonster.price);
    QString price;
    if(marketMonster.price>0)
        price=tr("Price: %1$").arg(marketMonster.price);
    else
        price=tr("Price: Free");
    if(DatapackClientLoader::datapackLoader.monsterExtra.contains(marketMonster.monster))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).thumb);
        item->setText(QStringLiteral("%1 level %2\n%3").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).name).arg(marketMonster.level).arg(price));
        item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra.value(marketMonster.monster).description);
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/small.png"));
        item->setText(QStringLiteral("Unknown monster with id %1 level %2\n%3").arg(marketMonster.monster).arg(marketMonster.level).arg(price));
    }
    ui->marketOwnMonster->addItem(item);
}

void BaseWindow::marketBuy(const bool &success)
{
    marketBuyInSuspend=false;
    if(!success)
    {
        addCash(marketBuyCashInSuspend);
        marketBuyObjectList.removeFirst();
        QMessageBox::warning(this,tr("Warning"),tr("Your buy in the market have failed"));
    }
    else
    {
        QHash<quint16,quint32> items;
        items[marketBuyObjectList.first().first]=marketBuyObjectList.first().second;
        add_to_inventory(items);
        marketBuyObjectList.removeFirst();
    }
    marketBuyCashInSuspend=0;
}

void BaseWindow::marketBuyMonster(const PlayerMonster &playerMonster)
{

    marketBuyInSuspend=false;
    ClientFightEngine::fightEngine.addPlayerMonster(playerMonster);
    load_monsters();
    marketBuyCashInSuspend=0;
}

void BaseWindow::marketPut(const bool &success)
{
    if(!success)
    {
        if(!marketPutObjectInSuspendList.isEmpty())
            add_to_inventory(marketPutObjectInSuspendList,false);
        if(!marketPutMonsterList.isEmpty())
        {
            ClientFightEngine::fightEngine.insertPlayerMonster(marketPutMonsterPlaceList.first(),marketPutMonsterList.first());
            load_monsters();
        }
        QMessageBox::warning(this,tr("Warning"),tr("Unable to put into the market"));
    }
    else
    {
        if(!marketPutMonsterList.isEmpty())
        {
            MarketMonster marketMonster;
            marketMonster.price=marketPutCashInSuspend;
            marketMonster.level=marketPutMonsterList.first().level;
            marketMonster.monster=marketPutMonsterList.first().monster;
            marketMonster.monsterId=marketPutMonsterList.first().id;
            addOwnMonster(marketMonster);
        }
        if(!marketPutObjectInSuspendList.isEmpty())
        {
            MarketObject marketObject;
            marketObject.price=marketPutCashInSuspend;
            marketObject.marketObjectId=0;
            marketObject.objectId=marketPutObjectInSuspendList.first().first;
            marketObject.quantity=marketPutObjectInSuspendList.first().second;
            QListWidgetItem *item=new QListWidgetItem();
            updateMarketObject(item,marketObject);
            ui->marketOwnObject->addItem(item);
        }
    }
    marketPutCashInSuspend=0;
    marketPutObjectInSuspendList.clear();
    marketPutMonsterList.clear();
    marketPutMonsterPlaceList.clear();
}

void BaseWindow::marketGetCash(const quint64 &cash)
{
    addCash(cash);
    ui->marketStat->setText(tr("Cash to withdraw: %1$").arg(0));
}

void BaseWindow::marketWithdrawCanceled()
{
    QMessageBox::warning(this,tr("Warning"),tr("Unable to withdraw from the market"));
    marketWithdrawInSuspend=false;
    if(!marketWithdrawObjectList.isEmpty())
    {
        QListWidgetItem *item=new QListWidgetItem();
        updateMarketObject(item,marketWithdrawObjectList.first());
        ui->marketOwnObject->addItem(item);
    }
    if(!marketWithdrawMonsterList.isEmpty())
        addOwnMonster(marketWithdrawMonsterList.first());
    marketWithdrawObjectList.clear();
    marketWithdrawMonsterList.clear();
}

void BaseWindow::marketWithdrawObject(const quint32 &objectId,const quint32 &quantity)
{
    marketWithdrawInSuspend=false;
    marketWithdrawObjectList.clear();
    marketWithdrawMonsterList.clear();
    add_to_inventory(objectId,quantity);
}

void BaseWindow::marketWithdrawMonster(const PlayerMonster &playerMonster)
{
    marketWithdrawInSuspend=false;
    marketWithdrawObjectList.clear();
    marketWithdrawMonsterList.clear();
    ClientFightEngine::fightEngine.addPlayerMonster(playerMonster);
}

void BaseWindow::on_marketPutObject_clicked()
{
    selectObject(ObjectType_SellToMarket);
}

void BaseWindow::on_marketPutMonster_clicked()
{
    selectObject(ObjectType_MonsterToTradeToMarket);
}

void BaseWindow::on_marketQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_marketWithdraw_clicked()
{
    CatchChallenger::Api_client_real::client->recoverMarketCash();
}

void BaseWindow::updateMarketObject(QListWidgetItem *item,const MarketObject &marketObject)
{
    item->setData(99,marketObject.marketObjectId);
    item->setData(98,marketObject.quantity);
    item->setData(97,marketObject.price);
    item->setData(95,marketObject.objectId);
    QString price;
    if(marketObject.price>0)
        price=tr("Price: %1$").arg(marketObject.price);
    else
        price=tr("Price: Free");
    QString quantity;
    if(marketObject.quantity>1)
        quantity=QStringLiteral(", ")+tr("quantity: %1").arg(marketObject.quantity);
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(marketObject.objectId))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.objectId).image);
        item->setText(QStringLiteral("%1%2\n%3").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.objectId).name).arg(quantity).arg(price));
        item->setToolTip(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.objectId).description);
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/small.png"));
        item->setText(QStringLiteral("Unknown item with id %1%2\n%3").arg(marketObject.objectId).arg(quantity).arg(price));
    }
}

void BaseWindow::on_marketObject_itemActivated(QListWidgetItem *item)
{
    if(marketBuyInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a buy in progress"));
        return;
    }
    quint32 priceQuantity;
    if(item->data(97).toUInt()>0)
        priceQuantity=cash/item->data(97).toUInt();
    else
        priceQuantity=item->data(98).toUInt();
    if(priceQuantity>item->data(98).toUInt())
        priceQuantity=item->data(98).toUInt();
    quint32 quantity=priceQuantity;
    if(quantity==0)
    {
        QMessageBox::warning(this,tr("Error"),tr("Have not cash to buy it"));
        return;
    }
    if(quantity>1)
    {
        bool ok;
        quantity=QInputDialog::getInt(this,tr("Quantity"),tr("How many item wish you buy?"),quantity,1,quantity,1,&ok);
        if(!ok)
            return;
    }
    CatchChallenger::Api_client_real::client->buyMarketObject(item->data(99).toUInt(),quantity);
    QPair<quint32,quint32> newEntry;
    newEntry.first=item->data(99).toUInt();
    newEntry.second=quantity;
    marketBuyObjectList << newEntry;
    marketBuyInSuspend=true;
    marketBuyCashInSuspend=quantity*item->data(97).toUInt();
    removeCash(marketBuyCashInSuspend);
    item->setData(98,item->data(98).toUInt()-quantity);
    if(item->data(98).toUInt()==0)
        delete item;
    else
    {
        MarketObject marketObject;
        marketObject.marketObjectId=item->data(99).toUInt();
        marketObject.quantity=item->data(98).toUInt();
        marketObject.price=item->data(97).toUInt();
        marketObject.objectId=item->data(95).toUInt();
        updateMarketObject(item,marketObject);
    }
}

void BaseWindow::on_marketOwnObject_itemActivated(QListWidgetItem *item)
{
    if(marketWithdrawInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a withdraw in progress"));
        return;
    }
    bool ok;
    quint32 quantity=1;
    if(item->data(98).toUInt()>1)
    {
        quantity=QInputDialog::getInt(this,tr("Quantity"),tr("How many item wish you withdraw?"),item->data(98).toUInt(),1,item->data(98).toUInt(),1,&ok);
        if(!ok)
            return;
    }
    CatchChallenger::Api_client_real::client->withdrawMarketObject(item->data(95).toUInt(),item->data(98).toUInt());
    marketWithdrawInSuspend=true;
    MarketObject marketObject;
    marketObject.marketObjectId=item->data(99).toUInt();
    marketObject.quantity=item->data(98).toUInt();
    marketObject.price=item->data(97).toUInt();
    marketObject.objectId=item->data(95).toUInt();
    marketWithdrawObjectList << marketObject;
    item->setData(98,item->data(98).toUInt()-quantity);
    if(item->data(98).toUInt()==0)
        delete item;
    else
        updateMarketObject(item,marketObject);
}

void BaseWindow::on_marketMonster_itemActivated(QListWidgetItem *item)
{
    if(marketBuyInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a buy in progress"));
        return;
    }
    quint32 priceQuantity;
    if(item->data(98).toUInt()>0)
        priceQuantity=cash/item->data(98).toUInt();
    else
        priceQuantity=1;
    if(priceQuantity>1)
        priceQuantity=1;
    quint32 quantity=priceQuantity;
    marketBuyCashInSuspend=item->data(98).toUInt();
    removeCash(marketBuyCashInSuspend);
    if(quantity==0)
    {
        QMessageBox::warning(this,tr("Error"),tr("Have not cash to buy it"));
        return;
    }
    CatchChallenger::Api_client_real::client->buyMarketMonster(item->data(99).toUInt());
    delete item;
}

void BaseWindow::on_marketOwnMonster_itemActivated(QListWidgetItem *item)
{
    if(ClientFightEngine::fightEngine.getPlayerMonster().size()>CommonSettings::commonSettings.maxPlayerMonsters)
    {
        QMessageBox::warning(this,tr("Warning"),tr("You can't wear this monster more"));
        return;
    }
    if(marketWithdrawInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a withdraw in progress"));
        return;
    }
    CatchChallenger::MarketMonster playerMonster;
    playerMonster.monster=item->data(99).toUInt();
    playerMonster.price=item->data(98).toUInt();
    playerMonster.level=item->data(96).toUInt();
    marketWithdrawMonsterList << playerMonster;
    CatchChallenger::Api_client_real::client->withdrawMarketMonster(item->data(99).toUInt());
    marketWithdrawInSuspend=true;
    delete item;
}
