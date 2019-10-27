#include "../../interface/BaseWindow.h"
#include "../../QtDatapackClientLoader.h"
#include "../../Api_client_real.h"
#include "../../../../general/base/FacilityLib.h"
#include "../../../../general/base/FacilityLibGeneral.h"
#include "../../../../general/base/GeneralStructures.h"
#include "../../../../general/base/CommonDatapack.h"
#include "../../../../general/base/CommonSettingsServer.h"
#include "ui_BaseWindow.h"
#include "../../FacilityLibClient.h"
#include "../../Ultimate.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QQmlContext>
#include <QUrl>
#include <iostream>

/* show only the plant into the inventory */

using namespace CatchChallenger;

void BaseWindow::insert_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y, const uint8_t &plant_id, const uint16_t &seconds_to_mature)
{
    Q_UNUSED(plant_id);
    Q_UNUSED(seconds_to_mature);
    if(mapId>=(uint32_t)QtDatapackClientLoader::datapackLoader->maps.size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than QtDatapackClientLoader::datapackLoader->maps.size()";
        return;
    }
    cancelAllPlantQuery(mapController->mapIdToString(mapId),x,y);
}

void BaseWindow::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    if(mapId>=(uint32_t)QtDatapackClientLoader::datapackLoader->maps.size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than QtDatapackClientLoader::datapackLoader->maps.size()";
        return;
    }
    cancelAllPlantQuery(mapController->mapIdToString(mapId),x,y);
}

void BaseWindow::cancelAllPlantQuery(const std::string map,const uint8_t x,const uint8_t y)
{
    unsigned int index;
    index=0;
    while(index<seed_in_waiting.size())
    {
        const SeedInWaiting &seedInWaiting=seed_in_waiting.at(index);
        if(seedInWaiting.map==map && seedInWaiting.x==x && seedInWaiting.y==y)
        {
            seed_in_waiting[index].map=std::string();
            seed_in_waiting[index].x=0;
            seed_in_waiting[index].y=0;
        }
        index++;
    }
    index=0;
    while(index<plant_collect_in_waiting.size())
    {
        const ClientPlantInCollecting &clientPlantInCollecting=plant_collect_in_waiting.at(index);
        if(clientPlantInCollecting.map==map && clientPlantInCollecting.x==x && clientPlantInCollecting.y==y)
        {
            plant_collect_in_waiting[index].map=std::string();
            plant_collect_in_waiting[index].x=0;
            plant_collect_in_waiting[index].y=0;
        }
        index++;
    }
}

void BaseWindow::seed_planted(const bool &ok)
{
    removeQuery(QueryType_Seed);
    if(ok)
    {
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted").toStdString());
        //do the rewards
        {
            const uint16_t &itemId=seed_in_waiting.front().seedItemId;
            if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(itemId)==
                    QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(this,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                return;
            }
            const uint8_t &plant=QtDatapackClientLoader::datapackLoader->itemToPlants.at(itemId);
            appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plant).rewards.reputation);
        }
    }
    else
    {
        if(!seed_in_waiting.front().map.empty())
        {
            mapController->remove_plant_full(seed_in_waiting.front().map,seed_in_waiting.front().x,seed_in_waiting.front().y);
            cancelAllPlantQuery(seed_in_waiting.front().map,seed_in_waiting.front().x,seed_in_waiting.front().y);
        }
        add_to_inventory(seed_in_waiting.front().seedItemId,1,false);
        showTip(tr("Seed cannot be planted").toStdString());
        load_inventory();
    }
    seed_in_waiting.erase(seed_in_waiting.cbegin());
}

void BaseWindow::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    removeQuery(QueryType_CollectPlant);
    switch(stat)
    {
        case Plant_collect_correctly_collected:
            //see to optimise CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true and use the internal random number list
            showTip(tr("Plant collected").toStdString());//the item is send by another message with the protocol
        break;
        /*case Plant_collect_empty_dirt:
            showTip(tr("Try collect an empty dirt").toStdString());
        break;
        case Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player").toStdString());
            mapController->insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                             plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
        break;
        case Plant_collect_impossible:
            showTip(tr("This plant can't be collected").toStdString());
            mapController->insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                             plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
        break;*/
        default:
            qDebug() << "BaseWindow::plant_collected(): unknown return";
        break;
    }
}

