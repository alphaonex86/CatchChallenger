#include "../base/interface/BaseWindow.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralStructures.h"
#include "../../../general/base/CommonDatapack.h"
#include "ui_BaseWindow.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>

/* show only the plant into the inventory */

using namespace CatchChallenger;

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

void BaseWindow::plant_collected(const CatchChallenger::Plant_collect &stat)
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
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].image);
                item->setText(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name+"\n"+tr("Quantity: %1").arg(i.value()));
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
    ui->listCraftingList->clear();
    crafting_recipes_items_to_graphical.clear();
    crafting_recipes_items_graphical.clear();
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    QSetIterator<quint32> i(informations.recipes);
    while (i.hasNext())
    {
        quint32 recipe=i.next();
        //load the material item
        if(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.contains(recipe))
        {
            QListWidgetItem *item=new QListWidgetItem();
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId].image);
                item->setText(DatapackClientLoader::datapackLoader.itemsExtra[CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId].name);
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                item->setText(tr("Unknow item: %1").arg(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId));
            }
            crafting_recipes_items_to_graphical[recipe]=item;
            crafting_recipes_items_graphical[item]=recipe;
            ui->listCraftingList->addItem(item);
        }
        else
            qDebug() << QString("BaseWindow::load_crafting_inventory(), crafting id not found into crafting recipe").arg(recipe);
    }
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::on_listPlantList_itemSelectionChanged()
{
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
    const DatapackClientLoader::PlantExtra &contentExtra=DatapackClientLoader::datapackLoader.plantExtra[plants_items_graphical[item]];
    const CatchChallenger::Plant &content=CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]];

    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(content.itemUsed))
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.itemsExtra[content.itemUsed].image);
        ui->labelPlantName->setText(DatapackClientLoader::datapackLoader.itemsExtra[content.itemUsed].name);
        ui->labelPlantFruitImage->setPixmap(DatapackClientLoader::datapackLoader.itemsExtra[content.itemUsed].image);
        ui->labelPlantDescription->setText(DatapackClientLoader::datapackLoader.itemsExtra[content.itemUsed].description);
    }
    else
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantName->setText(tr("Unknow plant (%1)").arg(content.itemUsed));
        ui->labelPlantFruitImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantDescription->setText(tr("This plant and these effects are unknown"));
    }

    ui->labelPlantedImage->setPixmap(contentExtra.tileset->tileAt(0)->image().scaled(32,64));
    ui->labelSproutedImage->setPixmap(contentExtra.tileset->tileAt(1)->image().scaled(32,64));
    ui->labelTallerImage->setPixmap(contentExtra.tileset->tileAt(2)->image().scaled(32,64));
    ui->labelFloweringImage->setPixmap(contentExtra.tileset->tileAt(3)->image().scaled(32,64));
    ui->labelFruitsImage->setPixmap(contentExtra.tileset->tileAt(4)->image().scaled(32,64));

    ui->labelPlantedText->setText(FacilityLib::secondsToString(0));
    ui->labelSproutedText->setText(FacilityLib::secondsToString(content.sprouted_seconds));
    ui->labelTallerText->setText(FacilityLib::secondsToString(content.taller_seconds));
    ui->labelFloweringText->setText(FacilityLib::secondsToString(content.flowering_seconds));
    ui->labelFruitsText->setText(FacilityLib::secondsToString(content.fruits_seconds));
    ui->labelPlantFruitText->setText(tr("Quantity: %1").arg((float)content.fix_quantity+((float)content.random_quantity)/RANDOM_FLOAT_PART_DIVIDER));

    ui->plantUse->setVisible(inSelection);
}

void BaseWindow::on_toolButton_quit_plants_clicked()
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

void BaseWindow::on_plantUse_clicked()
{
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
        return;
    on_listPlantList_itemActivated(items.first());
}

void BaseWindow::on_listPlantList_itemActivated(QListWidgetItem *item)
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
    objectSelection(true,CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]].itemUsed);
}

void BaseWindow::on_pushButton_interface_crafting_clicked()
{
    ui->stackedWidget->setCurrentIndex(6);
}

