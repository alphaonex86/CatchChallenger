#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"

#include <QInputDialog>

using namespace CatchChallenger;

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
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(monster.monster))
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<playerInformations.warehouse_monsters.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_monsters.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(monster.monster))
                warehouseMonsterOnPlayerList.push_back(monster);
            index++;
        }
    }
    return warehouseMonsterOnPlayerList;
}

void BaseWindow::on_toolButton_quit_warehouse_clicked()
{
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
        if(!client->wareHouseStore(withdrawMonsters,depositeMonsters))
        {
            emit error("Bug on warehouse API call");
            return;
        }
    }
    //validate the monster changes
    {
        const std::vector<PlayerMonster> &currentPlayerMonsters=client->getPlayerMonster();
        unsigned int index=0;
        while(index<withdrawMonsters.size())
        {
            if(withdrawMonsters.at(index)>=playerInformations.warehouse_monsters.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->addPlayerMonster(playerInformations.warehouse_monsters.at(withdrawMonsters.at(index)));
            index++;
        }
        index=0;
        while(index<depositeMonsters.size())
        {
            if(depositeMonsters.at(index)>=currentPlayerMonsters.size())
            {
                emit error("out of bound store list");
                return;
            }
            client->addPlayerMonsterWarehouse(currentPlayerMonsters.at(depositeMonsters.at(index)));
            index++;
        }
    }
    {
        const std::vector<PlayerMonster> &currentPlayerMonsters=client->getPlayerMonster();
        unsigned int index=0;
        while(index<depositeMonsters.size())
        {
            if(depositeMonsters.at(index)>=currentPlayerMonsters.size())
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
            if(withdrawMonsters.at(index)>=playerInformations.warehouse_monsters.size())
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
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(monster.monster))
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).description));
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
        while(index<playerInformations.warehouse_monsters.size())
        {
            const PlayerMonster &monster=playerInformations.warehouse_monsters.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(monster.monster))
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(tr("%1, level: %2")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).name))
                        .arg(monster.level)
                        );
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).description));
                item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster).front);
                item->setData(98,1);
                item->setData(99,index);
                ui->warehousePlayerStoredMonster->addItem(item);
            }
            index++;
        }
    }

    ui->warehouseDepositMonster->setEnabled(ui->warehousePlayerMonster->count()>1);
    ui->warehouseWithdrawMonster->setEnabled(ui->warehousePlayerStoredMonster->count()>0);
}
