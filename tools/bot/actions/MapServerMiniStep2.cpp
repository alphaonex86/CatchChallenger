#include "MapServerMini.h"
#include <iostream>
#include "../../client/base/interface/DatapackClientLoader.h"

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
            const MapParsedForBot::Layer &layer=step2.layers.at(index);
            BlockObject blockObject;
            blockObject.text=layer.text;
            blockObject.name=layer.name;
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
                        BlockObject &blockObject=blockList.at(codeZone-1);
                        blockObject.borderright=&nextMap;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &rightCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(rightCodeZone!=0)
                        {
                            BlockObject &rightBlockObject=nextMap.blockList.at(rightCodeZone-1);
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
                        BlockObject &blockObject=blockList.at(codeZone-1);
                        blockObject.borderbottom=&nextMap;
                        MapParsedForBot &step2nextMap=nextMap.step[1];
                        const uint8_t &bottomCodeZone=step2nextMap.map[newx+newy*nextMap.width];
                        if(bottomCodeZone!=0)
                        {
                            BlockObject &bottomBlockObject=nextMap.blockList.at(bottomCodeZone-1);
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
                BlockObject &blockObject=blockList.at(codeZone-1);
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
                        BlockObject &otherBlockObject=nextMap.blockList.at(otherCodeZone-1);
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
    MapParsedForBot &step2=step[1];
    if(step2.map==NULL)
        return false;
    MapParsedForBot::Layer &lostLayer=step2.layers[step2.layers.size()-1];


    //teleporter
    {
        int index=0;
        while(index<this->teleporter_list_size)
        {
            const CatchChallenger::CommonMap::Teleporter &teleporter=this->teleporter[index];
            const uint8_t &codeZone=step2.map[teleporter.source_x+teleporter.source_y*this->width];

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

            if(codeZone>0)
            {
                BlockObject &blockObject=blockList.at(codeZone-1);
                blockObject.teleporter_list.push_back(teleporter);

                MapParsedForBot::Layer &layer=step2.layers[codeZone-1];
                layer.contentList.push_back(content);
            }
            else
                lostLayer.contentList.push_back(content);
            index++;
        }
    }
    /*if(ui->comboBox_Layer->currentIndex()==0)
    {
        if(this->border.top.map!=NULL)
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("Top border %1 (offset: %2)")
                          .arg(QString::fromStdString(this->border.top.map->map_file))
                          .arg(this->this->border.top.x_offset)
                          );
            item->setIcon(QIcon(":/7.png"));
            ui->localTargets->addItem(item);
        }
        if(this->border.right.map!=NULL)
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("Right border %1 (offset: %2)")
                          .arg(QString::fromStdString(this->border.right.map->map_file))
                          .arg(this->border.right.y_offset)
                          );
            item->setIcon(QIcon(":/7.png"));
            ui->localTargets->addItem(item);
        }
        if(this->border.bottom.map!=NULL)
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("Botton border %1 (offset: %2)")
                          .arg(QString::fromStdString(this->border.bottom.map->map_file))
                          .arg(this->border.bottom.x_offset)
                          );
            item->setIcon(QIcon(":/7.png"));
            ui->localTargets->addItem(item);
        }
        if(this->border.left.map!=NULL)
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("Left border %1 (offset: %2)")
                          .arg(QString::fromStdString(this->border.left.map->map_file))
                          .arg(this->border.left.y_offset)
                          );
            item->setIcon(QIcon(":/7.png"));
            ui->localTargets->addItem(item);
        }
    }
*/
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
            const uint8_t &codeZone=step2.map[point.first+point.second*this->width];
            const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(itemEntry.item);

            MapParsedForBot::Layer::Content content;
            content.mapId=this->id;
            if(itemEntry.infinite)
                content.text=QString("Item on map %1 (infinite)").arg(itemExtra.name);
            else
                content.text=QString("Item on map %1").arg(itemExtra.name);
            content.icon=QIcon(itemExtra.image);

            if(codeZone>0)
            {
                BlockObject &blockObject=blockList.at(codeZone-1);
                blockObject.pointOnMap_Item.push_back(itemEntry);

                MapParsedForBot::Layer &layer=step2.layers[codeZone-1];
                layer.contentList.push_back(content);
            }
            else
                lostLayer.contentList.push_back(content);
        }
    }
/*        //fight
    {
        for(const auto& n : this->botsFight) {
            const uint8_t &codeZone=step2.map[n.first.first+n.first.second*this->width];
            if((codeZone>0 && (codeZone-1)==ui->comboBox_Layer->currentIndex()) || codeZone==0)
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
                                QListWidgetItem *item=new QListWidgetItem();
                                if(quantity>1)
                                    item->setText(QString("Fight %1: %2x %3")
                                                  .arg(fightId)
                                                  .arg(quantity)
                                                  .arg(itemExtra.name)
                                                  );
                                else
                                    item->setText(QString("Fight %1: %2")
                                                  .arg(fightId)
                                                  .arg(itemExtra.name)
                                                  );
                                item->setIcon(QIcon(itemExtra.image));
                                ui->localTargets->addItem(item);
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
                                QListWidgetItem *item=new QListWidgetItem();
                                item->setText(QString("Fight %1: %2 level %3")
                                              .arg(fightId)
                                              .arg(monsterExtra.name)
                                              .arg(monster.level)
                                              );
                                item->setIcon(QIcon(monsterExtra.thumb));
                                ui->localTargets->addItem(item);
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
            const uint8_t &codeZone=step2.map[n.first.first+n.first.second*this->width];
            if((codeZone>0 && (codeZone-1)==ui->comboBox_Layer->currentIndex()) || codeZone==0)
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
                            QListWidgetItem *item=new QListWidgetItem();
                            item->setText(QString("Shop %1: %2 %3$")
                                          .arg(shopId)
                                          .arg(itemExtra.name)
                                          .arg(price)
                                          );
                            item->setIcon(QIcon(itemExtra.image));
                            ui->localTargets->addItem(item);
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
            const uint8_t &codeZone=step2.map[n.first+n.first*this->width];
            if((codeZone>0 && (codeZone-1)==ui->comboBox_Layer->currentIndex()) || codeZone==0)
            {
                QListWidgetItem *item=new QListWidgetItem();
                item->setText(QString("Heal"));
                item->setIcon(QIcon(":/1.png"));
                ui->localTargets->addItem(item);
            }
        }
    }*/

    //transfer this to lower step
    {
        MapParsedForBot &step1=step[0];
        if(step1.map==NULL)
            return false;
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
    }

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
