#include "MapServerMini.h"
#include <iostream>
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "ActionsAction.h"

bool MapServerMini::preload_step2()
{
    std::vector<std::pair<uint8_t,uint8_t> > scanList;
    if(this->width==0 || this->height==0)
        return false;
    if(step.size()!=1)
        return false;
    const MapParsedForBot &step1=step.at(0);
    if(step1.map==NULL)
        return false;
    //control the layer 1
    {
        unsigned int index=0;
        while(index<step1.layers.size())
        {
            const MapParsedForBot::Layer &layer=step1.layers.at(index);
            if(layer.blockObject==NULL)
                abort();
            index++;
        }
    }

    MapParsedForBot step2;
    step2.map=(uint16_t *)malloc(width*height*sizeof(uint16_t));
    memset(step2.map,0x00,width*height*sizeof(uint16_t));//by default: not accessible zone
    uint16_t lastCodeZone=1;
    uint16_t currentCodeZoneStep1=0;

    //detect the block
    {
        int y=0;
        while(y<this->height)
        {
            int x=0;
            while(x<this->width)
            {
                if(step2.map[x+y*this->width]==0 && step1.map[x+y*this->width]!=0)
                {
                    currentCodeZoneStep1=step1.map[x+y*this->width];
                    step2.map[x+y*this->width]=lastCodeZone;
                    const MapParsedForBot::Layer &step1Layer=step1.layers.at(step1.map[x+y*this->width]-1);
                    if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
                    {
                        preload_step2_addNearTileToScanList(scanList,x,y);
                        while(!scanList.empty())
                        {
                            std::pair<uint8_t,uint8_t> pair=scanList.at(0);
                            scanList.erase(scanList.begin());
                            uint8_t x=pair.first,y=pair.second;
                            if(step2.map[x+y*this->width]==0 && step1.map[x+y*this->width]!=0)
                            {
                                if(step1.map[x+y*this->width]==currentCodeZoneStep1)
                                {
                                    step2.map[x+y*this->width]=lastCodeZone;
                                    preload_step2_addNearTileToScanList(scanList,x,y);
                                }
                            }
                        }
                    }
                    if(lastCodeZone>=65535)
                    {
                        std::cerr << "Too many zone" << std::endl;
                        abort();
                    }
                    MapParsedForBot::Layer layer;
                    layer.name="Block "+std::to_string(lastCodeZone);
                    layer.text=step1Layer.text;
                    step2.layers.push_back(layer);

                    lastCodeZone++;
                }
                x++;
            }
            y++;
        }
        {
            MapParsedForBot::Layer layer;
            layer.name="Lost layer";
            layer.text="";
            step2.layers.push_back(layer);
        }
    }

    //create the object
    {
        unsigned int index=0;
        while(index<step2.layers.size())
        {
            MapParsedForBot::Layer &layer=step2.layers.at(index);
            layer.blockObject=new BlockObject();
            BlockObject blockObject;
            blockObject.map=this;
            blockObject.id=index;

            blockObject.monstersCollisionValue=NULL;
            blockObject.color=MapServerMini::colorsList.at(index%MapServerMini::colorsList.size());

            blockObject.layer=NULL;
            *(layer.blockObject)=blockObject;
            index++;
        }
    }

    /*CatchChallenger::MapCondition mapConditionEmpty;
    mapConditionEmpty.type=CatchChallenger::MapConditionType_None;
    mapConditionEmpty.data.fightBot=0;
    mapConditionEmpty.data.item=0;
    mapConditionEmpty.data.quest=0;*/

    step.push_back(step2);
    //link the object
    {
        unsigned int index=0;
        while(index<step.at(step.size()-1).layers.size())
        {
            MapParsedForBot::Layer &layer=step[step.size()-1].layers.at(index);
            layer.blockObject->layer=&layer;
            index++;
        }
    }
    return true;
}

void MapServerMini::preload_step2_addNearTileToScanList(std::vector<std::pair<uint8_t,uint8_t> > &scanList,const uint8_t &x,const uint8_t &y)
{
    if(x>0)
    {
        std::pair<uint8_t,uint8_t> p(x-1,y);
        if(!vectorcontainsAtLeastOne(scanList,p))
            scanList.push_back(p);
    }
    if(y>0)
    {
        std::pair<uint8_t,uint8_t> p(x,y-1);
        if(!vectorcontainsAtLeastOne(scanList,p))
            scanList.push_back(p);
    }
    if(x<(this->width-1))
    {
        std::pair<uint8_t,uint8_t> p(x+1,y);
        if(!vectorcontainsAtLeastOne(scanList,p))
            scanList.push_back(p);
    }
    if(y<(this->height-1))
    {
        std::pair<uint8_t,uint8_t> p(x,y+1);
        if(!vectorcontainsAtLeastOne(scanList,p))
            scanList.push_back(p);
    }
}

