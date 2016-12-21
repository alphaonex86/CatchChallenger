#include "MapServerMini.h"
#include <iostream>

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

    MapParsedForBot step2;
    step2.map=(uint8_t *)malloc(width*height);
    memset(step2.map,0x00,width*height);//by default: not accessible zone
    uint8_t lastCodeZone=1;
    uint8_t currentCodeZoneStep1=0;

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
                    if(lastCodeZone>=255)
                    {
                        std::cerr << "Too many zone" << std::endl;
                        abort();
                    }
                    const MapParsedForBot::Layer &step1Layer=step1.layers.at(step1.map[x+y*this->width]-1);
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
    }

    //create the object
    {
        unsigned int index=0;
        while(index<step2.layers.size())
        {
            const MapParsedForBot::Layer &layer=step2.layers.at(index);
            BlockObject blockObject;
            blockObject.text=layer.text;
            blockObject.name=layer.name;
            blockObject.map=this;
            blockObject.id=index;
            blockList.push_back(blockObject);
            index++;
        }
    }

    //link the internal block
    {
        int y=0;
        while(y<(this->height-1))
        {
            int x=0;
            while(x<(this->width-1))
            {
                const uint8_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0 && (this->botLayerMask==NULL || this->botLayerMask[x+y*this->width]==0))
                {
                    //check the right tile
                    {
                        uint8_t newx=(x+1),newy=y;
                        const uint8_t &rightCodeZone=step2.map[newx+newy*this->width];
                        if(rightCodeZone!=0 && codeZone!=rightCodeZone)
                            if(this->botLayerMask==NULL || this->botLayerMask[newx+newy*this->width]==0)
                            {
                                BlockObject &blockObject=blockList.at(codeZone-1);
                                BlockObject &rightBlockObject=blockList.at(rightCodeZone-1);
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
                                            addBlockLink(blockObject,rightBlockObject);
                                    }
                                    else
                                        addBlockLink(blockObject,rightBlockObject);
                                }
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
                                            addBlockLink(rightBlockObject,blockObject);
                                    }
                                    else
                                        addBlockLink(rightBlockObject,blockObject);
                                }
                            }
                    }

                    //check the bottom tile
                    {
                        uint8_t newx=x,newy=(y+1);
                        const uint8_t &bottomCodeZone=step2.map[newx+newy*this->width];
                        if(bottomCodeZone!=0 && codeZone!=bottomCodeZone)
                            if(this->botLayerMask==NULL || this->botLayerMask[newx+newy*this->width]==0)
                            {
                                BlockObject &blockObject=blockList.at(codeZone-1);
                                BlockObject &bottomBlockObject=blockList.at(bottomCodeZone-1);
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
                                            addBlockLink(blockObject,bottomBlockObject);
                                    }
                                    else
                                        addBlockLink(blockObject,bottomBlockObject);
                                }
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
                                            addBlockLink(bottomBlockObject,blockObject);
                                    }
                                    else
                                        addBlockLink(bottomBlockObject,blockObject);
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

