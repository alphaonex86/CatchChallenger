#include "MapServerMini.h"

#include <iostream>

bool MapServerMini::preload_step1()
{
    if(this->width==0 || this->height==0)
        return false;
    QHash<QString,int> zoneHash;
    QList<QString> layerList;
    zoneHash.clear();
    zoneHash[""]=0;
    MapParsedForBot step1;
    {
        step1.map=(uint8_t *)malloc(width*height);
        memset(step1.map,0x00,width*height);//by default: not accessible zone
        int y=0;
        while(y<this->height)
        {
            int x=0;
            while(x<this->width)
            {
                QString zone;
                if(this->parsed_layer.walkable!=NULL)
                    if(this->parsed_layer.walkable[x+y*this->width])
                        zone+="w";
                if(this->parsed_layer.monstersCollisionMap!=NULL)
                    if(this->parsed_layer.monstersCollisionMap[x+y*this->width])
                        zone+="m"+QString::number(this->parsed_layer.monstersCollisionMap[x+y*this->width]);
                if(this->parsed_layer.dirt!=NULL)
                    if(this->parsed_layer.dirt[x+y*this->width])
                        zone+="d";
                if(this->parsed_layer.ledges!=NULL)
                    if(this->parsed_layer.ledges[x+y*this->width])
                        zone+="l"+QString::number(this->parsed_layer.ledges[x+y*this->width]);
                if(!zoneHash.contains(zone))
                {
                    int size=zoneHash.size();
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
        zoneHash.remove(0);
        while(layerList.size()<(zoneHash.size()+1))
            layerList << "";

        QHash<QString, int>::const_iterator i = zoneHash.constBegin();
        while (i != zoneHash.constEnd()) {
            layerList[i.value()]=i.key();
            ++i;
        }

        int index=1;
        while(index<layerList.size())
        {
            MapParsedForBot::Layer layer;
            layer.name="Layer "+std::to_string(index);
            layer.text=layerList.at(index).toStdString();
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
                abort();
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
                abort();
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
                abort();
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
