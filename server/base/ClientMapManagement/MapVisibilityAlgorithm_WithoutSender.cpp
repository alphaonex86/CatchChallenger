#include "MapVisibilityAlgorithm_WithoutSender.h"
#include "MapVisibilityAlgorithm_Simple.h"
#include "MapVisibilityAlgorithm_WithBorder.h"
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
            quint32 index=0;
            const quint32 &list_size=allClient.size();
            while(index<list_size)
            {
                static_cast<MapVisibilityAlgorithm_Simple*>(allClient.at(index))->purgeBuffer();
                index++;
            }
        }
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
            quint32 index=0;
            const quint32 &list_size=allClient.size();
            while(index<list_size)
            {
                static_cast<MapVisibilityAlgorithm_WithBorder*>(allClient.at(index))->purgeBuffer();
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
