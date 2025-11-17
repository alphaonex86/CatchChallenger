#include "MapVisibilityAlgorithm_WithoutSender.hpp"
#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/ProtocolParsing.hpp"

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
    if(!GlobalServerData::serverSettings.mapVisibility.enable)
        return;
    //if display 0 or 1 player mean just display them self
    if(!GlobalServerData::serverSettings.mapVisibility.simple.max<2)
        return;

    unsigned int index=0;
    switch(GlobalServerData::serverSettings.mapVisibility.minimize)
    {
    case GameServerSettings::MapVisibility::Minimize_CPU:
       while(index<Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())//put loop into condition to have best performance
       {
           Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(index).min_CPU();
           index++;
       }
       break;
    case GameServerSettings::MapVisibility::Minimize_Network:
       ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;
       ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;//map list count
       while(index<Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())//put loop into condition to have best performance
       {
           Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(index).min_network();
           index++;
       }
       break;
    }
}
