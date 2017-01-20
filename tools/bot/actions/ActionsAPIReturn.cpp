#include "ActionsAction.h"

void ActionsAction::seed_planted(const bool &ok)
{
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
                QMessageBox::critical(this,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                return;
            }
            const uint8_t &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);
            appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plant).rewards.reputation);
        }
    }
    else
    {
        if(!seed_in_waiting.first().map.isEmpty())
        {
            mapController->remove_plant_full(seed_in_waiting.first().map,seed_in_waiting.first().x,seed_in_waiting.first().y);
            cancelAllPlantQuery(seed_in_waiting.first().map,seed_in_waiting.first().x,seed_in_waiting.first().y);
        }
        add_to_inventory(seed_in_waiting.first().seedItemId,1,false);
        showTip(tr("Seed cannot be planted"));
        load_inventory();
    }
    seed_in_waiting.removeFirst();
}

void ActionsAction::plant_collected(const CatchChallenger::Plant_collect &stat)
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
            mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;
        case Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
            mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;
        default:
            qDebug() << "BaseWindow::plant_collected(): unknown return";
        break;
    }
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
        plant_collect_in_waiting.removeFirst();
}
