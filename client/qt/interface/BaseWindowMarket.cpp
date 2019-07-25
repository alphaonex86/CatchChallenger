#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../QtDatapackClientLoader.h"
#include "../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList)
{
    ui->marketWithdraw->setVisible(true);
    ui->marketStat->setText(tr("Cash to withdraw: %1$").arg(price));
    unsigned int index;
    //the object list
    ui->marketObject->clear();
    index=0;
    while(index<marketObjectList.size())
    {
        const MarketObject &marketObject=marketObjectList.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        updateMarketObject(item,marketObject);
        ui->marketObject->addItem(item);
        index++;
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
        std::string price;
        if(marketMonster.price>0)
            price=tr("Price: %1$").arg(marketMonster.price).toStdString();
        else
            price=tr("Price: Free").toStdString();
        if(QtDatapackClientLoader::datapackLoader.monsterExtra.find(marketMonster.monster)!=QtDatapackClientLoader::datapackLoader.monsterExtra.cend())
        {
            item->setIcon(QtDatapackClientLoader::datapackLoader.QtmonsterExtra.at(marketMonster.monster).thumb);
            item->setText(QStringLiteral("%1 level %2\n%3").arg(
                              QString::fromStdString(QtDatapackClientLoader::datapackLoader.monsterExtra.at(marketMonster.monster).name)
                              )
                          .arg(marketMonster.level).arg(QString::fromStdString(price)));
            item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader.monsterExtra.at(marketMonster.monster).description));
        }
        else
        {
            item->setIcon(QtDatapackClientLoader::datapackLoader.defaultInventoryImage());
            item->setText(QStringLiteral("Unknown item with id %1 level %2\n%3").arg(marketMonster.monster).arg(marketMonster.level)
                          .arg(QString::fromStdString(price)));
        }
        ui->marketMonster->addItem(item);
        index++;
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
    //the own monster list
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
    std::string price;
    if(marketMonster.price>0)
        price=tr("Price: %1$").arg(marketMonster.price).toStdString();
    else
        price=tr("Price: Free").toStdString();
    if(QtDatapackClientLoader::datapackLoader.monsterExtra.find(marketMonster.monster)!=QtDatapackClientLoader::datapackLoader.monsterExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader.QtmonsterExtra.at(marketMonster.monster).thumb);
        item->setText(QStringLiteral("%1 level %2\n%3")
                      .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader.monsterExtra.at(marketMonster.monster).name))
                      .arg(marketMonster.level)
                      .arg(QString::fromStdString(price)));
        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader.monsterExtra.at(marketMonster.monster).description));
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/small.png"));
        item->setText(QStringLiteral("Unknown monster with id %1 level %2\n%3").arg(marketMonster.monster)
                      .arg(marketMonster.level).arg(QString::fromStdString(price)));
    }
    ui->marketOwnMonster->addItem(item);
}

void BaseWindow::marketBuy(const bool &success)
{
    marketBuyInSuspend=false;
    if(!success)
    {
        addCash(marketBuyCashInSuspend);
        marketBuyObjectList.erase(marketBuyObjectList.cbegin());
        QMessageBox::warning(this,tr("Warning"),tr("Your buy in the market have failed"));
    }
    else
    {
        std::unordered_map<uint16_t,uint32_t> items;
        items[marketBuyObjectList.front().first]=marketBuyObjectList.front().second;
        add_to_inventory(items);
        marketBuyObjectList.erase(marketBuyObjectList.cend());
    }
    marketBuyCashInSuspend=0;
}

void BaseWindow::marketBuyMonster(const PlayerMonster &playerMonster)
{
    marketBuyInSuspend=false;
    fightEngine.addPlayerMonster(playerMonster);
    load_monsters();
    marketBuyCashInSuspend=0;
}

