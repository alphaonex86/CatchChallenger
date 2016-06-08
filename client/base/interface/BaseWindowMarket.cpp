#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::marketList(const uint64_t &price,const QList<MarketObject> &marketObjectList,const QList<MarketMonster> &marketMonsterList,const QList<MarketObject> &marketOwnObjectList,const QList<MarketMonster> &marketOwnMonsterList)
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
        item->setData(99,marketMonster.index);
        item->setData(98,(quint64)marketMonster.price);
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
    item->setData(99,marketMonster.index);
    item->setData(98,(quint64)marketMonster.price);
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
        QHash<uint16_t,uint32_t> items;
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
            //marketMonster.monsterId=marketPutMonsterList.first().id;
            addOwnMonster(marketMonster);
        }
        if(!marketPutObjectInSuspendList.isEmpty())
        {
            MarketObject marketObject;
            marketObject.price=marketPutCashInSuspend;
            //marketObject.marketObjectId=0;
            marketObject.item=marketPutObjectInSuspendList.first().first;
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

void BaseWindow::marketGetCash(const uint64_t &cash)
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

void BaseWindow::marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity)
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
    item->setData(99,marketObject.marketObjectUniqueId);
    item->setData(98,marketObject.quantity);
    item->setData(97,(quint64)marketObject.price);
    item->setData(95,marketObject.item);
    QString price;
    if(marketObject.price>0)
        price=tr("Price: %1$").arg(marketObject.price);
    else
        price=tr("Price: Free");
    QString quantity;
    if(marketObject.quantity>1)
        quantity=QStringLiteral(", ")+tr("quantity: %1").arg(marketObject.quantity);
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(marketObject.item))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.item).image);
        item->setText(QStringLiteral("%1%2\n%3").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.item).name).arg(quantity).arg(price));
        item->setToolTip(DatapackClientLoader::datapackLoader.itemsExtra.value(marketObject.item).description);
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/small.png"));
        item->setText(QStringLiteral("Unknown item with id %1%2\n%3").arg(marketObject.item).arg(quantity).arg(price));
    }
}

void BaseWindow::on_marketObject_itemActivated(QListWidgetItem *item)
{
    if(marketBuyInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a buy in progress"));
        return;
    }
    uint32_t priceQuantity;
    if(item->data(97).toUInt()>0)
        priceQuantity=cash/item->data(97).toUInt();
    else
        priceQuantity=item->data(98).toUInt();
    if(priceQuantity>item->data(98).toUInt())
        priceQuantity=item->data(98).toUInt();
    uint32_t quantity=priceQuantity;
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
    QPair<uint32_t,uint32_t> newEntry;
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
        marketObject.marketObjectUniqueId=item->data(99).toUInt();
        marketObject.quantity=item->data(98).toUInt();
        marketObject.price=item->data(97).toUInt();
        marketObject.item=item->data(95).toUInt();
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
    uint32_t quantity=1;
    if(item->data(98).toUInt()>1)
    {
        quantity=QInputDialog::getInt(this,tr("Quantity"),tr("How many item wish you withdraw?"),item->data(98).toUInt(),1,item->data(98).toUInt(),1,&ok);
        if(!ok)
            return;
    }
    CatchChallenger::Api_client_real::client->withdrawMarketObject(item->data(95).toUInt(),item->data(98).toUInt());
    marketWithdrawInSuspend=true;
    MarketObject marketObject;
    marketObject.marketObjectUniqueId=item->data(99).toUInt();
    marketObject.quantity=item->data(98).toUInt();
    marketObject.price=item->data(97).toUInt();
    marketObject.item=item->data(95).toUInt();
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
    uint32_t priceQuantity;
    if(item->data(98).toUInt()>0)
        priceQuantity=cash/item->data(98).toUInt();
    else
        priceQuantity=1;
    if(priceQuantity>1)
        priceQuantity=1;
    uint32_t quantity=priceQuantity;
    marketBuyCashInSuspend=item->data(98).toUInt();
    removeCash(marketBuyCashInSuspend);
    if(quantity==0)
    {
        QMessageBox::warning(this,tr("Error"),tr("Have not cash to buy it"));
        return;
    }
    CatchChallenger::Api_client_real::client->buyMarketMonsterByPosition(item->data(99).toUInt());
    delete item;
}

