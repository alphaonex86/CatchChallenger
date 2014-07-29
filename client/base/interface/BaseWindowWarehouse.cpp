#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "DatapackClientLoader.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::on_warehouseWithdrawCash_clicked()
{
    bool ok=true;
    int i;
    if((warehouse_cash-temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount cash to withdraw:"), 0, 0, warehouse_cash-temp_warehouse_cash, 1, &ok);
    if(!ok || i<=0)
        return;
    temp_warehouse_cash+=i;
    updateTheWareHouseContent();
}

void BaseWindow::on_warehouseDepositCash_clicked()
{
    bool ok=true;
    int i;
    if((cash+temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount cash to deposite:"), 0, 0, cash+temp_warehouse_cash, 1, &ok);
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
    quint32 quantity=0;
    quint32 id=item->data(99).toUInt();
    if(items.contains(id))
        quantity+=items.value(id);
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
    quint32 quantity=0;
    quint32 id=item->data(99).toUInt();
    if(warehouse_items.contains(id))
        quantity+=warehouse_items.value(id);
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
    quint32 id=item->data(99).toUInt();
    bool remain_valid_monster=false;
    int index=0;
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    while(index<warehouseMonsterOnPlayerList.size())
    {
        const PlayerMonster &monster=warehouseMonsterOnPlayerList.at(index);
        if(monster.id!=id && monster.egg_step==0 && monster.hp>0)
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
    monster_to_withdraw.removeOne(id);
    monster_to_deposit <<  id;
    updateTheWareHouseContent();
}

void BaseWindow::on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item)
{
    quint32 id=item->data(99).toUInt();
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    if(warehouseMonsterOnPlayerList.size()>CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't wear more monster!"));
        return;
    }
    monster_to_deposit.removeOne(id);
    monster_to_withdraw <<  id;
    updateTheWareHouseContent();
}

QList<PlayerMonster> BaseWindow::warehouseMonsterOnPlayer() const
{
    QList<PlayerMonster> warehouseMonsterOnPlayerList;
    {
        const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
        int index=0;
        int size=playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster) && !monster_to_deposit.contains(monster.id))
                warehouseMonsterOnPlayerList << monster;
            index++;
        }
    }
    {
        int index=0;
        int size=warehouse_playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster) && monster_to_withdraw.contains(monster.id))
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
    warehouse_cash=0;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_warehouseValidate_clicked()
{
    {
        QList<QPair<quint16,qint32> > change_warehouse_items_list;
        QHash<quint16,qint32>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            change_warehouse_items_list << QPair<quint16,qint32>(i.key(),i.value());
            ++i;
        }
        CatchChallenger::Api_client_real::client->wareHouseStore(temp_warehouse_cash,change_warehouse_items_list,monster_to_withdraw,monster_to_deposit);
    }
    //validate the change here
    if(temp_warehouse_cash>0)
        addCash(temp_warehouse_cash);
    else
        removeCash(-temp_warehouse_cash);
    warehouse_cash-=temp_warehouse_cash;
    {
        QHash<quint16,qint32>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            if(i.value()>0)
            {
                if(items.contains(i.key()))
                    items[i.key()]+=i.value();
                else
                    items[i.key()]=i.value();
                warehouse_items[i.key()]-=i.value();
                if(warehouse_items.value(i.key())==0)
                    warehouse_items.remove(i.key());
            }
            if(i.value()<0)
            {
                items[i.key()]+=i.value();
                if(items.value(i.key())==0)
                    items.remove(i.key());
                if(warehouse_items.contains(i.key()))
                    warehouse_items[i.key()]-=i.value();
                else
                    warehouse_items[i.key()]=-i.value();
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
            int sub_index=0;
            while(sub_index<warehouse_playerMonster.size())
            {
                if(warehouse_playerMonster.at(sub_index).id==monster_to_withdraw.at(index))
                {
                    playerMonsterToWithdraw << warehouse_playerMonster.at(sub_index);
                    warehouse_playerMonster.removeAt(sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
        CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(playerMonsterToWithdraw);
    }
    {
        int index=0;
        while(index<monster_to_deposit.size())
        {
            const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
            int sub_index=0;
            while(sub_index<playerMonster.size())
            {
                if(playerMonster.at(sub_index).id==monster_to_deposit.at(index))
                {
                    warehouse_playerMonster << playerMonster.at(sub_index);
                    CatchChallenger::ClientFightEngine::fightEngine.removeMonster(monster_to_deposit.at(index));
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    load_monsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You have correctly withdraw/deposit your goods"));
}
