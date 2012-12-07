#include "../base/interface/BaseWindow.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../../general/base/FacilityLib.h"
#include "ui_BaseWindow.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>

/* show only the plant into the inventory */

using namespace Pokecraft;

void BaseWindow::seed_planted(const bool &ok)
{
    removeQuery(QueryType_Seed);
    seedWait=false;
    if(ok)
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted"));
    else
    {
        if(items.contains(seed_in_waiting))
            items[seed_in_waiting]++;
        else
            items[seed_in_waiting]=1;
        showTip(tr("Seed cannot be planted"));
        load_inventory();
    }
}

void BaseWindow::plant_collected(const Pokecraft::Plant_collect &stat)
{
    collectWait=false;
    removeQuery(QueryType_CollectPlant);
    switch(stat)
    {
        case Plant_collect_correctly_collected:
            showTip(tr("Plant collected"));
        break;
        case Plant_collect_empty_dirt:
            showTip(tr("Try collected empty dirt"));
        break;
        case Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player"));
        break;
        case Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
        break;
        default:
        qDebug() << "BaseWindow::plant_collected(): unkonw return";
        return;
    }
}

void BaseWindow::load_plant_inventory()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_plant_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->listPlantList->clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.itemToPlants.contains(i.key()))
        {
            QListWidgetItem *item;
            item=new QListWidgetItem();
            plants_items_to_graphical[DatapackClientLoader::datapackLoader.itemToPlants[i.key()]]=item;
            plants_items_graphical[item]=DatapackClientLoader::datapackLoader.itemToPlants[i.key()];
            if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
                item->setText(DatapackClientLoader::datapackLoader.items[i.key()].name+"\n"+tr("Quantity: %1").arg(i.value()));
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                item->setText(QString("item id: %1").arg(i.key())+"\n"+tr("Quantity: %1").arg(i.value()));
            }
            ui->listPlantList->addItem(item);
        }
    }
}

void BaseWindow::load_crafting_inventory()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_crafting_inventory()";
    #endif
}

void BaseWindow::on_listPlantList_itemSelectionChanged()
{
    qDebug() << "on_listPlantList_itemSelectionChanged()";
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantName->setText(tr("Select a plant"));

        ui->labelPlantedImage->setPixmap(QPixmap());
        ui->labelSproutedImage->setPixmap(QPixmap());
        ui->labelTallerImage->setPixmap(QPixmap());
        ui->labelFloweringImage->setPixmap(QPixmap());
        ui->labelFruitsImage->setPixmap(QPixmap());
        ui->labelPlantFruitImage->setPixmap(QPixmap());

        ui->labelPlantedText->setText(QString());
        ui->labelSproutedText->setText(QString());
        ui->labelTallerText->setText(QString());
        ui->labelFloweringText->setText(QString());
        ui->labelFruitsText->setText(QString());
        ui->labelPlantFruitText->setText(QString());
        ui->labelPlantDescription->setText(QString());

        ui->plantUse->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    const DatapackClientLoader::Plant &content=DatapackClientLoader::datapackLoader.plants[plants_items_graphical[item]];

    if(DatapackClientLoader::datapackLoader.items.contains(content.itemUsed))
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.items[content.itemUsed].image);
        ui->labelPlantName->setText(DatapackClientLoader::datapackLoader.items[content.itemUsed].name);
        ui->labelPlantFruitImage->setPixmap(DatapackClientLoader::datapackLoader.items[content.itemUsed].image);
        ui->labelPlantDescription->setText(DatapackClientLoader::datapackLoader.items[content.itemUsed].description);
    }
    else
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantName->setText(tr("Unknow plant (%1)").arg(content.itemUsed));
        ui->labelPlantFruitImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantDescription->setText(tr("This plant and these effects are unknown"));
    }

    ui->labelPlantedImage->setPixmap(content.tileset->tileAt(0)->image().scaled(32,64));
    ui->labelSproutedImage->setPixmap(content.tileset->tileAt(1)->image().scaled(32,64));
    ui->labelTallerImage->setPixmap(content.tileset->tileAt(2)->image().scaled(32,64));
    ui->labelFloweringImage->setPixmap(content.tileset->tileAt(3)->image().scaled(32,64));
    ui->labelFruitsImage->setPixmap(content.tileset->tileAt(4)->image().scaled(32,64));

    ui->labelPlantedText->setText(FacilityLib::secondsToString(0));
    ui->labelSproutedText->setText(FacilityLib::secondsToString(content.sprouted_seconds));
    ui->labelTallerText->setText(FacilityLib::secondsToString(content.taller_seconds));
    ui->labelFloweringText->setText(FacilityLib::secondsToString(content.flowering_seconds));
    ui->labelFruitsText->setText(FacilityLib::secondsToString(content.fruits_seconds));
    ui->labelPlantFruitText->setText(tr("Quantity: %1").arg(content.quantity));

    ui->plantUse->setVisible(inSelection);
}

void Pokecraft::BaseWindow::on_toolButton_quit_plants_clicked()
{
    ui->listPlantList->reset();
    if(inSelection)
    {
        ui->stackedWidget->setCurrentIndex(1);
        objectSelection(false,0);
    }
    else
        ui->stackedWidget->setCurrentIndex(3);
    on_listPlantList_itemSelectionChanged();
}

void Pokecraft::BaseWindow::on_plantUse_clicked()
{
    qDebug() << "on_plantUse_clicked()";
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
        return;
    on_listPlantList_itemActivated(items.first());
}

void Pokecraft::BaseWindow::on_listPlantList_itemActivated(QListWidgetItem *item)
{
    if(!plants_items_graphical.contains(item))
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(!inSelection)
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): not in selection, use is not done actually";
        return;
    }
    objectSelection(true,DatapackClientLoader::datapackLoader.plants[plants_items_graphical[item]].itemUsed);
}
