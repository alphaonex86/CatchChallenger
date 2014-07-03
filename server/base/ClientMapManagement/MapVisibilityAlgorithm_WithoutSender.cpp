#include "MapVisibilityAlgorithm_WithoutSender.h"
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
        {
            int index=0;
            const int &list_size=GlobalServerData::serverPrivateVariables.map_list.size();
            while(index<list_size)
            {
                static_cast<Map_server_MapVisibility_Simple_StoreOnSender*>(GlobalServerData::serverPrivateVariables.flat_map_list[index])->purgeBuffer();
                index++;
            }
        }
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
        default:
        break;
    }
}