void BaseWindow::load_plant_inventory()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_plant_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->listPlantList->clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while(i!=playerInformations.items.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(i->first)!=
                QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
        {
            const uint8_t &plantId=QtDatapackClientLoader::datapackLoader->itemToPlants.at(i->first);
            QListWidgetItem *item;
            item=new QListWidgetItem();
            plants_items_to_graphical[plantId]=item;
            plants_items_graphical[item]=plantId;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(i->first)!=
                    QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[i->first].image);
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[i->first].name)+
                        "\n"+tr("Quantity: %1").arg(i->second));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                item->setText(QStringLiteral("item id: %1").arg(i->first)+"\n"+tr("Quantity: %1").arg(i->second));
            }
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
            {
                item->setText(item->text()+"\n"+tr("You don't have the requirements"));
                item->setFont(disableIntoListFont);
                item->setForeground(disableIntoListBrush);
            }
            ui->listPlantList->addItem(item);
        }
        ++i;
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
    Player_private_and_public_informations informations=client->get_player_informations();
    if(informations.recipes==NULL)
    {
        qDebug() << "BaseWindow::load_crafting_inventory(), crafting null";
        return;
    }
    uint16_t index=0;
    while(index<=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipesMaxId)
    {
        uint16_t recipe=index;
        if(informations.recipes[recipe/8] & (1<<(7-recipe%8)))
        {
            //load the material item
            if(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.find(recipe)
                    !=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId)!=
                        QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe]
                            .doItemId].image);
                    item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe]
                            .doItemId].name));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                    item->setText(tr("Unknow item: %1").arg(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[recipe].doItemId));
                }
                crafting_recipes_items_to_graphical[recipe]=item;
                crafting_recipes_items_graphical[item]=recipe;
                ui->listCraftingList->addItem(item);
            }
            else
                qDebug() << QStringLiteral("BaseWindow::load_crafting_inventory(), crafting id not found into crafting recipe").arg(recipe);
        }
        ++index;
    }
    on_listCraftingList_itemSelectionChanged();
}

