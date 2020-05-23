#include "ActionsAction.h"
#include "../../client/qt/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../client/qt/Api_client_real.hpp"

#include <QMessageBox>
#include <QString>

void ActionsAction::seed_planted_slot(const bool &ok)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    seed_planted(api,ok);
}

void ActionsAction::seed_planted(CatchChallenger::Api_protocol_Qt *api,const bool &ok)
{
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    ActionsBotInterface::Player &player=ActionsBotInterface::clientList[api];
    CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
    if(ok)
    {
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted"));
        //do the rewards
        {
            const uint32_t &itemId=player.seed_in_waiting.front().seedItemId;
            if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(itemId)==QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(NULL,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                return;
            }
            const uint8_t &plant=QtDatapackClientLoader::datapackLoader->itemToPlants.at(itemId);
            appendReputationRewards(api,CatchChallenger::CommonDatapack::commonDatapack.plants.at(plant).rewards.reputation);
        }
    }
    else
    {
        playerInformations.plantOnMap.erase(player.seed_in_waiting.front().indexOnMap);
        add_to_inventory(api,player.seed_in_waiting.front().seedItemId,1);
        showTip(tr("Seed cannot be planted"));
    }
    player.seed_in_waiting.erase(player.seed_in_waiting.cbegin());
}

void ActionsAction::plant_collected_slot(const CatchChallenger::Plant_collect &stat)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    plant_collected(api,stat);
}

void ActionsAction::plant_collected(CatchChallenger::Api_protocol_Qt *api,const CatchChallenger::Plant_collect &stat)
{
    if(ActionsBotInterface::clientList.find(api)==ActionsBotInterface::clientList.cend())
        return;
    ActionsAction::Player &player=actionsAction->clientList[api];
    if(player.plant_collect_in_waiting.empty())
        abort();
    CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
    const Player::ClientPlantInCollecting &plantInCollecting=player.plant_collect_in_waiting.at(0);
    switch(stat)
    {
        case CatchChallenger::Plant_collect_correctly_collected:
            //see to optimise CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true and use the internal random number list
            showTip(tr("Plant collected"));//the item is send by another message with the protocol
            playerInformations.plantOnMap.erase(plantInCollecting.indexOnMap);
        break;
        /*case CatchChallenger::Plant_collect_empty_dirt:
            showTip(tr("Try collect an empty dirt"));
        break;
        case CatchChallenger::Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player"));
            //mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            //cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;
        case CatchChallenger::Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
            //mapController->insert_plant_full(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y,plant_collect_in_waiting.first().plant_id,plant_collect_in_waiting.first().seconds_to_mature);
            //cancelAllPlantQuery(plant_collect_in_waiting.first().map,plant_collect_in_waiting.first().x,plant_collect_in_waiting.first().y);
        break;*/
        default:
            qDebug() << "BaseWindow::plant_collected(): unknown return";
        break;
    }
    /*if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
        player.plant_collect_in_waiting.erase(player.plant_collect_in_waiting.cbegin());*/
}
