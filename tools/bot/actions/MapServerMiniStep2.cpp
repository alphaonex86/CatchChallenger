#include "MapServerMini.h"
#include <iostream>
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"

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
                    layer.haveMonsterSet=false;
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
            layer.haveMonsterSet=false;
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

            blockObject.learn=false;
            blockObject.heal=false;
            blockObject.market=false;
            blockObject.zonecapture=false;
            blockObject.bordertop=NULL;
            blockObject.borderright=NULL;
            blockObject.borderbottom=NULL;
            blockObject.borderleft=NULL;
            blockObject.monstersCollisionValue=NULL;
            *layer.blockObject=blockObject;
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
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                BlockObject &rightBlockObject=*step2.layers[rightCodeZone-1].blockObject;
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
                                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                                BlockObject &bottomBlockObject=*step2.layers[bottomCodeZone-1].blockObject;
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
                        BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                        blockObject.borderright=&nextMap;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &rightCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(rightCodeZone!=0)
                        {
                            BlockObject &rightBlockObject=*step2nextMap.layers[rightCodeZone-1].blockObject;
                            rightBlockObject.borderleft=this;
                            if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                            {
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
                    }
                    //check the bottom tile
                    if(this->border.bottom.map!=NULL)
                    {
                        uint16_t newx=0,newy=(y+this->border.bottom.x_offset);
                        MapServerMini &nextMap=*static_cast<MapServerMini *>(this->border.bottom.map);
                        if(nextMap.step.size()<2)
                            abort();
                        BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                        blockObject.borderbottom=&nextMap;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &bottomCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(bottomCodeZone!=0)
                        {
                            BlockObject &bottomBlockObject=*step2nextMap.layers[bottomCodeZone-1].blockObject;
                            bottomBlockObject.bordertop=this;
                            if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                            {
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
                BlockObject &blockObject=*step2.layers[codeZone-1].blockObject;
                blockObject.teleporter_list.push_back(teleporter[index]);
                const uint8_t newx=teleporterEntry.destination_x,newy=teleporterEntry.destination_y;
                MapServerMini &nextMap=*static_cast<MapServerMini *>(teleporterEntry.map);
                if(nextMap.step.size()<2)
                    abort();
                MapParsedForBot &step2nextMap=nextMap.step[1];
                const uint8_t &otherCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                if(otherCodeZone!=0)
                    if(nextMap.botLayerMask==NULL || nextMap.botLayerMask[newx+newy*nextMap.width]==0)
                    {
                        BlockObject &otherBlockObject=*step2nextMap.layers[otherCodeZone-1].blockObject;
                        addBlockLink(blockObject,otherBlockObject);
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
            MapParsedForBot::Layer &lostLayer=currentStep.layers[currentStep.layers.size()-1];
            if(currentStep.map!=NULL)
            {
                if(!mapIsValid(currentStep))
                {
                    displayConsoleMap(currentStep);
                    abort();
                }
                //teleporter
                {
                    int index=0;
                    while(index<this->teleporter_list_size)
                    {
                        const CatchChallenger::CommonMap::Teleporter &teleporter=this->teleporter[index];
                        const uint8_t &codeZone=currentStep.map[teleporter.source_x+teleporter.source_y*this->width];

                        MapParsedForBot::Layer::Content content;
                        content.mapId=teleporter.map->id;
                        content.text=QString("From (%1,%2) to %3 (%4,%5)")
                                .arg(teleporter.source_x)
                                .arg(teleporter.source_y)
                                .arg(QString::fromStdString(teleporter.map->map_file))
                                .arg(teleporter.destination_x)
                                .arg(teleporter.destination_y)
                                ;
                        content.icon=QIcon(":/7.png");
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::OnlyGui;

                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();

                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.teleporter_list.push_back(teleporter);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                        index++;
                    }
                }
                //border
                {
                    if(this->border.top.map!=NULL)
                    {
                        MapParsedForBot::Layer::Content content;
                        content.mapId=border.top.map->id;
                        content.text=QString("Top border %1 (offset: %2)")
                                .arg(QString::fromStdString(this->border.top.map->map_file))
                                .arg(this->border.top.x_offset)
                                ;
                        content.icon=QIcon(":/7.png");
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::OnlyGui;

                        uint8_t codeZone=0;
                        uint8_t x=0;
                        while(x<this->width)
                        {
                            const uint8_t &codeZoneTemp=currentStep.map[x+0*this->width];
                            if(codeZoneTemp>0)
                            {
                                codeZone=codeZoneTemp;
                                break;
                            }
                            x++;
                        }
                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();
                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.bordertop=static_cast<MapServerMini *>(border.top.map);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                    }
                    if(this->border.right.map!=NULL)
                    {
                        MapParsedForBot::Layer::Content content;
                        content.mapId=border.right.map->id;
                        content.text=QString("Right border %1 (offset: %2)")
                                .arg(QString::fromStdString(this->border.right.map->map_file))
                                .arg(this->border.right.y_offset)
                                ;
                        content.icon=QIcon(":/7.png");
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::OnlyGui;

                        uint8_t codeZone=0;
                        uint8_t y=0;
                        while(y<this->height)
                        {
                            const uint8_t &codeZoneTemp=currentStep.map[(this->width-1)+y*this->width];
                            if(codeZoneTemp>0)
                            {
                                codeZone=codeZoneTemp;
                                break;
                            }
                            y++;
                        }
                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();
                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.borderright=static_cast<MapServerMini *>(border.right.map);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                    }
                    if(this->border.bottom.map!=NULL)
                    {
                        MapParsedForBot::Layer::Content content;
                        content.mapId=border.bottom.map->id;
                        content.text=QString("Bottom border %1 (offset: %2)")
                                .arg(QString::fromStdString(this->border.bottom.map->map_file))
                                .arg(this->border.bottom.x_offset)
                                ;
                        content.icon=QIcon(":/7.png");
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::OnlyGui;

                        uint8_t codeZone=0;
                        uint8_t x=0;
                        while(x<this->width)
                        {
                            const uint8_t &codeZoneTemp=currentStep.map[x+(this->height-1)*this->width];
                            if(codeZoneTemp>0)
                            {
                                codeZone=codeZoneTemp;
                                break;
                            }
                            x++;
                        }
                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();
                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.borderbottom=static_cast<MapServerMini *>(border.bottom.map);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                    }
                    if(this->border.left.map!=NULL)
                    {
                        MapParsedForBot::Layer::Content content;
                        content.mapId=border.left.map->id;
                        content.text=QString("Left border %1 (offset: %2)")
                                .arg(QString::fromStdString(this->border.left.map->map_file))
                                .arg(this->border.left.y_offset)
                                ;
                        content.icon=QIcon(":/7.png");
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::OnlyGui;

                        uint8_t codeZone=0;
                        uint8_t y=0;
                        while(y<this->height)
                        {
                            const uint8_t &codeZoneTemp=currentStep.map[0+y*this->width];
                            if(codeZoneTemp>0)
                            {
                                codeZone=codeZoneTemp;
                                break;
                            }
                            y++;
                        }
                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();
                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.borderleft=static_cast<MapServerMini *>(border.left.map);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                    }
                }
                //not clickable item
                /*
                std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
                std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
                std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;

                //insdustry -> skip, no position control on server side
            wild monster (and their object, day cycle)
            */
                //item on map
                {
                    for (auto it = this->pointOnMap_Item.begin(); it != this->pointOnMap_Item.cend(); ++it) {
                        const std::pair<uint8_t,uint8_t> &point=it->first;
                        const MapServerMini::ItemOnMap &itemEntry=it->second;
                        const uint8_t &codeZone=currentStep.map[point.first+point.second*this->width];
                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(itemEntry.item);

                        MapParsedForBot::Layer::Content content;
                        content.mapId=this->id;
                        if(itemEntry.infinite)
                            content.text=QString("Item on map %1 (infinite)").arg(itemExtra.name);
                        else
                            content.text=QString("Item on map %1").arg(itemExtra.name);
                        content.icon=QIcon(itemExtra.image);
                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                        if(codeZone>0)
                        {
                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                abort();
                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                            blockObject.pointOnMap_Item.push_back(itemEntry);

                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                            layer.contentList.push_back(content);
                        }
                        else
                            lostLayer.contentList.push_back(content);
                    }
                }
                //fight
                {
                    for(const auto& n : this->botsFight) {
                        const uint8_t x=n.first.first,y=n.first.second;
                        const uint8_t &codeZone=currentStep.map[x+y*this->width];
                        {
                            unsigned int index=0;
                            const std::vector<uint32_t> &fightsList=n.second;
                            while(index<fightsList.size())
                            {
                                const uint32_t &fightId=fightsList.at(index);
                                const CatchChallenger::BotFight &fight=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId);

                                //item
                                {
                                    unsigned int sub_index=0;
                                    while(sub_index<fight.items.size())
                                    {
                                        const CatchChallenger::BotFight::Item &item=fight.items.at(sub_index);
                                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(item.id);
                                        const uint32_t &quantity=item.quantity;
                                        {
                                            MapParsedForBot::Layer::Content content;
                                            content.mapId=this->id;
                                            if(quantity>1)
                                                content.text=QString("Fight %1: %2x %3")
                                                              .arg(fightId)
                                                              .arg(quantity)
                                                              .arg(itemExtra.name)
                                                              ;
                                            else
                                                content.text=QString("Fight %1: %2")
                                                              .arg(fightId)
                                                              .arg(itemExtra.name)
                                                              ;
                                            content.icon=QIcon(itemExtra.image);
                                            content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                                            if(codeZone>0)
                                            {
                                                if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                                    abort();
                                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                                blockObject.borderleft=static_cast<MapServerMini *>(border.left.map);

                                                MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                                                layer.contentList.push_back(content);
                                            }
                                            else
                                                lostLayer.contentList.push_back(content);
                                        }
                                        sub_index++;
                                    }
                                }
                                //monster
                                {
                                    unsigned int sub_index=0;
                                    while(sub_index<fight.monsters.size())
                                    {
                                        const CatchChallenger::BotFight::BotFightMonster &monster=fight.monsters.at(sub_index);
                                        const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(monster.id);
                                        {
                                            MapParsedForBot::Layer::Content content;
                                            content.mapId=this->id;
                                            content.text=QString("Fight %1: %2 level %3")
                                                    .arg(fightId)
                                                    .arg(monsterExtra.name)
                                                    .arg(monster.level)
                                                    ;
                                            content.icon=QIcon(monsterExtra.thumb);
                                            content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                                            if(codeZone>0)
                                            {
                                                if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                                    abort();
                                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                                blockObject.borderleft=static_cast<MapServerMini *>(border.left.map);

                                                MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                                                layer.contentList.push_back(content);
                                            }
                                            else
                                                lostLayer.contentList.push_back(content);
                                        }
                                        sub_index++;
                                    }
                                }

                                index++;
                            }
                        }
                    }
                }
                //shop
                {
                    for(const auto& n : this->shops) {
                        const uint8_t x=n.first.first,y=n.first.second;
                        const uint8_t &codeZone=currentStep.map[x+y*this->width];
                        {
                            unsigned int index=0;
                            const std::vector<uint32_t> &shopList=n.second;
                            while(index<shopList.size())
                            {
                                const uint32_t &shopId=shopList.at(index);
                                const CatchChallenger::Shop &shop=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId);

                                unsigned int sub_index=0;
                                while(sub_index<shop.prices.size())
                                {
                                    const CATCHCHALLENGER_TYPE_ITEM &item=shop.items.at(sub_index);
                                    const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(item);
                                    const uint32_t &price=shop.prices.at(sub_index);
                                    {
                                        MapParsedForBot::Layer::Content content;
                                        content.mapId=this->id;
                                        content.text=QString("Shop %1: %2 %3$")
                                                .arg(shopId)
                                                .arg(itemExtra.name)
                                                .arg(price);
                                        content.icon=QIcon(itemExtra.image);
                                        content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                                        if(codeZone>0)
                                        {
                                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                                abort();
                                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                            blockObject.borderleft=static_cast<MapServerMini *>(border.left.map);

                                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                                            layer.contentList.push_back(content);
                                        }
                                        else
                                            lostLayer.contentList.push_back(content);
                                    }
                                    sub_index++;
                                }

                                index++;
                            }
                        }
                    }
                }
                //heal
                {
                    for(const auto& n : this->heal) {
                        if(currentStep.map==NULL)
                            abort();
                        const uint8_t x=n.first,y=n.second;
                        const uint8_t &codeZone=currentStep.map[x+y*this->width];
                        {
                            MapParsedForBot::Layer::Content content;
                            content.mapId=this->id;
                            content.text="Heal";
                            content.icon=QIcon(":/1.png");
                            content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                            if(codeZone>0)
                            {
                                if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                {
                                    displayConsoleMap(currentStep);
                                    abort();
                                }
                                BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                blockObject.borderleft=static_cast<MapServerMini *>(border.left.map);

                                MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                                layer.contentList.push_back(content);
                            }
                            else
                                lostLayer.contentList.push_back(content);
                        }
                    }
                }

                //detect the block
                const CatchChallenger::ParsedLayer &parsedLayer=this->parsed_layer;
                if(parsedLayer.monstersCollisionMap!=NULL)
                {
                    std::unordered_set<uint8_t> lostMonster;
                    int y=0;
                    while(y<this->height)
                    {
                        int x=0;
                        while(x<this->width)
                        {
                            if(currentStep.map[x+y*this->width]!=0)
                            {
                                const uint8_t &codeZone=currentStep.map[x+y*this->width];
                                const uint8_t &monsterCode=parsedLayer.monstersCollisionMap[x+y*this->width];
                                if(monsterCode<parsedLayer.monstersCollisionList.size())
                                {
                                    std::vector<MapParsedForBot::Layer::Content> contentList;
                                    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=parsedLayer.monstersCollisionList.at(monsterCode);
                                    if(!monstersCollisionValue.walkOnMonsters.empty())
                                    {
                                        /// \todo do the real code
                                        //choice the first entry
                                        const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=monstersCollisionValue.walkOnMonsters.at(0);

                                        //monster
                                        {
                                            if(monsterCode>=parsedLayer.monstersCollisionList.size())
                                                abort();
                                            unsigned int sub_index=0;
                                            while(sub_index<monsterCollisionContent.defaultMonsters.size())
                                            {
                                                const CatchChallenger::MapMonster &mapMonster=monsterCollisionContent.defaultMonsters.at(sub_index);
                                                const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(mapMonster.id);
                                                {
                                                    MapParsedForBot::Layer::Content content;
                                                    content.mapId=this->id;
                                                    content.text=QString("Wild %2 level %3-%4, luck: %5")
                                                            .arg(monsterExtra.name)
                                                            .arg(mapMonster.minLevel)
                                                            .arg(mapMonster.maxLevel)
                                                            .arg(QString::number(mapMonster.luck)+"%")
                                                            ;
                                                    content.icon=QIcon(monsterExtra.thumb);
                                                    content.destinationDisplay=MapParsedForBot::Layer::DestinationDisplay::All;

                                                    contentList.push_back(content);
                                                }
                                                sub_index++;
                                            }
                                        }

                                        if(codeZone>0)
                                        {
                                            if((uint8_t)(codeZone-1)>=currentStep.layers.size())
                                                abort();
                                            BlockObject &blockObject=*currentStep.layers[codeZone-1].blockObject;
                                            if(blockObject.monstersCollisionValue==NULL)
                                                blockObject.monstersCollisionValue=&parsedLayer.monstersCollisionList[monsterCode];

                                            MapParsedForBot::Layer &layer=currentStep.layers[codeZone-1];
                                            if(!layer.haveMonsterSet)
                                            {
                                                layer.contentList.insert(layer.contentList.cend(),contentList.cbegin(),contentList.cend());
                                                layer.haveMonsterSet=true;
                                            }
                                        }
                                        else
                                        {
                                            if(lostMonster.find(monsterCode)!=lostMonster.cend())
                                            {
                                                lostMonster.insert(monsterCode);
                                                lostLayer.contentList.insert(lostLayer.contentList.cend(),contentList.cbegin(),contentList.cend());
                                            }
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

    /*//transfer this to lower step
    {
        if(!step1.layers.empty())
        {
            std::unordered_map<uint32_t,uint32_t> step1to2;
            //map the x,y zone
            int y=0;
            while(y<this->height)
            {
                int x=0;
                while(x<this->width)
                {
                    const uint8_t codeZone2=step2.map[x+y*this->width];
                    if(codeZone2!=0)
                        if(step1to2.find(codeZone2-1)==step1to2.cend())
                        {
                            const uint8_t codeZone1=step1.map[x+y*this->width];
                            if(codeZone1!=0)
                                step1to2[codeZone2-1]=codeZone1-1;
                            else
                                step1to2[codeZone2-1]=step1.layers.size()-1;
                        }
                    x++;
                }
                y++;
            }
            //link the missing zone
            unsigned int index=0;
            while(index<step2.layers.size())
            {
                if(step1to2.find(index)==step1to2.cend())
                    step1to2[index]=step1.layers.size()-1;
                index++;
            }
            //transfer the item
            index=0;
            while(index<step2.layers.size())
            {
                MapParsedForBot::Layer &layerstep1=step1.layers[step1to2.at(index)];
                MapParsedForBot::Layer &layerstep2=step2.layers[index];
                layerstep1.contentList.insert(layerstep1.contentList.end(), layerstep2.contentList.begin(), layerstep2.contentList.end());
                index++;
            }
        }
    }*/

    return true;
}

bool MapServerMini::preload_step2z()
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
            while(blockIndex<step2.layers.size())
            {
                const MapParsedForBot::Layer &layer=step2.layers.at(blockIndex);
                const BlockObject &block=*layer.blockObject;
                unsigned int index=0;
                bool contentEmpty=true;
                while(index<layer.contentList.size())
                {
                    const MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                    if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                    {
                        contentEmpty=false;
                        break;
                    }
                    index++;
                }

                if(layer.name!="Lost layer" || !contentEmpty)
                {
                    step2.graphvizText+="struct"+std::to_string(blockIndex+1)+" [label=\"";
                    step2.graphvizText+="<f0> "+layer.name;
                    //step2.graphvizText+="<f1> "+layer.text;

                    if(!contentEmpty)
                    {
                        step2.graphvizText+="|<f1> ";
                        unsigned int index=0;
                        while(index<layer.contentList.size())
                        {
                            const MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                            if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                                step2.graphvizText+=itemEntry.text.toStdString()+"\\n";
                            index++;
                        }
                    }
                    step2.graphvizText+="\"]\n";
                }
                pointerToIndex[&block]=blockIndex;
                blockIndex++;
            }
        }
        //the link
        {
            unsigned int blockIndex=0;
            while(blockIndex<step2.layers.size())
            {
                BlockObject &block=*step2.layers[blockIndex].blockObject;

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

void MapServerMini::displayConsoleMap(const MapParsedForBot &currentStep)
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
            uint8_t codeZone=currentStep.map[x+y*this->width];
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

bool MapServerMini::mapIsValid(const MapParsedForBot &currentStep)
{
    if(currentStep.map==NULL)
        return false;
    int y=0;
    while(y<(this->height-1))
    {
        int x=0;
        while(x<(this->width-1))
        {
            const uint8_t codeZone=currentStep.map[x+y*this->width];
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
