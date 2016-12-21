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

    /*digraph G {
rankdir=LR
node [shape=record]
struct2 [label="<f0> one|<f1> two"]
struct3 [label="<f0> A|<f1> B"]
struct2 -> struct3;
struct2 -> struct3 [dir=both];
}*/

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
