#include "BaseWindow.hpp"
#include "ui_BaseWindow.h"
#include "../QtDatapackClientLoader.hpp"
#include "../fight/interface/ClientFightEngine.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"

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
              .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)),
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
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(itemId).name)),
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
    int pos=ui->warehousePlayerMonster->row(item);
    if(pos<0)
        return;
    bool remain_valid_monster=false;
    unsigned int index=0;
    std::vector<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    while(index<warehouseMonsterOnPlayerList.size())
    {
        const PlayerMonster &monster=warehouseMonsterOnPlayerList.at(index);
        if(index!=(unsigned int)pos && monster.egg_step==0 && monster.hp>0)
        {
            remain_valid_monster=true;
            break;
        }
        index++;
    }
    if(!remain_valid_monster)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't deposite your last alive monster!"));
        return;
    }
    vectorremoveOne(monster_to_withdraw,static_cast<uint32_t>(pos));
    monster_to_deposit.push_back(pos);
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item)
{
    int pos=ui->warehousePlayerMonster->row(item);
    if(pos<0)
        return;
    std::vector<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    if(warehouseMonsterOnPlayerList.size()>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't wear more monster!"));
        return;
    }
    vectorremoveOne(monster_to_deposit,static_cast<uint32_t>(pos));
    monster_to_withdraw.push_back(pos);
    updateTheWareHouseContent();
}

std::vector<PlayerMonster> BaseWindow::warehouseMonsterOnPlayer() const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    std::vector<PlayerMonster> warehouseMonsterOnPlayerList;
    {
        const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
        unsigned int index=0;
        while(index<playerMonster.size())
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=
                    CatchChallenger::CommonDatapack::commonDatapack.monsters.cend() &&
                    !vectorcontainsAtLeastOne(monster_to_deposit,index))
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<playerInformations.warehouse_playerMonster.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=
                    CatchChallenger::CommonDatapack::commonDatapack.monsters.cend() &&
                    vectorcontainsAtLeastOne(monster_to_withdraw,index))
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    return warehouseMonsterOnPlayerList;
}

void BaseWindow::on_toolButton_quit_warehouse_clicked()
{
    monster_to_withdraw.clear();
    monster_to_deposit.clear();
    change_warehouse_items.clear();
    temp_warehouse_cash=0;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_warehouseValidate_clicked()
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    {
        std::vector<std::pair<uint16_t,int32_t> > change_warehouse_items_list;
        for (const auto &n : change_warehouse_items)
            change_warehouse_items_list.push_back(std::pair<uint16_t,int32_t>(n.first,n.second));
        client->wareHouseStore(temp_warehouse_cash,change_warehouse_items_list,monster_to_withdraw,monster_to_deposit);
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
        std::vector<PlayerMonster> playerMonsterToWithdraw;
        unsigned int index=0;
        while(index<monster_to_withdraw.size())
        {
            unsigned int sub_index=0;
            while(sub_index<playerInformations.warehouse_playerMonster.size())
            {
                if(sub_index==monster_to_withdraw.at(index))
                {
                    playerMonsterToWithdraw.push_back(playerInformations.warehouse_playerMonster.at(sub_index));
                    playerInformations.warehouse_playerMonster.erase(playerInformations.warehouse_playerMonster.cbegin()+sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
        fightEngine.addPlayerMonster(playerMonsterToWithdraw);
    }
    {
        unsigned int index=0;
        while(index<monster_to_deposit.size())
        {
            const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
            const uint8_t &monsterPosition=static_cast<uint8_t>(monster_to_deposit.at(index));
            playerInformations.warehouse_playerMonster.push_back(playerMonster.at(monsterPosition));
            fightEngine.removeMonsterByPosition(monsterPosition);
            index++;
        }
    }
    load_monsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You have correctly withdraw/deposit your goods").toStdString());
}
