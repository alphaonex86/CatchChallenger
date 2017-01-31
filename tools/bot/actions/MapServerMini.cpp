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
    for (auto it = pointOnMap_Item.begin(); it != pointOnMap_Item.end(); ++it) { // calls a_map.begin() and a_map.end()
        const std::pair<uint8_t,uint8_t> &pair=it->first;
        MapServerMini::ItemOnMap &itemOnMap=it->second;
        QString resolvedFileName=DatapackClientLoader::datapackLoader.getDatapackPath()+DatapackClientLoader::datapackLoader.getMainDatapackPath()+QString::fromStdString(map_file);
        if(!resolvedFileName.endsWith(".tmx"))
            resolvedFileName+=".tmx";
        if(DatapackClientLoader::datapackLoader.itemOnMap.contains(resolvedFileName))
        {
            const QHash<QPair<uint8_t,uint8_t>,uint16_t> &item=DatapackClientLoader::datapackLoader.itemOnMap.value(resolvedFileName);
            const QPair<uint8_t,uint8_t> QtPoint(pair.first,pair.second);
            if(item.contains(QtPoint))
                itemOnMap.indexOfItemOnMap=item.value(QtPoint);
            else
                abort();
        }
        else
            abort();
    }
    return true;
}
