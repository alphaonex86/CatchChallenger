#include "ActionsAction.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsServer.h"

#include <QMessageBox>
#include <QString>

void ActionsAction::seed_planted(const bool &ok)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(!ActionsBotInterface::clientList.contains(api))
        return;
    ActionsBotInterface::Player &player=ActionsBotInterface::clientList[api];
    CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
    if(ok)
    {
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted"));
        //do the rewards
        {
            const uint32_t &itemId=player.seed_in_waiting.first().seedItemId;
            if(!DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(NULL,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                return;
            }
            const uint8_t &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);
            appendReputationRewards(api,CatchChallenger::CommonDatapack::commonDatapack.plants.at(plant).rewards.reputation);
        }
    }
    else
    {
        playerInformations.plantOnMap.erase(player.seed_in_waiting.first().indexOnMap);
        add_to_inventory(api,player.seed_in_waiting.first().seedItemId,1);
        showTip(tr("Seed cannot be planted"));
    }
    player.seed_in_waiting.removeFirst();
}

void ActionsAction::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(!ActionsBotInterface::clientList.contains(api))
        return;
    ActionsBotInterface::Player &player=ActionsBotInterface::clientList[api];
    switch(stat)
    {
        case CatchChallenger::Plant_collect_correctly_collected:
            showTip(tr("Plant collected"));
        break;
        case CatchChallenger::Plant_collect_empty_dirt:
            showTip(tr("Try collect an empty dirt"));
        break;
        case CatchChallenger::Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player"));
            /*mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);*/
        break;
        case CatchChallenger::Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
            /*mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);*/
        break;
        default:
            qDebug() << "BaseWindow::plant_collected(): unknown return";
        break;
    }
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
        player.plant_collect_in_waiting.removeFirst();
}
