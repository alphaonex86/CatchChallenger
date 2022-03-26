#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::on_warehouseWithdrawCash_clicked()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    bool ok=true;
    int i;
    if((playerInformations.warehouse_cash-temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount cash to withdraw:"), 0, 0,
                                 static_cast<uint32_t>(playerInformations.warehouse_cash-temp_warehouse_cash), 1, &ok);
    if(!ok || i<=0)
        return;
    temp_warehouse_cash+=i;
    updateTheWareHouseContent();
}

void BaseWindow::on_warehouseDepositCash_clicked()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    bool ok=true;
    int i;
    if((playerInformations.cash+temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount cash to deposite:"), 0, 0,
                                 static_cast<uint32_t>(playerInformations.cash+temp_warehouse_cash), 1, &ok);
    if(!ok || i<=0)
        return;
    temp_warehouse_cash-=i;
    updateTheWareHouseContent();
}

void BaseWindow::on_warehouseWithdrawItem_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerStoredInventory->selectedItems();
    if(itemList.size()!=1)
    {
        if(ui->warehousePlayerStoredInventory->count()==1)
        {
            on_warehousePlayerStoredInventory_itemActivated(ui->warehousePlayerStoredInventory->item(0));
            return;
        }
        QMessageBox::warning(this,tr("Error"),tr("Select an item to transfer!"));
        return;
    }
    on_warehousePlayerStoredInventory_itemActivated(itemList.first());
}

void BaseWindow::on_warehouseDepositItem_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerInventory->selectedItems();
    if(itemList.size()!=1)
    {
        if(ui->warehousePlayerInventory->count()==1)
        {
            on_warehousePlayerInventory_itemActivated(ui->warehousePlayerInventory->item(0));
            return;
        }
        QMessageBox::warning(this,tr("Error"),tr("Select an item to transfer!"));
        return;
    }
    on_warehousePlayerInventory_itemActivated(itemList.first());
}

void BaseWindow::on_warehouseWithdrawMonster_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerStoredMonster->selectedItems();
    if(itemList.size()!=1)
    {
        if(ui->warehousePlayerStoredMonster->count()==1)
        {
            on_warehousePlayerStoredMonster_itemActivated(ui->warehousePlayerStoredMonster->item(0));
            return;
        }
        QMessageBox::warning(this,tr("Error"),tr("Select an monster to transfer!"));
        return;
    }
    on_warehousePlayerStoredMonster_itemActivated(itemList.first());
}

void BaseWindow::on_warehouseDepositMonster_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerMonster->selectedItems();
    if(itemList.size()!=1)
    {
        if(ui->warehousePlayerMonster->count()==1)
        {
            on_warehousePlayerMonster_itemActivated(ui->warehousePlayerMonster->item(0));
            return;
        }
        QMessageBox::warning(this,tr("Error"),tr("Select an monster to transfer!"));
        return;
    }
    on_warehousePlayerMonster_itemActivated(itemList.first());
}

void BaseWindow::on_warehousePlayerInventory_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    uint32_t quantity=0;
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(playerInformations.items.find(itemId)!=playerInformations.items.cend())
        quantity+=playerInformations.items.at(itemId);
    if(change_warehouse_items.find(itemId)!=change_warehouse_items.cend())
        quantity+=change_warehouse_items.at(itemId);
    if(quantity==0)
    {
        error("Error with item quantity into warehousePlayerInventory");
        return;
    }
    bool ok=true;
    int i;
    if(quantity==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:")
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(itemId).name)),
                                 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.find(itemId)!=change_warehouse_items.cend())
        change_warehouse_items[itemId]-=i;
    else
        change_warehouse_items[itemId]=-i;
    if(change_warehouse_items.at(itemId)==0)
        change_warehouse_items.erase(itemId);
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerStoredInventory_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    uint32_t quantity=0;
    const uint16_t &itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(playerInformations.warehouse_items.find(itemId)!=playerInformations.warehouse_items.cend())
        quantity+=playerInformations.warehouse_items.at(itemId);
    if(change_warehouse_items.find(itemId)!=change_warehouse_items.cend())
        quantity-=change_warehouse_items.at(itemId);
    if(quantity==0)
    {
        error("Error with item quantity into warehousePlayerStoredInventory");
        return;
    }
    bool ok=true;
    int i;
    if(quantity==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount %1 to withdraw:")
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(itemId).name)),
                                 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.find(itemId)!=change_warehouse_items.cend())
        change_warehouse_items[itemId]+=i;
    else
        change_warehouse_items[itemId]=i;
    if(change_warehouse_items.at(itemId)==0)
        change_warehouse_items.erase(itemId);
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerMonster_itemActivated(QListWidgetItem *item)
{
    if(ui->warehousePlayerStoredMonster->count()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
    {
        QMessageBox::critical(this,tr("Error"),tr("You have already the maximum of %1 monster into your warehouse").arg(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters));
        return;
    }
    QListWidgetItem *i=ui->warehousePlayerMonster->takeItem(ui->warehousePlayerMonster->row(item));
    ui->warehousePlayerStoredMonster->addItem(i);
}

void BaseWindow::on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item)
{
    if(ui->warehousePlayerMonster->count()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        QMessageBox::critical(this,tr("Error"),tr("You have already the maximum of %1 monster with you").arg(CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters));
        return;
    }
    QListWidgetItem *i=ui->warehousePlayerStoredMonster->takeItem(ui->warehousePlayerStoredMonster->row(item));
    ui->warehousePlayerMonster->addItem(i);
}