void BaseWindow::marketPut(const bool &success)
{
    if(!success)
    {
        if(!marketPutObjectInSuspendList.empty())
            add_to_inventory(marketPutObjectInSuspendList,false);
        if(!marketPutMonsterList.empty())
        {
            fightEngine.insertPlayerMonster(marketPutMonsterPlaceList.front(),marketPutMonsterList.front());
            load_monsters();
        }
        QMessageBox::warning(this,tr("Warning"),tr("Unable to put into the market"));
    }
    else
    {
        if(!marketPutMonsterList.empty())
        {
            MarketMonster marketMonster;
            marketMonster.price=marketPutCashInSuspend;
            marketMonster.level=marketPutMonsterList.front().level;
            marketMonster.monster=marketPutMonsterList.front().monster;
            //marketMonster.monsterId=marketPutMonsterList.first().id;
            addOwnMonster(marketMonster);
        }
        if(!marketPutObjectInSuspendList.empty())
        {
            MarketObject marketObject;
            marketObject.price=marketPutCashInSuspend;
            //marketObject.marketObjectId=0;
            marketObject.item=marketPutObjectInSuspendList.front().first;
            marketObject.quantity=marketPutObjectInSuspendList.front().second;
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
    addCash(static_cast<uint32_t>(cash));
    ui->marketStat->setText(tr("Cash to withdraw: %1$").arg(0));
}

void BaseWindow::marketWithdrawCanceled()
{
    QMessageBox::warning(this,tr("Warning"),tr("Unable to withdraw from the market"));
    marketWithdrawInSuspend=false;
    if(!marketWithdrawObjectList.empty())
    {
        QListWidgetItem *item=new QListWidgetItem();
        updateMarketObject(item,marketWithdrawObjectList.front());
        ui->marketOwnObject->addItem(item);
    }
    if(!marketWithdrawMonsterList.empty())
        addOwnMonster(marketWithdrawMonsterList.front());
    marketWithdrawObjectList.clear();
    marketWithdrawMonsterList.clear();
}

void BaseWindow::marketWithdrawObject(const uint16_t &objectId,const uint32_t &quantity)
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
    fightEngine.addPlayerMonster(playerMonster);
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
    client->recoverMarketCash();
}

void BaseWindow::updateMarketObject(QListWidgetItem *item,const MarketObject &marketObject)
{
    item->setData(99,marketObject.marketObjectUniqueId);
    item->setData(98,marketObject.quantity);
    item->setData(97,(quint64)marketObject.price);
    item->setData(95,marketObject.item);
    std::string price;
    if(marketObject.price>0)
        price=tr("Price: %1$").arg(marketObject.price).toStdString();
    else
        price=tr("Price: Free").toStdString();
    std::string quantity;
    if(marketObject.quantity>1)
        quantity=", "+tr("quantity: %1").arg(marketObject.quantity).toStdString();
    if(QtDatapackClientLoader::datapackLoader.itemsExtra.find(marketObject.item)!=QtDatapackClientLoader::datapackLoader.itemsExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader.QtitemsExtra.at(marketObject.item).image);
        item->setText(QStringLiteral("%1%2\n%3").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader.itemsExtra.at(marketObject.item).name))
                      .arg(QString::fromStdString(quantity)).arg(QString::fromStdString(price)));
        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader.itemsExtra.at(marketObject.item).description));
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/small.png"));
        item->setText(QStringLiteral("Unknown item with id %1%2\n%3")
                      .arg(marketObject.item)
                      .arg(QString::fromStdString(quantity))
                      .arg(QString::fromStdString(price)));
    }
}

