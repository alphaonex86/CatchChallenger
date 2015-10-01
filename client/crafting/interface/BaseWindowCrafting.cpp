#include "../../base/interface/BaseWindow.h"
#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/Api_client_real.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/FacilityLibGeneral.h"
#include "../../../general/base/GeneralStructures.h"
#include "../../../general/base/CommonDatapack.h"
#include "ui_BaseWindow.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QQmlContext>
#include <QUrl>

/* show only the plant into the inventory */

using namespace CatchChallenger;

void BaseWindow::insert_plant(const uint32_t &mapId, const uint8_t &x, const uint8_t &y, const uint8_t &plant_id, const uint16_t &seconds_to_mature)
{
    Q_UNUSED(plant_id);
    Q_UNUSED(seconds_to_mature);
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    cancelAllPlantQuery(MapController::mapController->mapIdToString(mapId),x,y);
}

void BaseWindow::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    if(mapId>=(uint32_t)DatapackClientLoader::datapackLoader.maps.size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than DatapackClientLoader::datapackLoader.maps.size()";
        return;
    }
    cancelAllPlantQuery(MapController::mapController->mapIdToString(mapId),x,y);
}

void BaseWindow::cancelAllPlantQuery(const QString map,const uint8_t x,const uint8_t y)
{
    int index;
    index=0;
    while(index<seed_in_waiting.size())
    {
        const SeedInWaiting &seedInWaiting=seed_in_waiting.at(index);
        if(seedInWaiting.map==map && seedInWaiting.x==x && seedInWaiting.y==y)
        {
            seed_in_waiting[index].map=QString();
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
            plant_collect_in_waiting[index].map=QString();
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
        showTip(tr("Seed correctly planted"));
        //do the rewards
        {
            const uint32_t &itemId=seed_in_waiting.first().seedItemId;
            if(!DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(this,tr("Error"),tr("Internal error"));
                return;
            }
            const uint8_t &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);
            appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.plants.value(plant).rewards.reputation);
        }
    }
    else
    {
        if(!seed_in_waiting.first().map.isEmpty())
        {
            MapController::mapController->remove_plant_full(seed_in_waiting.first().map,seed_in_waiting.first().x,seed_in_waiting.first().y);
            cancelAllPlantQuery(seed_in_waiting.first().map,seed_in_waiting.first().x,seed_in_waiting.first().y);
        }
        add_to_inventory(seed_in_waiting.first().seedItemId,1,false);
        showTip(tr("Seed cannot be planted"));
        load_inventory();
    }
    seed_in_waiting.removeFirst();
}

void BaseWindow::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    removeQuery(QueryType_CollectPlant);
    switch(stat)
    {
        case Plant_collect_correctly_collected:
            showTip(tr("Plant collected"));
        break;
        case Plant_collect_empty_dirt:
            showTip(tr("Try collect an empty dirt"));
        break;
        case Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player"));
            MapController::mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;
        case Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
            MapController::mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;
        default:
            qDebug() << "BaseWindow::plant_collected(): unknown return";
        break;
    }
    plant_collect_in_waiting.removeFirst();
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
    QHashIterator<uint16_t,uint32_t> i(items);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.itemToPlants.contains(i.key()))
        {
            const uint8_t &plantId=DatapackClientLoader::datapackLoader.itemToPlants.value(i.key());
            QListWidgetItem *item;
            item=new QListWidgetItem();
            plants_items_to_graphical[plantId]=item;
            plants_items_graphical[item]=plantId;
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].image);
                item->setText(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name+"\n"+tr("Quantity: %1").arg(i.value()));
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                item->setText(QStringLiteral("item id: %1").arg(i.key())+"\n"+tr("Quantity: %1").arg(i.value()));
            }
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.plants.value(plantId).requirements.reputation))
            {
                item->setText(item->text()+"\n"+tr("You don't have the requirements"));
                item->setFont(disableIntoListFont);
                item->setForeground(disableIntoListBrush);
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
    QSetIterator<uint16_t> i(informations.recipes);
    while (i.hasNext())
    {
        uint32_t recipe=i.next();
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
            qDebug() << QStringLiteral("BaseWindow::load_crafting_inventory(), crafting id not found into crafting recipe").arg(recipe);
    }
    on_listCraftingList_itemSelectionChanged();
}