std::vector<PlayerMonster> BaseWindow::warehouseMonsterOnPlayer() const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    std::vector<PlayerMonster> warehouseMonsterOnPlayerList;
    {
        const std::vector<PlayerMonster> &playerMonster=client->getPlayerMonster();
        unsigned int index=0;
        while(index<playerMonster.size())
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=
                    CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<playerInformations.warehouse_playerMonster.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=
                    CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    return warehouseMonsterOnPlayerList;
}

void BaseWindow::on_toolButton_quit_warehouse_clicked()
{
    change_warehouse_items.clear();
    temp_warehouse_cash=0;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_warehouseValidate_clicked()
{
    std::vector<uint8_t> withdrawMonsters;
    for(int i = 0; i < ui->warehousePlayerMonster->count(); ++i)
    {
        const QListWidgetItem *item=ui->warehousePlayerMonster->item(i);
        if(item->data(98).toUInt()==1)
            withdrawMonsters.push_back(item->data(99).toUInt());
    }
    std::sort(withdrawMonsters.rbegin(),withdrawMonsters.rend());
    std::vector<uint8_t> depositeMonsters;
    for(int i = 0; i < ui->warehousePlayerStoredMonster->count(); ++i)
    {
        const QListWidgetItem *item=ui->warehousePlayerStoredMonster->item(i);
        if(item->data(98).toUInt()==0)
            depositeMonsters.push_back(item->data(99).toUInt());
    }
    std::sort(depositeMonsters.rbegin(),depositeMonsters.rend());

    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    {
        std::vector<std::pair<uint16_t,uint32_t> > withdrawItems;
        std::vector<std::pair<uint16_t,uint32_t> > depositeItems;
        for (const auto &n : change_warehouse_items)
            if(n.second>0)
                withdrawItems.push_back(std::pair<uint16_t,int32_t>(n.first,n.second));
            else if(n.second<0)
                depositeItems.push_back(std::pair<uint16_t,int32_t>(n.first,-n.second));
        uint64_t withdrawCash=0;
        uint64_t depositeCash=0;
        if(temp_warehouse_cash>0)
            withdrawCash=temp_warehouse_cash;
        else
            depositeCash=-temp_warehouse_cash;
        if(!client->wareHouseStore(withdrawCash,depositeCash,withdrawItems,depositeItems,withdrawMonsters,depositeMonsters))
        {
            emit error("Bug on warehouse API call");
            return;
        }
    }
    //validate the change here
    if(temp_warehouse_cash>0)
        addCash(static_cast<uint32_t>(temp_warehouse_cash));
    else
        removeCash(static_cast<uint32_t>(-temp_warehouse_cash));
    playerInformations.warehouse_cash-=temp_warehouse_cash;
    {
        for (const auto &n : change_warehouse_items) {
            if(n.second>0)
            {
                if(playerInformations.items.find(n.first)!=playerInformations.items.cend())
                    playerInformations.items[n.first]+=n.second;
                else
                    playerInformations.items[n.first]=n.second;
                playerInformations.warehouse_items[n.first]-=n.second;
                if(playerInformations.warehouse_items.at(n.first)==0)
                    playerInformations.warehouse_items.erase(n.first);
            }
            if(n.second<0)
            {
                playerInformations.items[n.first]+=n.second;
                if(playerInformations.items.at(n.first)==0)
                    playerInformations.items.erase(n.first);
                if(playerInformations.warehouse_items.find(n.first)!=playerInformations.warehouse_items.cend())
                    playerInformations.warehouse_items[n.first]-=n.second;
                else
                    playerInformations.warehouse_items[n.first]=-n.second;
            }
        }
        load_inventory();
        load_plant_inventory();
    }
    {
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            if(withdrawMonsters.at(index)>=playerInformations.warehouse_playerMonster.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->addPlayerMonster(playerInformations.warehouse_playerMonster.at(withdrawMonsters.at(index)));
            index++;
        }
        index=0;
        while(index<depositeMonsters.size())
        {
            if(depositeMonsters.at(index)>=playerInformations.playerMonster.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->addPlayerMonsterWarehouse(playerInformations.playerMonster.at(depositeMonsters.at(index)));
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            if(depositeMonsters.at(index)>=playerInformations.playerMonster.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->removeMonsterByPosition(depositeMonsters.at(index));
            index++;
        }
        index=0;
        while(index<withdrawMonsters.size())
        {
            if(withdrawMonsters.at(index)>=playerInformations.warehouse_playerMonster.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->removeMonsterWarehouseByPosition(withdrawMonsters.at(index));
            index++;
        }
    }

    load_monsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You have correctly withdraw/deposit your goods").toStdString());
}

void BaseWindow::updateTheWareHouseContent()
{
    if(!haveInventory || !datapackIsParsed)
        return;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();

    //inventory
    {
        ui->warehousePlayerInventory->clear();
        auto i=playerInformations.items.begin();
        while(i!=playerInformations.items.cend())
        {
            int32_t quantity=i->second;
            if(change_warehouse_items.find(i->first)!=change_warehouse_items.cend())
                quantity+=change_warehouse_items.at(i->first);
            if(quantity>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(i->first,quantity));
            ++i;
        }
        for (const auto &j : change_warehouse_items) {
            if(playerInformations.items.find(j.first)==playerInformations.items.cend() && j.second>0)
                ui->warehousePlayerInventory->addItem(itemToGraphic(j.first,j.second));
        }
    }

    qDebug() << QStringLiteral("ui->warehousePlayerInventory icon size: %1x%2").arg(ui->warehousePlayerInventory->iconSize().width()).arg(ui->warehousePlayerInventory->iconSize().height());

    //inventory warehouse
    {
        ui->warehousePlayerStoredInventory->clear();
        auto i=playerInformations.warehouse_items.begin();
        while(i!=playerInformations.warehouse_items.cend())
        {
            int32_t quantity=i->second;
            if(change_warehouse_items.find(i->first)!=change_warehouse_items.cend())
                quantity-=change_warehouse_items.at(i->first);
            if(quantity>0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(i->first,quantity));
            ++i;
        }
        for (const auto &j : change_warehouse_items) {
            if(playerInformations.warehouse_items.find(j.first)==playerInformations.warehouse_items.cend() && j.second<0)
                ui->warehousePlayerStoredInventory->addItem(itemToGraphic(j.first,-j.second));
        }
    }

    //cash
    ui->warehousePlayerCash->setText(tr("Cash: %1").arg(playerInformations.cash+temp_warehouse_cash));
    ui->warehousePlayerStoredCash->setText(tr("Cash: %1").arg(playerInformations.warehouse_cash-temp_warehouse_cash));

    //do before because the dispatch put into random of it
    ui->warehousePlayerStoredMonster->clear();
    ui->warehousePlayerMonster->clear();

    //monster
    {
        const std::vector<PlayerMonster> &playerMonster=client->getPlayerMonster();
        unsigned int index=0;
        while(index<playerMonster.size())
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).description));
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front);
                item->setData(98,0);
                item->setData(99,index);
                ui->warehousePlayerMonster->addItem(item);
            }
            index++;
        }
    }

    //monster warehouse
    {
        unsigned int index=0;
        while(index<playerInformations.warehouse_playerMonster.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster).description));
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front);
                item->setData(98,1);
                item->setData(99,index);
                ui->warehousePlayerStoredMonster->addItem(item);
            }
            index++;
        }
    }

    //set the button enabled
    ui->warehouseWithdrawCash->setEnabled((playerInformations.warehouse_cash-temp_warehouse_cash)>0);
    ui->warehouseDepositCash->setEnabled((playerInformations.cash+temp_warehouse_cash)>0);
    ui->warehouseDepositItem->setEnabled(ui->warehousePlayerInventory->count()>0);
    ui->warehouseWithdrawItem->setEnabled(ui->warehousePlayerStoredInventory->count()>0);
    ui->warehouseDepositMonster->setEnabled(ui->warehousePlayerMonster->count()>1);
    ui->warehouseWithdrawMonster->setEnabled(ui->warehousePlayerStoredMonster->count()>0);
}
