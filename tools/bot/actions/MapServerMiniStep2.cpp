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

    //link the internal block
    {
        int y=0;
        while(y<this->height)
        {
            int x=0;
            while(x<this->width)
            {
                const uint16_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0)
                {
                    //check the right tile
                    if(x<(this->width-1))
                    {
                        uint8_t newx=(x+1),newy=y;
                        const uint16_t &rightCodeZone=step2.map[newx+newy*this->width];
                        if(rightCodeZone!=0 && codeZone!=rightCodeZone)
                        {
                            BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                            BlockObject &rightBlockObject=*step2.layers[rightCodeZone-1].blockObject;
                            //if current not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
                            {
                                //if can go to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight
                                        )
                                {
                                    if(this->parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)this->parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight)
                                            addBlockLink(blockObject,rightBlockObject,BlockObject::LinkType::SourceInternalRightBlock,x,y);
                                    }
                                    else
                                        addBlockLink(blockObject,rightBlockObject,BlockObject::LinkType::SourceInternalRightBlock,x,y);
                                }
                            }
                            //if the other not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[newx+newy*this->width]!=false)
                            {
                                //if can come from to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft
                                        )
                                {
                                    if(this->parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)this->parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft)
                                            addBlockLink(rightBlockObject,blockObject,BlockObject::LinkType::SourceInternalLeftBlock,newx,newy);
                                    }
                                    else
                                        addBlockLink(rightBlockObject,blockObject,BlockObject::LinkType::SourceInternalLeftBlock,newx,newy);
                                }
                            }
                        }
                    }

                    //check the bottom tile
                    if(y<(this->height-1))
                    {
                        uint8_t newx=x,newy=(y+1);
                        const uint16_t &bottomCodeZone=step2.map[newx+newy*this->width];
                        if(bottomCodeZone!=0 && codeZone!=bottomCodeZone)
                        {
                            BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                            BlockObject &bottomBlockObject=*step2.layers[bottomCodeZone-1].blockObject;
                            //if current not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
                            {
                                //if can go to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom
                                        )
                                {
                                    if(this->parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)this->parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom)
                                            addBlockLink(blockObject,bottomBlockObject,BlockObject::LinkType::SourceInternalBottomBlock,x,y);
                                    }
                                    else
                                        addBlockLink(blockObject,bottomBlockObject,BlockObject::LinkType::SourceInternalBottomBlock,x,y);
                                }
                            }
                            //if the other not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[newx+newy*this->width]!=false)
                            {
                                //if can come from to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop
                                        )
                                {
                                    if(this->parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)this->parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop)
                                            addBlockLink(bottomBlockObject,blockObject,BlockObject::LinkType::SourceInternalTopBlock,newx,newy);
                                    }
                                    else
                                        addBlockLink(bottomBlockObject,blockObject,BlockObject::LinkType::SourceInternalTopBlock,newx,newy);
                                }
                            }
                        }
                    }

                    lastCodeZone++;
                }
                x++;
            }
            y++;
        }
    }

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

bool MapServerMini::addBlockLink(BlockObject &blockObjectFrom, BlockObject &blockObjectTo, const BlockObject::LinkType &linkSourceFrom,
                                 /*point to go:*/const uint8_t &x,const uint8_t &y)
{
    //search into the destination
    {
        if(blockObjectTo.links.find(&blockObjectFrom)!=blockObjectTo.links.cend())
        {
            BlockObject::LinkInformation &linkInformationTo=blockObjectTo.links[&blockObjectFrom];
            linkInformationTo.direction=BlockObject::LinkDirection::BothDirection;
            if(blockObjectFrom.links.find(&blockObjectTo)==blockObjectFrom.links.cend())
            {
                BlockObject::LinkInformation linkInformationFrom;
                linkInformationFrom.direction=BlockObject::LinkDirection::BothDirection;
                blockObjectFrom.links[&blockObjectTo]=linkInformationFrom;
            }
            BlockObject::LinkInformation &linkInformationFrom=blockObjectFrom.links[&blockObjectTo];
            BlockObject::LinkPoint linkPoint;
            linkPoint.type=linkSourceFrom;
            linkPoint.x=x;
            linkPoint.y=y;
            linkInformationFrom.points.push_back(linkPoint);
        }
        else
        {
            //search into the source
            if(blockObjectFrom.links.find(&blockObjectTo)==blockObjectFrom.links.cend())
            {
                BlockObject::LinkInformation linkInformationFrom;
                linkInformationFrom.direction=BlockObject::LinkDirection::ToTheTarget;
                blockObjectFrom.links[&blockObjectTo]=linkInformationFrom;
            }
            BlockObject::LinkInformation &linkInformationFrom=blockObjectFrom.links[&blockObjectTo];
            BlockObject::LinkPoint linkPoint;
            linkPoint.type=linkSourceFrom;
            linkPoint.x=x;
            linkPoint.y=y;
            linkInformationFrom.points.push_back(linkPoint);
        }
    }
    return true;
}

