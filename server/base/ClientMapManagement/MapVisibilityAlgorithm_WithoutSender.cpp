#include "MapVisibilityAlgorithm_WithoutSender.h"
#include "MapVisibilityAlgorithm_Simple_StoreOnReceiver.h"
#include "MapVisibilityAlgorithm_WithBorder_StoreOnReceiver.h"
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