MapServerMini::BlockObject::LinkCondition &MapServerMini::searchConditionOrCreate(BlockObject::LinkInformation &linkInformationFrom,const CatchChallenger::MapCondition &condition)
{
    unsigned int index=0;
    while(index<linkInformationFrom.linkConditions.size())
    {
        BlockObject::LinkCondition &linkCondition=linkInformationFrom.linkConditions[index];
        switch(linkCondition.condition.type)
        {
            case CatchChallenger::MapConditionType_None:
            return linkCondition;
            case CatchChallenger::MapConditionType_Clan://not do for now
            return linkCondition;
            case CatchChallenger::MapConditionType_FightBot:
                if(linkCondition.condition.data.fightBot==condition.data.fightBot)
                    return linkCondition;
            break;
            case CatchChallenger::MapConditionType_Item:
                if(linkCondition.condition.data.item==condition.data.item)
                    return linkCondition;
            break;
            case CatchChallenger::MapConditionType_Quest:
                 if(linkCondition.condition.data.quest==condition.data.quest)
                    return linkCondition;
            break;
            default:
            break;
        }
        index++;
    }
    BlockObject::LinkCondition newLinkCondition;
    newLinkCondition.condition=condition;
    linkInformationFrom.linkConditions.push_back(newLinkCondition);
    return linkInformationFrom.linkConditions.back();
}

bool MapServerMini::addBlockLink(BlockObject &blockObjectFrom, BlockObject &blockObjectTo, const BlockObject::LinkType &linkSourceFrom,
                                 /*point to go:*/const uint8_t &x,const uint8_t &y,
                                 const CatchChallenger::MapCondition &condition)
{
    //search into the destination
    BlockObject::LinkPoint linkPoint;
    linkPoint.type=linkSourceFrom;
    linkPoint.x=x;
    linkPoint.y=y;
    MapServerMini::BlockObject::LinkCondition &linkCondition=MapServerMini::searchConditionOrCreate(blockObjectFrom.links[&blockObjectTo],condition);
    switch (linkCondition.condition.type) {
        case CatchChallenger::MapConditionType_None:
        case CatchChallenger::MapConditionType_Clan:
        case CatchChallenger::MapConditionType_FightBot:
        case CatchChallenger::MapConditionType_Item:
        case CatchChallenger::MapConditionType_Quest:
        break;
    default:
        abort();
        break;
    }
    linkCondition.points.push_back(linkPoint);
    return true;
}