bool MapServerMini::preload_step2b()
{
    if(step.size()!=2)
        return false;
    MapParsedForBot &step2=step[1];
    if(step2.map==NULL)
        return false;

    //connect the right border
    if(this->border.right.map!=NULL)
    {
        uint8_t y_start=0,y_stop=0;
        bool isValid=true;
        if(this->border.right.y_offset<0)
        {
            if(-(this->border.right.y_offset)<this->border.right.map->height)
            {
                y_start=0;
                const uint16_t &tot=this->border.right.map->height+this->border.right.y_offset;
                if(tot>this->height)
                    y_stop=this->height;
                else
                    y_stop=tot;
            }
            else
                isValid=false;
        }
        else
        {
            if(this->border.right.y_offset<this->height)
            {
                y_start=this->border.right.y_offset;
                const uint16_t &tot=this->border.right.map->height+this->border.right.y_offset;
                if(tot>this->height)
                    y_stop=this->height;
                else
                    y_stop=tot;
            }
            else
                isValid=false;
        }
        if(isValid)
        {
            const uint8_t &x=this->width-1;
            uint8_t y=y_start;
            while(y<y_stop)
            {
                const uint16_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0)
                {
                    //check the right tile
                    if(this->border.right.map!=NULL)
                    {
                        uint16_t newx=0,newy=(y+this->border.right.y_offset);
                        MapServerMini &nextMap=*static_cast<MapServerMini *>(this->border.right.map);
                        if(nextMap.step.size()<2)
                            abort();
                        BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &rightCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(rightCodeZone!=0)
                        {
                            BlockObject &rightBlockObject=*step2nextMap.layers[rightCodeZone-1].blockObject;
                            //if current not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
                            {
                                //if can go to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*nextMap.width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight)
                                            addBlockLink(blockObject,rightBlockObject,BlockObject::LinkType::SourceRightMap,x,y);
                                    }
                                    else
                                        addBlockLink(blockObject,rightBlockObject,BlockObject::LinkType::SourceRightMap,x,y);
                                }
                            }
                            //if the other not dirt or other not walkable layer
                            if(nextMap.parsed_layer.walkable!=NULL && nextMap.parsed_layer.walkable[newx+newy*nextMap.width]!=false)
                            {
                                //if can come from to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*nextMap.width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft)
                                            addBlockLink(rightBlockObject,blockObject,BlockObject::LinkType::SourceLeftMap,newx,newy);
                                    }
                                    else
                                        addBlockLink(rightBlockObject,blockObject,BlockObject::LinkType::SourceLeftMap,newx,newy);
                                }
                            }
                        }
                    }
                }
                y++;
            }
        }
    }

    //connect the bottom border
    if(this->border.bottom.map!=NULL)
    {
        uint8_t x_start=0,x_stop=0;
        bool isValid=true;
        if(this->border.bottom.x_offset<0)
        {
            if(-(this->border.bottom.x_offset)<this->border.bottom.map->width)
            {
                x_start=0;
                const uint16_t &tot=this->border.bottom.map->width+this->border.bottom.x_offset;
                if(tot>this->width)
                    x_stop=this->width;
                else
                    x_stop=tot;
            }
            else
                isValid=false;
        }
        else
        {
            if(this->border.bottom.x_offset<this->width)
            {
                x_start=this->border.bottom.x_offset;
                const uint16_t &tot=this->border.bottom.map->width+this->border.bottom.x_offset;
                if(tot>this->width)
                    x_stop=this->width;
                else
                    x_stop=tot;
            }
            else
                isValid=false;
        }
        if(isValid)
        {
            const uint8_t &y=this->height-1;
            uint8_t x=x_start;
            while(x<x_stop)
            {
                const uint16_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0)
                {
                    //check the bottom tile
                    if(this->border.bottom.map!=NULL)
                    {
                        uint16_t newx=(x+this->border.bottom.x_offset),newy=0;
                        MapServerMini &nextMap=*static_cast<MapServerMini *>(this->border.bottom.map);
                        if(nextMap.step.size()<2)
                            abort();
                        BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &bottomCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(bottomCodeZone!=0)
                        {
                            BlockObject &bottomBlockObject=*step2nextMap.layers[bottomCodeZone-1].blockObject;
                            //if current not dirt or other not walkable layer
                            if(parsed_layer.walkable!=NULL && parsed_layer.walkable[x+y*this->width]!=false)
                            {
                                //if can go to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*nextMap.width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom)
                                            addBlockLink(blockObject,bottomBlockObject,BlockObject::LinkType::SourceBottomMap,x,y);
                                    }
                                    else
                                        addBlockLink(blockObject,bottomBlockObject,BlockObject::LinkType::SourceBottomMap,x,y);
                                }
                            }
                            //if the other not dirt or other not walkable layer
                            if(nextMap.parsed_layer.walkable!=NULL && nextMap.parsed_layer.walkable[newx+newy*nextMap.width]!=false)
                            {
                                //if can come from to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*nextMap.width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop)
                                            addBlockLink(bottomBlockObject,blockObject,BlockObject::LinkType::SourceTopMap,newx,newy);
                                    }
                                    else
                                        addBlockLink(bottomBlockObject,blockObject,BlockObject::LinkType::SourceTopMap,newx,newy);
                                }
                            }
                        }
                    }
                }

                x++;
            }
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
                                addBlockLink(blockObject,otherBlockObject,BlockObject::LinkType::SourceTeleporter,x,y);
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
                                        blockObject.dirt.push_back(newDirtPoint);
                                    }
                                    else
                                    {
                                        BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                        blockObject.dirt.push_back(newDirtPoint);
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
                                const std::vector<uint32_t> &fightsList=n.second;
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
                                const std::vector<uint32_t> &shopList=n.second;
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
                                            if(codeZone>0)
                                            {
                                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                                blockObject.monstersCollisionValue=&monstersCollisionValue;
                                            }
                                            else
                                            {
                                                BlockObject &blockObject=*currentStep.layers[currentStep.layers.size()-1].blockObject;
                                                blockObject.monstersCollisionValue=&monstersCollisionValue;
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