std::string BaseWindow::reputationRequirementsToText(const ReputationRequirements &reputationRequirements)
{
    if(reputationRequirements.reputationId>=CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
    {
        std::cerr << "reputationRequirements.reputationId" << reputationRequirements.reputationId
                  << ">=CatchChallenger::CommonDatapack::commonDatapack.reputation.size()"
                  << CatchChallenger::CommonDatapack::commonDatapack.reputation.size() << std::endl;
        return tr("Unknown reputation id: %1").arg(reputationRequirements.reputationId).toStdString();
    }
    const Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRequirements.reputationId);
    if(QtDatapackClientLoader::datapackLoader->reputationExtra.find(reputation.name)==
            QtDatapackClientLoader::datapackLoader->reputationExtra.cend())
    {
        std::cerr << "!QtDatapackClientLoader::datapackLoader->reputationExtra.contains("+reputation.name+")" << std::endl;
        return tr("Unknown reputation name: %1").arg(QString::fromStdString(reputation.name)).toStdString();
    }
    const QtDatapackClientLoader::ReputationExtra &reputationExtra=QtDatapackClientLoader::datapackLoader->reputationExtra.at(reputation.name);
    if(reputationRequirements.positif)
    {
        if(reputationRequirements.level>=reputationExtra.reputation_positive.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_positive.at(reputationRequirements.level);
    }
    else
    {
        if(reputationRequirements.level>=reputationExtra.reputation_negative.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_negative.at(reputationRequirements.level);
    }
}

void BaseWindow::on_listPlantList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
    {
        ui->labelPlantImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
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
        ui->labelPlantRequirementsAndRewards->setText(QString());
        if(Ultimate::ultimate.isUltimate())
            ui->labelPlantByDay->setText(QString());

        ui->plantUse->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    const QtDatapackClientLoader::QtPlantExtra &contentExtra=QtDatapackClientLoader::datapackLoader->QtplantExtra[plants_items_graphical[item]];
    const CatchChallenger::Plant &plant=CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]];

    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(plant.itemUsed)!=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        const QtDatapackClientLoader::ItemExtra &itemExtra=QtDatapackClientLoader::datapackLoader->itemsExtra.at(plant.itemUsed);
        const QtDatapackClientLoader::QtItemExtra &QtitemExtra=QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(plant.itemUsed);
        ui->labelPlantImage->setPixmap(QtitemExtra.image);
        ui->labelPlantName->setText(QString::fromStdString(itemExtra.name));
        ui->labelPlantFruitImage->setPixmap(QtitemExtra.image);
        ui->labelPlantDescription->setText(QString::fromStdString(itemExtra.description));
        //requirements and rewards
        {
            QStringList requirements;
            {
                unsigned int index=0;
                while(index<plant.requirements.reputation.size())
                {
                    requirements.push_back(QString::fromStdString(
                          reputationRequirementsToText(plant.requirements.reputation.at(index))));
                    index++;
                }
            }
            if(requirements.empty())
                ui->labelPlantRequirementsAndRewards->setText(QString());
            else
                ui->labelPlantRequirementsAndRewards->setText(tr("Requirements: ")+requirements.join(QStringLiteral(", ")));
            if(Ultimate::ultimate.isUltimate())
            {
                QStringList rewards_less_reputation,rewards_more_reputation;
                {
                    unsigned int index=0;
                    while(index<plant.rewards.reputation.size())
                    {
                        QString name=QStringLiteral("???");
                        const ReputationRewards &reputationRewards=plant.rewards.reputation.at(index);
                        if(reputationRewards.reputationId<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
                        {
                            const Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRewards.reputationId);
                            if(QtDatapackClientLoader::datapackLoader->reputationExtra.find(reputation.name)!=
                                    QtDatapackClientLoader::datapackLoader->reputationExtra.cend())
                                name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->reputationExtra.at(reputation.name).name);
                        }
                        if(reputationRewards.point<0)
                            rewards_less_reputation.push_back(name);
                        else if(reputationRewards.point>0)
                            rewards_more_reputation.push_back(name);
                        index++;
                    }
                }
                if(!rewards_less_reputation.empty() || !rewards_more_reputation.empty())
                {
                    ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+QStringLiteral("<br />"));
                    if(!rewards_less_reputation.empty())
                        ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+tr("Less reputation in: ")+rewards_less_reputation.join(QStringLiteral(", ")));
                    if(!rewards_less_reputation.empty() && !rewards_more_reputation.empty())
                        ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+QStringLiteral(", "));
                    if(!rewards_more_reputation.empty())
                        ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+tr("More reputation in: ")+rewards_more_reputation.join(QStringLiteral(", ")));
                }
            }
        }
        if(Ultimate::ultimate.isUltimate())
            if(plant.fruits_seconds>0)
            {
                double quantity=(double)plant.fix_quantity+(double)plant.random_quantity/(double)RANDOM_FLOAT_PART_DIVIDER-1;
                double quantityByDay=quantity*(double)86400/(double)plant.fruits_seconds;
                if(CommonDatapack::commonDatapack.items.item.find(plant.itemUsed)!=CommonDatapack::commonDatapack.items.item.cend())
                    ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0))+", "+tr("income by day: %1").arg(QString::number(quantityByDay*CommonDatapack::commonDatapack.items.item[plant.itemUsed].price,'f',0)));
                else
                    ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0)));
            }
    }
    else
    {
        ui->labelPlantImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        ui->labelPlantName->setText(tr("Unknow plant (%1)").arg(plant.itemUsed));
        ui->labelPlantFruitImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        ui->labelPlantDescription->setText(tr("This plant and these effects are unknown"));
        ui->labelPlantRequirementsAndRewards->setText(QString());
        if(Ultimate::ultimate.isUltimate())
            if(plant.fruits_seconds>0)
            {
                double quantity=(double)plant.fix_quantity+(double)plant.random_quantity/(double)RANDOM_FLOAT_PART_DIVIDER-1;
                double quantityByDay=quantity*(double)86400/(double)plant.fruits_seconds;
                ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0)));
            }
    }

    ui->labelPlantedImage->setPixmap(contentExtra.tileset->tileAt(0)->image().scaled(32,64));
    ui->labelSproutedImage->setPixmap(contentExtra.tileset->tileAt(1)->image().scaled(32,64));
    ui->labelTallerImage->setPixmap(contentExtra.tileset->tileAt(2)->image().scaled(32,64));
    ui->labelFloweringImage->setPixmap(contentExtra.tileset->tileAt(3)->image().scaled(32,64));
    ui->labelFruitsImage->setPixmap(contentExtra.tileset->tileAt(4)->image().scaled(32,64));

    ui->labelPlantedText->setText(QString::fromStdString(FacilityLibClient::timeToString(0)));
    ui->labelSproutedText->setText(QString::fromStdString(FacilityLibClient::timeToString(plant.sprouted_seconds)));
    ui->labelTallerText->setText(QString::fromStdString(FacilityLibClient::timeToString(plant.taller_seconds)));
    ui->labelFloweringText->setText(QString::fromStdString(FacilityLibClient::timeToString(plant.flowering_seconds)));
    ui->labelFruitsText->setText(QString::fromStdString(FacilityLibClient::timeToString(plant.fruits_seconds)));
    ui->labelPlantFruitText->setText(tr("Quantity: %1").arg((double)plant.fix_quantity+((double)plant.random_quantity)/RANDOM_FLOAT_PART_DIVIDER));

    ui->plantUse->setVisible(inSelection);
}

