#include "MapServerMini.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"

QList<QColor> MapServerMini::colorsList;

MapServerMini::MapServerMini()
{
    parsed_layer.walkable=NULL;
    parsed_layer.monstersCollisionMap=NULL;
    parsed_layer.dirt=NULL;
    parsed_layer.ledges=NULL;
    teleporter=NULL;
}

bool MapServerMini::preload_other_pre()
{
    return true;
}

bool MapServerMini::preload_post_subdatapack()
{
    std::string resolvedFileName=DatapackClientLoader::datapackLoader.getDatapackPath()+
            DatapackClientLoader::datapackLoader.getMainDatapackPath()+
            map_file;
    if(stringEndsWith(resolvedFileName,".tmx"))
        resolvedFileName+=".tmx";
    for (auto it = pointOnMap_Item.begin(); it != pointOnMap_Item.end(); ++it) { // calls a_map.begin() and a_map.end()
        const std::pair<uint8_t,uint8_t> &pair=it->first;
        MapServerMini::ItemOnMap &itemOnMap=it->second;
        if(DatapackClientLoader::datapackLoader.itemOnMap.find(resolvedFileName)!=DatapackClientLoader::datapackLoader.itemOnMap.cend())
        {
            const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &item=DatapackClientLoader::datapackLoader.itemOnMap.at(resolvedFileName);
            const std::pair<uint8_t,uint8_t> QtPoint(pair.first,pair.second);
            if(item.find(QtPoint)!=item.cend())
            {
                uint32_t indexOfItemOnMap=item.at(QtPoint);
                itemOnMap.indexOfItemOnMap=indexOfItemOnMap;
                const uint16_t &currentCodeZone=step.at(1).map[pair.first+pair.second*width];
                if(currentCodeZone!=0)
                {
                    MapServerMini::BlockObject * blockObject=step.at(1).layers.at(currentCodeZone-1).blockObject;
                    MapServerMini::ItemOnMap &itemOnMap=blockObject->pointOnMap_Item[std::pair<uint8_t,uint8_t>(pair.first,pair.second)];
                    itemOnMap.indexOfItemOnMap=indexOfItemOnMap;
                }
            }
            else
                abort();
        }
        else
            abort();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::unordered_set<uint32_t> known_indexOfItemOnMap;
        for ( const auto &item : pointOnMap_Item )
        {
            const MapServerMini::ItemOnMap &itemOnMap=item.second;
            if(known_indexOfItemOnMap.find(itemOnMap.indexOfItemOnMap)!=known_indexOfItemOnMap.cend())
                abort();
            known_indexOfItemOnMap.insert(itemOnMap.indexOfItemOnMap);
        }
    }
    {
        std::unordered_set<uint32_t> known_indexOfItemOnMap;
        const MapParsedForBot &currentStep=step.at(1);
        unsigned int layerIndex=0;
        while(layerIndex<currentStep.layers.size())
        {
            const BlockObject &blockObject=*currentStep.layers.at(layerIndex).blockObject;

            for ( const auto &item : blockObject.pointOnMap_Item )
            {
                const MapServerMini::ItemOnMap &itemOnMap=item.second;
                if(known_indexOfItemOnMap.find(itemOnMap.indexOfItemOnMap)!=known_indexOfItemOnMap.cend())
                    abort();
                known_indexOfItemOnMap.insert(itemOnMap.indexOfItemOnMap);
            }
            layerIndex++;
        }
    }
    #endif
    return true;
}
