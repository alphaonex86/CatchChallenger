#include "MapVisibilityAlgorithm_WithoutSender.h"
#include "MapVisibilityAlgorithm_Simple_StoreOnReceiver.h"
#include "MapVisibilityAlgorithm_WithBorder_StoreOnReceiver.h"
#include "Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "../GlobalServerData.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_WithoutSender MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender;

MapVisibilityAlgorithm_WithoutSender::MapVisibilityAlgorithm_WithoutSender()
{
}

MapVisibilityAlgorithm_WithoutSender::~MapVisibilityAlgorithm_WithoutSender()
{
}

void MapVisibilityAlgorithm_WithoutSender::generalPurgeBuffer()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
        if(GlobalServerData::serverSettings.mapVisibility.simple.storeOnSender)
        {
            int index=0;
            const int &list_size=GlobalServerData::serverPrivateVariables.flat_map_list.size();
            while(index<list_size)
            {
                static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(GlobalServerData::serverPrivateVariables.flat_map_list.at(index))->purgeBuffer();
                index++;
            }
        }
        else
        {
            int index=0;
            const int &list_size=allClient.size();
            while(index<list_size)
            {
                static_cast<MapVisibilityAlgorithm_Simple_StoreOnReceiver*>(allClient.at(index))->purgeBuffer();
                index++;
            }
        }
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
            int index=0;
            const int &list_size=allClient.size();
            while(index<list_size)
            {
                static_cast<MapVisibilityAlgorithm_WithBorder_StoreOnReceiver*>(allClient.at(index))->purgeBuffer();
                index++;
            }
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
        default:
        break;
    }
}