void BaseWindow::on_toolButton_quit_plants_clicked()
{
    ui->listPlantList->reset();
    if(inSelection)
    {
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        objectSelection(false,0);
    }
    else
        ui->stackedWidget->setCurrentWidget(ui->page_inventory);
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
    if(plants_items_graphical.find(item)==plants_items_graphical.cend())
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
    ui->stackedWidget->setCurrentWidget(ui->page_crafting);
}

void BaseWindow::on_toolButton_quit_crafting_clicked()
{
    ui->listCraftingList->reset();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::on_listCraftingList_itemSelectionChanged()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    ui->listCraftingMaterials->clear();
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
    {
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        ui->labelCraftingDetails->setText(tr("Select a recipe"));
        ui->craftingUse->setVisible(false);
        return;
    }
    QListWidgetItem *itemMaterials=displayedItems.first();
    const CatchChallenger::CrafingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[crafting_recipes_items_graphical[itemMaterials]];

    qDebug() << "on_listCraftingList_itemSelectionChanged() load the name";
    //load the name
    QString name;
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.doItemId)!=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].name);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.doItemId].image);
    }
    else
    {
        name=tr("Unknow item (%1)").arg(content.doItemId);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
    }
    ui->labelCraftingDetails->setText(tr("Name: <b>%1</b><br /><br />Success: <b>%2%</b><br /><br />Result: <b>%3</b>").arg(name).arg(content.success).arg(content.quantity));

    //load the materials
    bool haveMaterials=true;
    unsigned int index=0;
    QString nameMaterials;
    QListWidgetItem *item;
    uint32_t quantity;
    while(index<content.materials.size())
    {
        //load the material item
        item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.materials.at(index).item)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            nameMaterials=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].name);
            item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.materials.at(index).item].image);
        }
        else
        {
            nameMaterials=tr("Unknow item (%1)").arg(content.materials.at(index).item);
            item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(playerInformations.items.find(content.materials.at(index).item)!=playerInformations.items.cend())
            quantity=playerInformations.items.at(content.materials.at(index).item);

        //load the display
        item->setText(tr("Needed: %1 %2\nIn the inventory: %3 %4").arg(content.materials.at(index).quantity).arg(nameMaterials).arg(quantity).arg(nameMaterials));
        if(quantity<content.materials.at(index).quantity)
        {
            item->setFont(disableIntoListFont);
            item->setForeground(disableIntoListBrush);
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
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    //recipeInUsing
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
        return;
    QListWidgetItem *selectedItem=displayedItems.first();
    const CatchChallenger::CrafingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes[crafting_recipes_items_graphical[selectedItem]];

    QStringList mIngredients;
    QString mRecipe;
    QString mProduct;
    //load the materials
    unsigned int index=0;
    while(index<content.materials.size())
    {
        if(playerInformations.items.find(content.materials.at(index).item)==playerInformations.items.cend())
            return;
        if(playerInformations.items.at(content.materials.at(index).item)<content.materials.at(index).quantity)
            return;
        uint32_t sub_index=0;
        while(sub_index<content.materials.at(index).quantity)
        {
            mIngredients.push_back(QUrl::fromLocalFile(
                                       QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].imagePath)
                                   ).toEncoded());
            sub_index++;
        }
        index++;
    }
    index=0;
    std::vector<std::pair<uint16_t,uint32_t> > recipeUsage;
    while(index<content.materials.size())
    {
        std::pair<uint16_t,uint32_t> pair;
        pair.first=content.materials.at(index).item;
        pair.second=content.materials.at(index).quantity;
        remove_to_inventory(pair.first,pair.second);
        recipeUsage.push_back(pair);
        index++;
    }
    materialOfRecipeInUsing.push_back(recipeUsage);
    //the product do
    std::pair<uint16_t,uint32_t> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing.push_back(pair);
    mProduct=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].imagePath)).toEncoded();
    mRecipe=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.itemToLearn].imagePath)).toEncoded();
    //update the UI
    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    client->useRecipe(crafting_recipes_items_graphical.at(selectedItem));
    appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.at(
                                crafting_recipes_items_graphical.at(selectedItem)).rewards.reputation);
    //create animation widget
    if(animationWidget!=NULL)
        delete animationWidget;
    if(qQuickViewContainer!=NULL)
        delete qQuickViewContainer;
    animationWidget=new QQuickView();
    qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
    qQuickViewContainer->setMinimumSize(QSize(800,600));
    qQuickViewContainer->setMaximumSize(QSize(800,600));
    qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
    ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
    //show the animation
    previousAnimationWidget=ui->stackedWidget->currentWidget();
    ui->stackedWidget->setCurrentWidget(ui->page_animation);
    if(craftingAnimationObject!=NULL)
        delete craftingAnimationObject;
    craftingAnimationObject=new CraftingAnimation(mIngredients,
                                                  mRecipe,mProduct,
                                                  QUrl::fromLocalFile(QString::fromStdString(
              playerBackImagePath)).toEncoded());
    animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
    animationWidget->rootContext()->setContextProperty("craftingAnimationObject",craftingAnimationObject);
    const QString datapackQmlFile=QString::fromStdString(client->datapackPathBase())+"qml/crafting-animation.qml";
    if(QFile(datapackQmlFile).exists())
        animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
    else
        animationWidget->setSource(QStringLiteral("qrc:/qml/crafting-animation.qml"));
}

void BaseWindow::on_listCraftingMaterials_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->craftingUse->clicked();
}

void BaseWindow::recipeUsed(const RecipeUsage &recipeUsage)
{
    switch(recipeUsage)
    {
        case RecipeUsage_ok:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            add_to_inventory(productOfRecipeInUsing.front().first,productOfRecipeInUsing.front().second);
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        break;
        case RecipeUsage_impossible:
        {
            unsigned int index=0;
            while(index<materialOfRecipeInUsing.front().size())
            {
                add_to_inventory(materialOfRecipeInUsing.front().at(index).first,
                                 materialOfRecipeInUsing.front().at(index).first,false);
                index++;
            }
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        }
        break;
        case RecipeUsage_failed:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
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