void BaseWindow::on_marketObject_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(marketBuyInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a buy in progress"));
        return;
    }
    uint32_t priceQuantity;
    if(item->data(97).toUInt()>0)
        priceQuantity=static_cast<uint32_t>(playerInformations.cash)/static_cast<uint32_t>(item->data(97).toUInt());
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
    client->buyMarketObject(item->data(99).toUInt(),quantity);

    /*updateMarketObject() define:
    item->setData(99,marketObject.marketObjectUniqueId);
    item->setData(98,marketObject.quantity);
    item->setData(97,(quint64)marketObject.price);
    item->setData(95,marketObject.item);*/

    std::pair<uint32_t,uint32_t> newEntry;
    newEntry.first=item->data(95).toUInt();
    newEntry.second=quantity;
    marketBuyObjectList.push_back(newEntry);
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
        marketObject.item=static_cast<uint16_t>(item->data(95).toUInt());
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
    client->withdrawMarketObject(static_cast<uint16_t>(item->data(95).toUInt()),item->data(98).toUInt());
    marketWithdrawInSuspend=true;
    MarketObject marketObject;
    marketObject.marketObjectUniqueId=item->data(99).toUInt();
    marketObject.quantity=item->data(98).toUInt();
    marketObject.price=item->data(97).toUInt();
    marketObject.item=static_cast<uint16_t>(item->data(95).toUInt());
    marketWithdrawObjectList.push_back(marketObject);
    item->setData(98,item->data(98).toUInt()-quantity);
    if(item->data(98).toUInt()==0)
        delete item;
    else
        updateMarketObject(item,marketObject);
}

void BaseWindow::on_marketMonster_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(marketBuyInSuspend)
    {
        QMessageBox::warning(this,tr("Error"),tr("You have aleady a buy in progress"));
        return;
    }
    uint32_t priceQuantity;
    if(item->data(98).toUInt()>0)
        priceQuantity=static_cast<uint32_t>(playerInformations.cash)/static_cast<uint32_t>(item->data(98).toUInt());
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
    client->buyMarketMonster(item->data(99).toUInt());
    delete item;
}

void BaseWindow::on_marketOwnMonster_itemActivated(QListWidgetItem *item)
{
    if(fightEngine.getPlayerMonster().size()>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
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
    playerMonster.monster=static_cast<uint16_t>(item->data(99).toUInt());
    playerMonster.price=item->data(98).toUInt();
    playerMonster.level=static_cast<uint8_t>(item->data(96).toUInt());
    marketWithdrawMonsterList.push_back(playerMonster);
    client->withdrawMarketMonster(item->data(99).toUInt());
    marketWithdrawInSuspend=true;
    delete item;
}

void BaseWindow::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
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

    const QPixmap skin(getFrontSkin(skinInt));
    if(!skin.isNull())
    {
        //reset the other player info
        ui->tradePlayerImage->setVisible(true);
        ui->tradePlayerPseudo->setVisible(true);
        ui->tradeOtherImage->setVisible(true);
        ui->tradeOtherPseudo->setVisible(true);
        ui->tradeOtherImage->setPixmap(skin);
        ui->tradeOtherPseudo->setText(QString::fromStdString(pseudo));
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
    ui->tradeOtherStat->setText(tr("The other player don't have validate their selection"));
}

void BaseWindow::tradeCanceledByOther()
{
    if(ui->stackedWidget->currentWidget()!=ui->page_trade)
        return;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled your trade request").toStdString());
    addCash(ui->tradePlayerCash->value());
    add_to_inventory(tradeCurrentObjects,false);
    fightEngine.addPlayerMonster(tradeCurrentMonsters);
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
    showTip(tr("Your trade is successfull").toStdString());
    add_to_inventory(tradeOtherObjects);
    addCash(ui->tradeOtherCash->value());
    removeCash(ui->tradePlayerCash->value());
    tradeEvolutionMonsters=fightEngine.addPlayerMonster(tradeOtherMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
    checkEvolution();
}

void BaseWindow::tradeAddTradeCash(const uint64_t &cash)
{
    ui->tradeOtherCash->setValue(ui->tradeOtherCash->value()+static_cast<uint32_t>(cash));
}

void BaseWindow::tradeAddTradeObject(const uint16_t &item,const uint32_t &quantity)
{
    if(tradeOtherObjects.find(item)!=tradeOtherObjects.cend())
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    ui->tradeOtherItems->clear();
    for(const auto &n : tradeOtherObjects) {
        ui->tradeOtherItems->addItem(itemToGraphic(n.first,n.second));
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    for(const auto &n : tradeCurrentObjects) {
        ui->tradePlayerItems->addItem(itemToGraphic(n.first,n.second));
    }
}