void BaseWindow::on_marketOwnMonster_itemActivated(QListWidgetItem *item)
{
    if(ClientFightEngine::fightEngine.getPlayerMonster().size()>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
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

void BaseWindow::tradeAcceptedByOther(const QString &pseudo,const uint8_t &skinInt)
{
    ui->stackedWidget->setCurrentWidget(ui->page_trade);
    tradeOtherStat=TradeOtherStat_InWait;
    //reset the current player info
    ui->tradePlayerCash->setMinimum(0);
    ui->tradePlayerCash->setValue(0);
    ui->tradePlayerItems->clear();
    ui->tradePlayerMonsters->clear();
    ui->tradePlayerCash->setEnabled(true);
    ui->tradePlayerItems->setEnabled(true);
    ui->tradePlayerMonsters->setEnabled(true);
    ui->tradeAddItem->setEnabled(true);
    ui->tradeAddMonster->setEnabled(true);
    ui->tradeValidate->setEnabled(true);

    const QPixmap skin(getFrontSkinPath(skinInt));
    if(!skin.isNull())
    {
        //reset the other player info
        ui->tradePlayerImage->setVisible(true);
        ui->tradePlayerPseudo->setVisible(true);
        ui->tradeOtherImage->setVisible(true);
        ui->tradeOtherPseudo->setVisible(true);
        ui->tradeOtherImage->setPixmap(skin);
        ui->tradeOtherPseudo->setText(pseudo);
    }
    else
    {
        ui->tradePlayerImage->setVisible(false);
        ui->tradePlayerPseudo->setVisible(false);
        ui->tradeOtherImage->setVisible(false);
        ui->tradeOtherPseudo->setVisible(false);
    }
    ui->tradeOtherCash->setValue(0);
    ui->tradeOtherItems->clear();
    ui->tradeOtherMonsters->clear();
    ui->tradeOtherStat->setText(tr("The other player have not validation their selection"));
}

void BaseWindow::tradeCanceledByOther()
{
    if(ui->stackedWidget->currentWidget()!=ui->page_trade)
        return;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled your trade request"));
    addCash(ui->tradePlayerCash->value());
    add_to_inventory(tradeCurrentObjects,false);
    CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(tradeCurrentMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeFinishedByOther()
{
    tradeOtherStat=TradeOtherStat_Accepted;
    if(!ui->tradeValidate->isEnabled())
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
        ui->tradeOtherStat->setText(tr("The other player have validated the selection"));
}

void BaseWindow::tradeValidatedByTheServer()
{
    if(ui->stackedWidget->currentWidget()==ui->page_trade)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Your trade is successfull"));
    add_to_inventory(tradeOtherObjects);
    addCash(ui->tradeOtherCash->value());
    tradeEvolutionMonsters=CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(tradeOtherMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
    checkEvolution();
}

void BaseWindow::tradeAddTradeCash(const uint64_t &cash)
{
    ui->tradeOtherCash->setValue(ui->tradeOtherCash->value()+cash);
}

void BaseWindow::tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity)
{
    if(tradeOtherObjects.contains(item))
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    ui->tradeOtherItems->clear();
    QHashIterator<uint16_t,uint32_t> i(tradeOtherObjects);
    while (i.hasNext()) {
        i.next();
        ui->tradeOtherItems->addItem(itemToGraphic(i.key(),i.value()));
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    QHashIterator<uint16_t,uint32_t> i(tradeCurrentObjects);
    while (i.hasNext()) {
        i.next();
        ui->tradePlayerItems->addItem(itemToGraphic(i.key(),i.value()));
    }
}