void BaseWindow::on_toolButton_quit_crafting_clicked()
{
    ui->listCraftingList->reset();
    ui->stackedWidget->setCurrentIndex(1);
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::on_listCraftingList_itemSelectionChanged()
{
    ui->listCraftingMaterials->clear();
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
    {
        ui->labelCraftingImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelCraftingDetails->setText(tr("Select a recipe"));
        ui->craftingUse->setVisible(false);
        return;
    }
    QListWidgetItem *itemMaterials=displayedItems.first();
    const CatchChallenger::CrafingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[crafting_recipes_items_graphical[itemMaterials]];

    qDebug() << "on_listCraftingList_itemSelectionChanged() load the name";
    //load the name
    QString name;
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(content.doItemId))
    {
        name=DatapackClientLoader::datapackLoader.itemsExtra[content.doItemId].name;
        ui->labelCraftingImage->setPixmap(DatapackClientLoader::datapackLoader.itemsExtra[content.doItemId].image);
    }
    else
    {
        name=tr("Unknow item (%1)").arg(content.doItemId);
        ui->labelCraftingImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
    }
    ui->labelCraftingDetails->setText(QString("Name: <b>%1</b><br /><br />Success: <b>%2%</b><br /><br />Result: <b>%3</b>").arg(name).arg(content.success).arg(content.quantity));

    //load the materials
    bool haveMaterials=true;
    int index=0;
    QString nameMaterials;
    QListWidgetItem *item;
    quint32 quantity;
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    while(index<content.materials.size())
    {
        //load the material item
        item=new QListWidgetItem();
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(content.materials.at(index).itemId))
        {
            nameMaterials=DatapackClientLoader::datapackLoader.itemsExtra[content.materials.at(index).itemId].name;
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[content.materials.at(index).itemId].image);
        }
        else
        {
            nameMaterials=tr("Unknow item (%1)").arg(content.materials.at(index).itemId);
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(items.contains(content.materials.at(index).itemId))
            quantity=items[content.materials.at(index).itemId];

        //load the display
        item->setText(tr("Needed: %1 %2\nIn the inventory: %3 %4").arg(content.materials.at(index).quantity).arg(nameMaterials).arg(quantity).arg(nameMaterials));
        if(quantity<content.materials.at(index).quantity)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }

        if(quantity<content.materials.at(index).quantity)
            haveMaterials=false;

        ui->listCraftingMaterials->addItem(item);
        ui->craftingUse->setVisible(haveMaterials);
        index++;
    }
}

void BaseWindow::on_craftingUse_clicked()
{
    //recipeInUsing
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
        return;
    QListWidgetItem *selectedItem=displayedItems.first();
    const CatchChallenger::CrafingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[crafting_recipes_items_graphical[selectedItem]];

    //load the materials
    int index=0;
    while(index<content.materials.size())
    {
        if(!items.contains(content.materials.at(index).itemId))
            return;
        if(items[content.materials.at(index).itemId]<content.materials.at(index).quantity)
            return;
        index++;
    }
    index=0;
    QList<QPair<quint32,quint32> > recipeUsage;
    while(index<content.materials.size())
    {
        QPair<quint32,quint32> pair;
        pair.first=content.materials.at(index).itemId;
        pair.second=content.materials.at(index).quantity;
        items[pair.first]-=pair.second;
        if(items[pair.first]==0)
            items.remove(pair.first);
        recipeUsage << pair;
        index++;
    }
    materialOfRecipeInUsing << recipeUsage;
    //the product do
    QPair<quint32,quint32> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing << pair;
    //update the UI
    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    CatchChallenger::Api_client_real::client->useRecipe(crafting_recipes_items_graphical[selectedItem]);
}

void BaseWindow::recipeUsed(const RecipeUsage &recipeUsage)
{
    switch(recipeUsage)
    {
        case RecipeUsage_ok:
            materialOfRecipeInUsing.removeFirst();
            if(items.contains(productOfRecipeInUsing.first().first))
                items[productOfRecipeInUsing.first().first]+=productOfRecipeInUsing.first().second;
            else
                items[productOfRecipeInUsing.first().first]=productOfRecipeInUsing.first().second;
            productOfRecipeInUsing.removeFirst();
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        break;
        case RecipeUsage_impossible:
        {
            int index=0;
            while(index<materialOfRecipeInUsing.first().size())
            {
                if(items.contains(materialOfRecipeInUsing.first().at(index).first))
                    items[materialOfRecipeInUsing.first().at(index).first]+=materialOfRecipeInUsing.first().at(index).first;
                else
                    items[materialOfRecipeInUsing.first().at(index).first]=materialOfRecipeInUsing.first().at(index).first;
                index++;
            }
            materialOfRecipeInUsing.removeFirst();
            productOfRecipeInUsing.removeFirst();
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        }
        break;
        case RecipeUsage_failed:
            materialOfRecipeInUsing.removeFirst();
            productOfRecipeInUsing.removeFirst();
        break;
        default:
        qDebug() << "recipeUsed() unknow code";
        return;
    }
}

void BaseWindow::on_listCraftingList_itemActivated(QListWidgetItem *)
{
    if(ui->craftingUse->isVisible())
        on_craftingUse_clicked();
}

