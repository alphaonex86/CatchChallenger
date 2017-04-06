#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonSettingsServer.h"

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
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount cash to withdraw:"), 0, 0, playerInformations.warehouse_cash-temp_warehouse_cash, 1, &ok);
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
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount cash to deposite:"), 0, 0, playerInformations.cash+temp_warehouse_cash, 1, &ok);
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
    uint32_t id=item->data(99).toUInt();
    if(playerInformations.items.find(id)!=playerInformations.items.cend())
        quantity+=playerInformations.items.at(id);
    if(change_warehouse_items.contains(id))
        quantity+=change_warehouse_items.value(id);
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
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(id).name), 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.contains(id))
        change_warehouse_items[id]-=i;
    else
        change_warehouse_items[id]=-i;
    if(change_warehouse_items.value(id)==0)
        change_warehouse_items.remove(id);
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerStoredInventory_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    uint32_t quantity=0;
    uint32_t id=item->data(99).toUInt();
    if(playerInformations.warehouse_items.find(id)!=playerInformations.warehouse_items.cend())
        quantity+=playerInformations.warehouse_items.at(id);
    if(change_warehouse_items.contains(id))
        quantity-=change_warehouse_items.value(id);
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
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount %1 to withdraw:").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(id).name), 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.contains(id))
        change_warehouse_items[id]+=i;
    else
        change_warehouse_items[id]=i;
    if(change_warehouse_items.value(id)==0)
        change_warehouse_items.remove(id);
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerMonster_itemActivated(QListWidgetItem *item)
{
    int pos=ui->warehousePlayerMonster->row(item);
    if(pos<0)
        return;
    bool remain_valid_monster=false;
    int index=0;
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    while(index<warehouseMonsterOnPlayerList.size())
    {
        const PlayerMonster &monster=warehouseMonsterOnPlayerList.at(index);
        if(index!=pos && monster.egg_step==0 && monster.hp>0)
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
    monster_to_withdraw.removeOne(pos);
    monster_to_deposit <<  pos;
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item)
{
    int pos=ui->warehousePlayerMonster->row(item);
    if(pos<0)
        return;
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    if(warehouseMonsterOnPlayerList.size()>CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't wear more monster!"));
        return;
    }
    monster_to_deposit.removeOne(pos);
    monster_to_withdraw <<  pos;
    updateTheWareHouseContent();
}

QList<PlayerMonster> BaseWindow::warehouseMonsterOnPlayer() const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    QList<PlayerMonster> warehouseMonsterOnPlayerList;
    {
        const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
        int index=0;
        int size=playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend() && !monster_to_deposit.contains(index))
                warehouseMonsterOnPlayerList << monster;
            index++;
        }
    }
    {
        int index=0;
        int size=playerInformations.warehouse_playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=playerInformations.warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend() && monster_to_withdraw.contains(index))
                warehouseMonsterOnPlayerList << monster;
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
        QList<QPair<uint16_t,int32_t> > change_warehouse_items_list;
        QHash<uint16_t,int32_t>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            change_warehouse_items_list << QPair<uint16_t,int32_t>(i.key(),i.value());
            ++i;
        }
        client->wareHouseStore(temp_warehouse_cash,change_warehouse_items_list,monster_to_withdraw,monster_to_deposit);
    }
    //validate the change here
    if(temp_warehouse_cash>0)
        addCash(temp_warehouse_cash);
    else
        removeCash(-temp_warehouse_cash);
    playerInformations.warehouse_cash-=temp_warehouse_cash;
    {
        QHash<uint16_t,int32_t>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            if(i.value()>0)
            {
                if(playerInformations.items.find(i.key())!=playerInformations.items.cend())
                    playerInformations.items[i.key()]+=i.value();
                else
                    playerInformations.items[i.key()]=i.value();
                playerInformations.warehouse_items[i.key()]-=i.value();
                if(playerInformations.warehouse_items.at(i.key())==0)
                    playerInformations.warehouse_items.erase(i.key());
            }
            if(i.value()<0)
            {
                playerInformations.items[i.key()]+=i.value();
                if(playerInformations.items.at(i.key())==0)
                    playerInformations.items.erase(i.key());
                if(playerInformations.warehouse_items.find(i.key())!=playerInformations.warehouse_items.cend())
                    playerInformations.warehouse_items[i.key()]-=i.value();
                else
                    playerInformations.warehouse_items[i.key()]=-i.value();
            }
            ++i;
        }
        load_inventory();
        load_plant_inventory();
    }
    {
        QList<PlayerMonster> playerMonsterToWithdraw;
        int index=0;
        while(index<monster_to_withdraw.size())
        {
            unsigned int sub_index=0;
            while(sub_index<playerInformations.warehouse_playerMonster.size())
            {
                if(sub_index==monster_to_withdraw.at(index))
                {
                    playerMonsterToWithdraw << playerInformations.warehouse_playerMonster.at(sub_index);
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
        int index=0;
        while(index<monster_to_deposit.size())
        {
            const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
            const uint8_t &monsterPosition=monster_to_deposit.at(index);
            playerInformations.warehouse_playerMonster.push_back(playerMonster.at(monsterPosition));
            fightEngine.removeMonsterByPosition(monsterPosition);
            index++;
        }
    }
    load_monsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You have correctly withdraw/deposit your goods"));
}
