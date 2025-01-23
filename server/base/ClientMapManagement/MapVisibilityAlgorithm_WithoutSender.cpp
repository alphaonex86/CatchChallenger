#include "MapVisibilityAlgorithm_WithoutSender.hpp"
#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../GlobalServerData.hpp"

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
    if(!GlobalServerData::serverSettings.mapVisibility.simple.enable)
        return;
    if(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update==nullptr)
        return;
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
