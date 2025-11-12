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
    //if display 0 or 1 player mean just display them self
    if(!GlobalServerData::serverSettings.mapVisibility.simple.max<2)
        return;
    unsigned int index=0;
    while(index<Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())
    {
        Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(index).purgeBuffer();
        index++;
    }
}