bool MapServerMini::preload_step2b()
{
    if(step.size()!=2)
        return false;
    MapParsedForBot &step2=step[1];
    if(step2.map==NULL)
        return false;

    CatchChallenger::MapCondition mapConditionEmpty;
    mapConditionEmpty.type=CatchChallenger::MapConditionType_None;
    mapConditionEmpty.data.fightBot=0;
    mapConditionEmpty.data.item=0;
    mapConditionEmpty.data.quest=0;

    //link the internal block
    {
        uint8_t y=0;
        while(y<this->height)
        {
            uint8_t x=0;
            while(x<this->width)
            {
                const uint16_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0)
                {
                    if(this->parsed_layer.ledges==NULL || this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges)
                    {
                        //check the right move
                        {
                            uint16_t otherCodeZone=0;
                            CatchChallenger::CommonMap *current_map=this;
                            COORD_TYPE current_x=x;
                            COORD_TYPE current_y=y;
                            bool canMove=false,needrepeate=false;
                            do
                            {
                                needrepeate=false;
                                canMove=CatchChallenger::MoveOnTheMap::moveWithoutTeleport(CatchChallenger::Direction_move_at_right,&current_map,&current_x,&current_y,true,false);
                                if(!canMove)
                                    break;
                                if(current_map==this && codeZone==otherCodeZone)
                                    break;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                if(casted_map->parsed_layer.ledges!=NULL)
                                {
                                    if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]==
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight)
                                        needrepeate=true;
                                    else if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]!=
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges)
                                        break;
                                }
                                const MapParsedForBot &otherStep2=casted_map->step[1];
                                otherCodeZone=otherStep2.map[current_x+current_y*casted_map->width];
                            } while(needrepeate);
                            if(canMove && otherCodeZone!=0 && (current_map!=this || codeZone!=otherCodeZone))
                            {
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                MapParsedForBot &step2nextMap=casted_map->step[1];
                                BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceInternalRightBlock,x,y,mapConditionEmpty);
                            }
                        }
                        //check the left move
                        {
                            uint16_t otherCodeZone=0;
                            CatchChallenger::CommonMap *current_map=this;
                            COORD_TYPE current_x=x;
                            COORD_TYPE current_y=y;
                            bool canMove=false,needrepeate=false;
                            do
                            {
                                needrepeate=false;
                                canMove=CatchChallenger::MoveOnTheMap::moveWithoutTeleport(CatchChallenger::Direction_move_at_left,&current_map,&current_x,&current_y,true,false);
                                if(!canMove)
                                    break;
                                if(current_map==this && codeZone==otherCodeZone)
                                    break;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                if(casted_map->parsed_layer.ledges!=NULL)
                                {
                                    if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]==
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft)
                                        needrepeate=true;
                                    else if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]!=
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges)
                                        break;
                                }
                                const MapParsedForBot &otherStep2=casted_map->step[1];
                                otherCodeZone=otherStep2.map[current_x+current_y*casted_map->width];
                            } while(needrepeate);
                            if(canMove && otherCodeZone!=0 && (current_map!=this || codeZone!=otherCodeZone))
                            {
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                MapParsedForBot &step2nextMap=casted_map->step[1];
                                BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceInternalLeftBlock,x,y,mapConditionEmpty);
                            }
                        }
                        //check the top move
                        {
                            uint16_t otherCodeZone=0;
                            CatchChallenger::CommonMap *current_map=this;
                            COORD_TYPE current_x=x;
                            COORD_TYPE current_y=y;
                            bool canMove=false,needrepeate=false;
                            do
                            {
                                needrepeate=false;
                                canMove=CatchChallenger::MoveOnTheMap::moveWithoutTeleport(CatchChallenger::Direction_move_at_top,&current_map,&current_x,&current_y,true,false);
                                if(!canMove)
                                    break;
                                if(current_map==this && codeZone==otherCodeZone)
                                    break;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                if(casted_map->parsed_layer.ledges!=NULL)
                                {
                                    if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]==
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop)
                                        needrepeate=true;
                                    else if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]!=
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges)
                                        break;
                                }
                                const MapParsedForBot &otherStep2=casted_map->step[1];
                                otherCodeZone=otherStep2.map[current_x+current_y*casted_map->width];
                            } while(needrepeate);
                            if(canMove && otherCodeZone!=0 && (current_map!=this || codeZone!=otherCodeZone))
                            {
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                MapParsedForBot &step2nextMap=casted_map->step[1];
                                BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceInternalTopBlock,x,y,mapConditionEmpty);
                            }
                        }
                        //check the bottom move
                        {
                            uint16_t otherCodeZone=0;
                            CatchChallenger::CommonMap *current_map=this;
                            COORD_TYPE current_x=x;
                            COORD_TYPE current_y=y;
                            bool canMove=false,needrepeate=false;
                            do
                            {
                                needrepeate=false;
                                canMove=CatchChallenger::MoveOnTheMap::moveWithoutTeleport(CatchChallenger::Direction_move_at_bottom,&current_map,&current_x,&current_y,true,false);
                                if(!canMove)
                                    break;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                if(casted_map->parsed_layer.ledges!=NULL)
                                {
                                    if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]==
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom)
                                        needrepeate=true;
                                    else if(casted_map->parsed_layer.ledges[current_x+current_y*casted_map->width]!=
                                            CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges)
                                        break;
                                }
                                else
                                    break;
                                const MapParsedForBot &otherStep2=casted_map->step[1];
                                otherCodeZone=otherStep2.map[current_x+current_y*casted_map->width];
                            } while(needrepeate);
                            if(canMove && otherCodeZone!=0 && (current_map!=this || codeZone!=otherCodeZone))
                            {
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                MapServerMini *casted_map=static_cast<MapServerMini *>(current_map);
                                MapParsedForBot &step2nextMap=casted_map->step[1];
                                BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceInternalBottomBlock,x,y,mapConditionEmpty);
                            }
                        }
                    }
                }
                x++;
            }
            y++;
        }
    }

    //The teleporter
    {
        unsigned int index=0;
        while(index<teleporter_list_size)
        {
            const Teleporter &teleporterEntry=teleporter[index];//for very small list < 20 teleporter, it's this structure the more fast, code not ready for more than 127
            const uint8_t x=teleporterEntry.source_x,y=teleporterEntry.source_y;
            const uint16_t &codeZone=step2.map[x+y*this->width];
            //if current not dirt or other not walkable layer
            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
            {
                if(codeZone!=0)
                {
                    BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                    const uint8_t newx=teleporterEntry.destination_x,newy=teleporterEntry.destination_y;
                    const MapServerMini *nextMap=static_cast<MapServerMini *>(teleporterEntry.map);
                    if(newx<nextMap->width && newy<nextMap->height)
                    {
                        if(nextMap->step.size()<2)
                            abort();
                        const MapParsedForBot &step2nextMap=nextMap->step.at(1);
                        const uint8_t &otherCodeZone=step2nextMap.map[newx+newy*nextMap->width];
                        if(otherCodeZone!=0)
                            //if the other not dirt or other not walkable layer
                            if(nextMap->parsed_layer.walkable!=NULL && nextMap->parsed_layer.walkable[newx+newy*nextMap->width]!=false)
                            {
                                BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceTeleporter,x,y,teleporterEntry.condition);
                            }
                    }
                    else
                        std::cerr << "tp from: " << map_file << ", next map load: " << nextMap->map_file << " on " << std::to_string(newx) << "," << std::to_string(newy) << " is out of range (" << std::to_string(nextMap->width) << "," << std::to_string(nextMap->height) << ")" << std::endl;
                }
            }
            index++;
        }
    }
    return true;
}

