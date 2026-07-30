#include "MapServerMini.h"

#include <iostream>
#include "../BotAbort.h"

bool MapServerMini::preload_step1()
{
    if(this->width==0 || this->height==0)
        return false;
    if(this->flat_simplified_map.size()!=(size_t)this->width*this->height)
        std::cerr << "WARNING: map " << this->map_file << " flat_simplified_map.size()=" << this->flat_simplified_map.size() << " != width*height=" << (size_t)this->width*this->height << " (w=" << std::to_string(this->width) << " h=" << std::to_string(this->height) << ")" << std::endl;
    std::unordered_map<std::string,int> zoneHash;
    std::vector<std::string> layerList;
    zoneHash.clear();
    zoneHash[std::string()]=0;
    MapParsedForBot step1;
    {
        step1.map=(uint16_t *)malloc(width*height*sizeof(uint16_t));
        memset(step1.map,0x00,width*height*sizeof(uint16_t));//by default: not accessible zone
        int y=0;
        while(y<this->height)
        {
            int x=0;
            while(x<this->width)
            {
                const std::pair<uint8_t,uint8_t> p(x,y);
                std::string zone;
                bool walkable=false;
                if(this->flat_simplified_map.size()==(size_t)this->width*this->height)
                {
                    const uint8_t &var=this->flat_simplified_map[x+y*this->width];
                    if(var==0)
                        walkable=true;
                    if(pointOnMap_Item.find(p)!=pointOnMap_Item.cend())
                    {
                        const MapServerMini::ItemOnMap &itemOnMap=pointOnMap_Item.at(p);
                        if((itemOnMap.infinite && itemOnMap.visible) || (itemOnMap.visible && !walkable))
                            zone+="itemonmap"+std::to_string(x)+","+std::to_string(y);
                        //walkable=true;
                    }
                    if(walkable)
                        zone+="w";
                    if(var>0 && var<200)
                        zone+="m"+std::to_string(var);
                    if(var==249)
                        zone+="d";
                    if(var)
                        zone+="l"+std::to_string(var-250);
                }

                if(botOnMap.find(p)!=botOnMap.cend())
                {
                    const std::vector<uint16_t> &botsList=botOnMap.at(p);
                    unsigned int index=0;
                    while(index<botsList.size())
                    {
                        zone+="bot"+std::to_string(botsList.at(index));
                        index++;
                    }
                }
                if(zoneHash.find(zone)==zoneHash.cend())
                {
                    int size=(int)zoneHash.size();
                    zoneHash[zone]=size;
                }
                //color
                int codeZone=zoneHash[zone];
                step1.map[x+y*this->width]=codeZone;

                x++;
            }
            y++;
        }
    }
    /*std::cout << this->map_file << std::endl;
    displayConsoleMap(step1);*/
    {
        layerList.clear();
        //drop the "not accessible" zone (the empty key holds code 0), then keep
        //one slot per remaining zone plus the unused index 0
        zoneHash.erase(std::string());
        while(layerList.size()<(zoneHash.size()+1))
            layerList.push_back(std::string());

        //the hash order does not matter: each entry is written at its own code
        std::unordered_map<std::string,int>::const_iterator i=zoneHash.cbegin();
        while(i!=zoneHash.cend()) {
            layerList[i->second]=i->first;
            ++i;
        }

        size_t index=1;
        while(index<layerList.size())
        {
            MapParsedForBot::Layer layer;
            layer.name="Layer "+std::to_string(index);
            layer.text=layerList.at(index);
            step1.layers.push_back(layer);
            index++;
        }
    }

    min_x=0;
    {
        int x=0;
        while(x<this->width)
        {
            int y=0;
            while(y<this->height)
            {
                if(step1.map[x+y*this->width]!=0)
                    break;
                y++;
            }
            if(y<this->height)
                break;
            x++;
            min_x=x;
        }
    }
    max_x=this->width;
    {
        int x=this->width-1;
        while(x>=0)
        {
            int y=0;
            while(y<this->height)
            {
                if(step1.map[x+y*this->width]!=0)
                    break;
                y++;
            }
            if(y<this->height)
                break;
            max_x=x;
            x--;
        }
    }
    min_y=0;
    {
        int y=0;
        while(y<this->height)
        {
            int x=0;
            while(x<this->width)
            {
                if(step1.map[x+y*this->width]!=0)
                    break;
                x++;
            }
            if(x<this->width)
                break;
            y++;
            min_y=y;
        }
    }
    max_y=this->height;
    {
        int y=this->height-1;
        while(y>=0)
        {
            int x=0;
            while(x<this->width)
            {
                if(step1.map[x+y*this->width]!=0)
                    break;
                x++;
            }
            if(x<this->width)
                break;
            max_y=y;
            y--;
        }
    }
    {
        MapParsedForBot::Layer layer;
        layer.name="Lost layer";
        layer.text="";
        step1.layers.push_back(layer);
    }
    //create the object
    {
        unsigned int index=0;
        while(index<step1.layers.size())
        {
            MapParsedForBot::Layer &layer=step1.layers[index];
            layer.blockObject=new BlockObject();
            BlockObject blockObject;
            blockObject.map=this;
            blockObject.id=index;

            blockObject.monstersCollisionValue=NULL;
            blockObject.color=MapServerMini::colorsList.at(index%MapServerMini::colorsList.size());
            blockObject.layer=NULL;
            *(layer.blockObject)=blockObject;
            if(layer.blockObject==NULL)
                BOT_ABORT();
            index++;
        }
    }
    //control the layer 1
    {
        unsigned int index=0;
        while(index<step1.layers.size())
        {
            const MapParsedForBot::Layer &layer=step1.layers.at(index);
            if(layer.blockObject==NULL)
                BOT_ABORT();
            index++;
        }
    }
    step.push_back(step1);
    //control the layer 1
    {
        unsigned int index=0;
        while(index<step1.layers.size())
        {
            const MapParsedForBot::Layer &layer=step1.layers.at(index);
            if(layer.blockObject==NULL)
                BOT_ABORT();
            index++;
        }
    }
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