bool MapServerMini::addBlockLink(BlockObject &blockObjectFrom,BlockObject &blockObjectTo)
{
    //search into the destination
    {
        if(blockObjectTo.links.find(&blockObjectFrom)!=blockObjectTo.links.cend())
        {
            blockObjectTo.links[&blockObjectFrom]=BlockObject::LinkType::BothDirection;
            blockObjectFrom.links[&blockObjectTo]=BlockObject::LinkType::BothDirection;
        }
        else
        {
            //search into the source
            if(blockObjectFrom.links.find(&blockObjectTo)==blockObjectFrom.links.cend())
                blockObjectFrom.links[&blockObjectTo]=BlockObject::LinkType::ToTheTarget;
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
                const uint8_t codeZone=step2.map[x+y*this->width];
                if(codeZone!=0 && (this->botLayerMask==NULL || this->botLayerMask[x+y*this->width]==0))
                {
                    //check the right tile
                    if(this->border.right.map!=NULL)
                    {
                        uint16_t newx=0,newy=(y+this->border.right.y_offset);
                        MapServerMini &nextMap=*static_cast<MapServerMini *>(this->border.right.map);
                        if(nextMap.step.size()<2)
                            abort();
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &rightCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(rightCodeZone!=0)
                            if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                            {
                                BlockObject &blockObject=blockList.at(codeZone-1);
                                BlockObject &rightBlockObject=nextMap.blockList.at(rightCodeZone-1);
                                //if can go to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesRight)
                                            addBlockLink(blockObject,rightBlockObject);
                                    }
                                    else
                                        addBlockLink(blockObject,rightBlockObject);
                                }
                                //if can come from to right
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesLeft)
                                            addBlockLink(rightBlockObject,blockObject);
                                    }
                                    else
                                        addBlockLink(rightBlockObject,blockObject);
                                }
                            }
                    }
                    //check the bottom tile
                    if(this->border.bottom.map!=NULL)
                    {
                        uint16_t newx=0,newy=(y+this->border.bottom.x_offset);
                        MapServerMini &nextMap=*static_cast<MapServerMini *>(this->border.bottom.map);
                        if(nextMap.step.size()<2)
                            abort();
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &bottomCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(bottomCodeZone!=0)
                            if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                            {
                                BlockObject &blockObject=blockList.at(codeZone-1);
                                BlockObject &bottomBlockObject=nextMap.blockList.at(bottomCodeZone-1);
                                //if can go to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesBottom)
                                            addBlockLink(blockObject,bottomBlockObject);
                                    }
                                    else
                                        addBlockLink(blockObject,bottomBlockObject);
                                }
                                //if can come from to bottom
                                if(this->parsed_layer.ledges==NULL ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges ||
                                        this->parsed_layer.ledges[x+y*this->width]==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop
                                        )
                                {
                                    if(nextMap.parsed_layer.ledges!=NULL)
                                    {
                                        const CatchChallenger::ParsedLayerLedges &ledge=(CatchChallenger::ParsedLayerLedges)nextMap.parsed_layer.ledges[newx+newy*this->width];
                                        if(ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_NoLedges || ledge==CatchChallenger::ParsedLayerLedges::ParsedLayerLedges_LedgesTop)
                                            addBlockLink(bottomBlockObject,blockObject);
                                    }
                                    else
                                        addBlockLink(bottomBlockObject,blockObject);
                                }
                            }
                    }
                }

                y++;
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
            const uint8_t &codeZone=step2.map[x+y*this->width];
            if(codeZone!=0 && (this->botLayerMask==NULL || this->botLayerMask[x+y*this->width]==0))
            {
                const uint8_t newx=teleporterEntry.destination_x,newy=teleporterEntry.destination_y;
                MapServerMini &nextMap=*static_cast<MapServerMini *>(teleporterEntry.map);
                if(nextMap.step.size()<2)
                    abort();
                MapParsedForBot &step2nextMap=nextMap.step[1];
                const uint8_t &otherCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                if(otherCodeZone!=0)
                    if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                    {
                        BlockObject &blockObject=blockList.at(codeZone-1);
                        BlockObject &bottomBlockObject=nextMap.blockList.at(otherCodeZone-1);
                        addBlockLink(blockObject,bottomBlockObject);
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
    MapParsedForBot &step2=step[1];
    if(step2.map==NULL)
        return false;

    // do the GraphViz content
    {
        step2.graphvizText+="";
        step2.graphvizText+="digraph G {\n";
        step2.graphvizText+="rankdir=LR\n";
        step2.graphvizText+="node [shape=record]\n";

        // the block
        std::unordered_map<const BlockObject *,unsigned int> pointerToIndex;
        {
            unsigned int blockIndex=0;
            while(blockIndex<blockList.size())
            {
                const BlockObject &block=blockList.at(blockIndex);
                step2.graphvizText+="struct"+std::to_string(blockIndex+1)+" [label=\"<f0> "+block.name+"|<f1> "+block.text+"\"]\n";
                pointerToIndex[&block]=blockIndex;
                blockIndex++;
            }
        }
        //the link
        {
            unsigned int blockIndex=0;
            while(blockIndex<blockList.size())
            {
                BlockObject &block=blockList[blockIndex];

                std::unordered_map<BlockObject *,BlockObject::LinkType>::iterator it=block.links.begin();
                while(it!=block.links.cend())
                {
                    const BlockObject * const nextBlock=it->first;
                    if(it->second!=BlockObject::LinkType::BothDirection || &block<=nextBlock || block.map!=nextBlock->map)
                    {
                        if(pointerToIndex.find(nextBlock)!=pointerToIndex.cend())
                            step2.graphvizText+="struct"+std::to_string(blockIndex+1)+" -> struct"+std::to_string(pointerToIndex.at(nextBlock)+1);
                        else
                            step2.graphvizText+="struct"+std::to_string(blockIndex+1)+" -> \""+nextBlock->map->map_file+", block "+std::to_string(nextBlock->id+1)+"\"";
                        switch(it->second)
                        {
                            case BlockObject::LinkType::BothDirection:
                                step2.graphvizText+=" [dir=both];\n";
                            break;
                            default:
                                step2.graphvizText+=";\n";
                            break;
                        }
                    }

                    ++it;
                }

                blockIndex++;
            }
        }

        //step2.graphvizText+="struct2 -> struct3 [dir=both];\n";

        step2.graphvizText+="}";
    }
    return true;
}