QString BaseWindow::reputationRequirementsToText(const ReputationRequirements &reputationRequirements)
{
    if(reputationRequirements.reputationId>=CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
        return QStringLiteral("???");
    const Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRequirements.reputationId);
    if(!DatapackClientLoader::datapackLoader.reputationExtra.contains(reputation.name))
        return QStringLiteral("???");
    const DatapackClientLoader::ReputationExtra &reputationExtra=DatapackClientLoader::datapackLoader.reputationExtra.value(reputation.name);
    if(reputationRequirements.positif)
    {
        if(reputationRequirements.level>=reputationExtra.reputation_positive.size())
            return QStringLiteral("???");
        else
            return reputationExtra.reputation_positive.at(reputationRequirements.level);
    }
    else
    {
        if(reputationRequirements.level>=reputationExtra.reputation_negative.size())
            return QStringLiteral("???");
        else
            return reputationExtra.reputation_negative.at(reputationRequirements.level);
    }
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
        ui->labelPlantRequirementsAndRewards->setText(QString());
        #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
        ui->labelPlantByDay->setText(QString());
        #endif

        ui->plantUse->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    const DatapackClientLoader::PlantExtra &contentExtra=DatapackClientLoader::datapackLoader.plantExtra[plants_items_graphical[item]];
    const CatchChallenger::Plant &plant=CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]];

    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(plant.itemUsed))
    {
        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(plant.itemUsed);
        ui->labelPlantImage->setPixmap(itemExtra.image);
        ui->labelPlantName->setText(itemExtra.name);
        ui->labelPlantFruitImage->setPixmap(itemExtra.image);
        ui->labelPlantDescription->setText(itemExtra.description);
        //requirements and rewards
        {
            QStringList requirements;
            {
                int index=0;
                while(index<plant.requirements.reputation.size())
                {
                    requirements << reputationRequirementsToText(plant.requirements.reputation.at(index));
                    index++;
                }
            }
            if(requirements.isEmpty())
                ui->labelPlantRequirementsAndRewards->setText(QString());
            else
                ui->labelPlantRequirementsAndRewards->setText(tr("Requirements: ")+requirements.join(QStringLiteral(", ")));
            #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
            QStringList rewards_less_reputation,rewards_more_reputation;
            {
                int index=0;
                while(index<plant.rewards.reputation.size())
                {
                    QString name=QStringLiteral("???");
                    const ReputationRewards &reputationRewards=plant.rewards.reputation.at(index);
                    if(reputationRewards.reputationId<CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
                    {
                        const Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRewards.reputationId);
                        if(DatapackClientLoader::datapackLoader.reputationExtra.contains(reputation.name))
                            name=DatapackClientLoader::datapackLoader.reputationExtra.value(reputation.name).name;
                    }
                    if(reputationRewards.point<0)
                        rewards_less_reputation << name;
                    else if(reputationRewards.point>0)
                        rewards_more_reputation << name;
                    index++;
                }
            }
            if(!rewards_less_reputation.isEmpty() || !rewards_more_reputation.isEmpty())
            {
                ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+QStringLiteral("<br />"));
                if(!rewards_less_reputation.isEmpty())
                    ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+tr("Less reputation in: ")+rewards_less_reputation.join(QStringLiteral(", ")));
                if(!rewards_less_reputation.isEmpty() && !rewards_more_reputation.isEmpty())
                    ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+QStringLiteral(", "));
                if(!rewards_more_reputation.isEmpty())
                    ui->labelPlantRequirementsAndRewards->setText(ui->labelPlantRequirementsAndRewards->text()+tr("More reputation in: ")+rewards_more_reputation.join(QStringLiteral(", ")));
            }
            #endif
        }
        #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
        if(plant.fruits_seconds>0)
        {
            double quantity=(double)plant.fix_quantity+(double)plant.random_quantity/(double)RANDOM_FLOAT_PART_DIVIDER-1;
            double quantityByDay=quantity*(double)86400/(double)plant.fruits_seconds;
            if(CommonDatapack::commonDatapack.items.item.contains(plant.itemUsed))
                ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0))+", "+tr("income by day: %1").arg(QString::number(quantityByDay*CommonDatapack::commonDatapack.items.item[plant.itemUsed].price,'f',0)));
            else
                ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0)));
        }
        #endif
    }
    else
    {
        ui->labelPlantImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantName->setText(tr("Unknow plant (%1)").arg(plant.itemUsed));
        ui->labelPlantFruitImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->labelPlantDescription->setText(tr("This plant and these effects are unknown"));
        ui->labelPlantRequirementsAndRewards->setText(QString());
        #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
        if(plant.fruits_seconds>0)
        {
            double quantity=(double)plant.fix_quantity+(double)plant.random_quantity/(double)RANDOM_FLOAT_PART_DIVIDER-1;
            double quantityByDay=quantity*(double)86400/(double)plant.fruits_seconds;
            ui->labelPlantByDay->setText(tr("Plant by day: %1").arg(QString::number(quantityByDay,'f',0)));
        }
        #endif
    }

    ui->labelPlantedImage->setPixmap(contentExtra.tileset->tileAt(0)->image().scaled(32,64));
    ui->labelSproutedImage->setPixmap(contentExtra.tileset->tileAt(1)->image().scaled(32,64));
    ui->labelTallerImage->setPixmap(contentExtra.tileset->tileAt(2)->image().scaled(32,64));
    ui->labelFloweringImage->setPixmap(contentExtra.tileset->tileAt(3)->image().scaled(32,64));
    ui->labelFruitsImage->setPixmap(contentExtra.tileset->tileAt(4)->image().scaled(32,64));

    ui->labelPlantedText->setText(FacilityLibGeneral::secondsToString(0));
    ui->labelSproutedText->setText(FacilityLibGeneral::secondsToString(plant.sprouted_seconds));
    ui->labelTallerText->setText(FacilityLibGeneral::secondsToString(plant.taller_seconds));
    ui->labelFloweringText->setText(FacilityLibGeneral::secondsToString(plant.flowering_seconds));
    ui->labelFruitsText->setText(FacilityLibGeneral::secondsToString(plant.fruits_seconds));
    ui->labelPlantFruitText->setText(tr("Quantity: %1").arg((float)plant.fix_quantity+((float)plant.random_quantity)/RANDOM_FLOAT_PART_DIVIDER));

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
    ui->labelCraftingDetails->setText(tr("Name: <b>%1</b><br /><br />Success: <b>%2%</b><br /><br />Result: <b>%3</b>").arg(name).arg(content.success).arg(content.quantity));

    //load the materials
    bool haveMaterials=true;
    int index=0;
    QString nameMaterials;
    QListWidgetItem *item;
    uint32_t quantity;
    while(index<content.materials.size())
    {
        //load the material item
        item=new QListWidgetItem();
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(content.materials.at(index).item))
        {
            nameMaterials=DatapackClientLoader::datapackLoader.itemsExtra[content.materials.at(index).item].name;
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[content.materials.at(index).item].image);
        }
        else
        {
            nameMaterials=tr("Unknow item (%1)").arg(content.materials.at(index).item);
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(items.contains(content.materials.at(index).item))
            quantity=items[content.materials.at(index).item];

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
    int index=0;
    while(index<content.materials.size())
    {
        if(!items.contains(content.materials.at(index).item))
            return;
        if(items[content.materials.at(index).item]<content.materials.at(index).quantity)
            return;
        uint32_t sub_index=0;
        while(sub_index<content.materials.at(index).quantity)
        {
            mIngredients << QUrl::fromLocalFile(DatapackClientLoader::datapackLoader.itemsExtra[content.materials.at(index).item].imagePath).toEncoded();
            sub_index++;
        }
        index++;
    }
    index=0;
    QList<QPair<uint32_t,uint32_t> > recipeUsage;
    while(index<content.materials.size())
    {
        QPair<uint32_t,uint32_t> pair;
        pair.first=content.materials.at(index).item;
        pair.second=content.materials.at(index).quantity;
        remove_to_inventory(pair.first,pair.second);
        recipeUsage << pair;
        index++;
    }
    materialOfRecipeInUsing << recipeUsage;
    //the product do
    QPair<uint32_t,uint32_t> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing << pair;
    mProduct=QUrl::fromLocalFile(DatapackClientLoader::datapackLoader.itemsExtra[content.doItemId].imagePath).toEncoded();
    mRecipe=QUrl::fromLocalFile(DatapackClientLoader::datapackLoader.itemsExtra[content.itemToLearn].imagePath).toEncoded();
    //update the UI
    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    CatchChallenger::Api_client_real::client->useRecipe(crafting_recipes_items_graphical.value(selectedItem));
    appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.value(crafting_recipes_items_graphical.value(selectedItem)).rewards.reputation);
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
    craftingAnimationObject=new CraftingAnimation(mIngredients,mRecipe,mProduct,QUrl::fromLocalFile(playerBackImagePath).toEncoded());
    animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
    animationWidget->rootContext()->setContextProperty("craftingAnimationObject",craftingAnimationObject);
    const QString datapackQmlFile=CatchChallenger::Api_client_real::client->datapackPathBase()+"qml/crafting-animation.qml";
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
            materialOfRecipeInUsing.removeFirst();
            add_to_inventory(productOfRecipeInUsing.first().first,productOfRecipeInUsing.first().second);
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
                add_to_inventory(materialOfRecipeInUsing.first().at(index).first,materialOfRecipeInUsing.first().at(index).first,false);
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

