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
            unsigned int index=0;
            const unsigned int &list_size=Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size;
            while(index<list_size)
            {
                Map_server_MapVisibility_Simple_StoreOnSender * const map=Map_server_MapVisibility_Simple_StoreOnSender::map_to_update[index];
                map->purgeBuffer();
                index++;
            }
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;
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