bool MapServerMini::preload_step2c()
{
    if(step.size()!=2)
        return false;

    {
            unsigned int indexLayer=0;
            while(indexLayer<step.size())
            {
                MapParsedForBot &currentStep=step[indexLayer];
                if(currentStep.map!=NULL)
                {
                    if(!mapIsValid(currentStep))
                    {
                        displayConsoleMap(currentStep);
                        abort();
                    }
                    const CatchChallenger::ParsedLayer &parsedLayer=this->parsed_layer;
                    //not clickable item
                    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
                    //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
                    //std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;
                    //insdustry -> skip, no position control on server side
                    //wild monster (and their object, day cycle)

                    //dirt
                    if(parsedLayer.dirt!=NULL)
                    {
                        int y=0;
                        while(y<this->height)
                        {
                            int x=0;
                            while(x<this->width)
                            {
                                if(currentStep.map[x+y*this->width]!=0 && parsedLayer.dirt[x+y*this->width]==true)
                                {
                                    const uint16_t &codeZone=currentStep.map[x+y*this->width];
                                    std::pair<uint8_t,uint8_t> newDirtPoint{x,y};
                                    if(codeZone>0)
                                    {
                                        BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                        blockObject.block.push_back(newDirtPoint);
                                    }
                                    else
                                    {
                                        BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                        blockObject.block.push_back(newDirtPoint);
                                    }
                                }
                                x++;
                            }
                            y++;
                        }
                    }
                    //item on map
                    {
                        for (auto it = this->pointOnMap_Item.begin(); it != this->pointOnMap_Item.cend(); ++it) {
                            const std::pair<uint8_t,uint8_t> &point=it->first;
                            const MapServerMini::ItemOnMap &itemEntry=it->second;
                            const uint16_t &codeZone=currentStep.map[point.first+point.second*this->width];

                            if(codeZone>0)
                            {
                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                blockObject.pointOnMap_Item[point]=itemEntry;
                            }
                            else
                            {
                                BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                blockObject.pointOnMap_Item[point]=itemEntry;
                            }
                        }
                    }
                    //fight
                    {
                        for(const auto& n : this->botsFight) {
                            const std::pair<uint8_t,uint8_t> &point=n.first;
                            const uint8_t x=n.first.first,y=n.first.second;
                            const uint16_t &codeZone=currentStep.map[x+y*this->width];
                            {
                                const std::vector<uint16_t> &fightsList=n.second;
                                if(codeZone>0)
                                {
                                    BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                    blockObject.botsFight[point]=fightsList;
                                }
                                else
                                {
                                    BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                    blockObject.botsFight[point]=fightsList;
                                }
                            }
                        }
                    }
                    //shop
                    {
                        for(const auto& n : this->shops) {
                            const std::pair<uint8_t,uint8_t> &point=n.first;
                            const uint8_t x=n.first.first,y=n.first.second;
                            const uint16_t &codeZone=currentStep.map[x+y*this->width];
                            {
                                const std::vector<uint16_t> &shopList=n.second;
                                if(codeZone>0)
                                {
                                    BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                    blockObject.shops[point]=shopList;
                                }
                                else
                                {
                                    BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                    blockObject.shops[point]=shopList;
                                }
                            }
                        }
                    }
                    //heal
                    {
                        for(const auto& n : this->heal) {
                            const std::pair<uint8_t,uint8_t> point(n.first,n.second);
                            const uint8_t x=n.first,y=n.second;
                            const uint16_t &codeZone=currentStep.map[x+y*this->width];
                            {
                                if(codeZone>0)
                                {
                                    BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                    blockObject.heal.insert(point);
                                }
                                else
                                {
                                    BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                    blockObject.heal.insert(point);
                                }
                            }
                        }
                    }

                    //detect the wild monster
                    if(parsedLayer.monstersCollisionMap!=NULL)
                    {
                        int y=0;
                        while(y<this->height)
                        {
                            int x=0;
                            while(x<this->width)
                            {
                                if(currentStep.map[x+y*this->width]!=0)
                                {
                                    const uint16_t &codeZone=currentStep.map[x+y*this->width];
                                    const uint8_t &monsterCode=parsedLayer.monstersCollisionMap[x+y*this->width];
                                    if(monsterCode<parsedLayer.monstersCollisionList.size())
                                    {
                                        const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=parsedLayer.monstersCollisionList.at(monsterCode);
                                        if(!monstersCollisionValue.walkOnMonsters.empty())
                                        {
                                            std::pair<uint8_t,uint8_t> newPoint{x,y};
                                            if(codeZone>0)
                                            {
                                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                                blockObject.monstersCollisionValue=&monstersCollisionValue;
                                                blockObject.block.push_back(newPoint);
                                            }
                                            else
                                            {
                                                BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                                blockObject.monstersCollisionValue=&monstersCollisionValue;
                                                blockObject.block.push_back(newPoint);
                                            }
                                        }
                                    }
                                }
                                x++;
                            }
                            y++;
                        }
                    }
                }
                else
                    std::cerr << "map with zone code not found: " << map_file << std::endl;

                indexLayer++;
            }
        }

    return true;
}

