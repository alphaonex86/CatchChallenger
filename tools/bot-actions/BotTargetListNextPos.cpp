#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>

//if(target.bestPath.empty()) return real target, else near search the next block
std::pair<uint8_t, uint8_t> BotTargetList::getNextPosition(const MapServerMini::BlockObject * const blockObject,ActionsBotInterface::GlobalTarget &target)
{
    std::pair<uint8_t,uint8_t> point(0,0);
    if(target.blockObject==NULL)
    {
        std::cerr << "error: target.blockObject==NULL into getNextPosition()" << std::endl;
        return point;
    }
    if(target.bestPath.empty())
    {
        switch(target.type)
        {
            case ActionsBotInterface::GlobalTarget::ItemOnMap:
            for(auto it = blockObject->pointOnMap_Item.begin();it!=blockObject->pointOnMap_Item.cend();++it)
            {
                const MapServerMini::ItemOnMap &itemOnMap=it->second;
                if(itemOnMap.item==target.extra)
                    return it->first;
            }
            break;
            case ActionsBotInterface::GlobalTarget::Fight:
                for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
                {
                    const std::vector<uint32_t> &botsFightList=it->second;
                    if(vectorcontainsAtLeastOne(botsFightList,target.extra))
                        return it->first;
                }
            break;
            case ActionsBotInterface::GlobalTarget::Shop:
                for(auto it = blockObject->shops.begin();it!=blockObject->shops.cend();++it)
                {
                    const std::vector<uint32_t> &shops=it->second;
                    if(vectorcontainsAtLeastOne(shops,target.extra))
                        return it->first;
                }
            break;
            case ActionsBotInterface::GlobalTarget::Heal:
                for(auto it = blockObject->heal.begin();it!=blockObject->heal.cend();++it)
                    return *it;
            break;
            case ActionsBotInterface::GlobalTarget::WildMonster:
                QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded (2): %1").arg(target.type));
                target.blockObject=NULL;
                target.extra=0;
                target.linkPoint.x=0;
                target.linkPoint.y=0;
                target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
                target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
                abort();
            break;
            case ActionsBotInterface::GlobalTarget::Dirt:
            {
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(target.extra);
                point.first=plantOrDirt.x;
                point.second=plantOrDirt.y;
                return point;
            }
            break;
            case ActionsBotInterface::GlobalTarget::Plant:
            {
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(target.extra);
                point.first=plantOrDirt.x;
                point.second=plantOrDirt.y;
                return point;
            }
            break;
            default:
                QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded (3): %1").arg(target.type));
                target.blockObject=NULL;
                target.extra=0;
                target.linkPoint.x=0;
                target.linkPoint.y=0;
                target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
                target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
                abort();
            break;
        }
    }
    else
        abort();
    return point;
}