void MapServerMini::displayConsoleMap(const MapParsedForBot &currentStep) const
{
    if(currentStep.map==NULL)
        return;
    {
        int x=0;
        while(x<(this->width-1))
        {
            std::cout << "-";
            x++;
        }
        std::cout << std::endl;
    }
    int y=0;
    while(y<(this->height-1))
    {
        int x=0;
        while(x<(this->width-1))
        {
            uint16_t codeZone=currentStep.map[x+y*this->width];
            if(codeZone<=0)
                std::cout << " ";
            else if(codeZone<=9)
                std::cout << std::to_string(codeZone);
            else
            {
                codeZone-=10;
                if(codeZone<=25)
                    std::cout << (char)65+codeZone;
                else
                {
                    codeZone-=25;
                    if(codeZone<=25)
                        std::cout << (char)97+codeZone;
                    else
                        std::cout << (char)1+codeZone;
                }
            }
            x++;
        }
        y++;
        std::cout << std::endl;
    }
    {
        int x=0;
        while(x<(this->width-1))
        {
            std::cout << "-";
            x++;
        }
        std::cout << std::endl;
    }
}

bool MapServerMini::mapIsValid(const MapParsedForBot &currentStep) const
{
    if(currentStep.map==NULL)
        return false;
    int y=0;
    while(y<(this->height-1))
    {
        int x=0;
        while(x<(this->width-1))
        {
            const uint16_t codeZone=currentStep.map[x+y*this->width];
            if(codeZone>0)
            {
                if((uint32_t)(codeZone-1)>=currentStep.layers.size())
                    return false;
            }
            x++;
        }
        y++;
    }
    return true;
}
