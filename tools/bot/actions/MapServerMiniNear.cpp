#include "MapServerMini.h"
#include <iostream>
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "ActionsAction.h"

std::string MapServerMini::graphStepNearMap(const unsigned int &depth)
{
    // do the map list
    unsigned int actualDepth=0;
    std::unordered_set<const MapServerMini *> validMaps;
    std::vector<const MapServerMini *> mapToParse;
    mapToParse.push_back(this);
    while(actualDepth<depth)
    {
        std::vector<const MapServerMini *> newMapToParse;
        unsigned int indexMapToParse=0;
        while(indexMapToParse<mapToParse.size())
        {
            const MapServerMini * const currentMap=mapToParse.at(indexMapToParse);
            validMaps.insert(currentMap);
            unsigned int indexNearMap=0;
            while(indexNearMap<currentMap->near_map.size())
            {
                const MapServerMini * const nearMap=static_cast<const MapServerMini *>(currentMap->near_map.at(indexNearMap));
                if(validMaps.find(nearMap)==validMaps.cend())
                    newMapToParse.push_back(nearMap);
                indexNearMap++;
            }
            indexMapToParse++;
        }
        mapToParse=newMapToParse;
        actualDepth++;
    }
    // do the GraphViz content
    {
        std::string overall_graphvizText="";
        overall_graphvizText+="digraph G {\n";
        overall_graphvizText+="rankdir=LR\n";
        overall_graphvizText+="node [shape=record]\n";

        std::unordered_set<const BlockObject *> destinationMaps;
        for(const auto& n : ActionsAction::actionsAction->map_list) {
            const MapServerMini * const mapServer=static_cast<MapServerMini *>(n.second);
            if(mapServer->step.size()>=2)
            {
                const MapParsedForBot &step2=mapServer->step[1];
                if(step2.map!=NULL)
                {
                    unsigned int indexLayer=0;
                    while(indexLayer<step2.layers.size())
                    {
                        const MapParsedForBot::Layer &layer=step2.layers.at(indexLayer);
                        const BlockObject &block=*layer.blockObject;
                        for(const auto& n:block.links) {
                            const BlockObject * const nextBlock=n.first;
                            if(validMaps.find(mapServer)!=validMaps.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                                destinationMaps.insert(nextBlock);
                        }
                        indexLayer++;
                    }
                }
            }
        }

        for(const auto& n : ActionsAction::actionsAction->map_list) {
            const MapServerMini * const mapServer=static_cast<MapServerMini *>(n.second);
            if(mapServer->step.size()>=2 && validMaps.find(mapServer)!=validMaps.cend())
            {
                const MapParsedForBot &step2=mapServer->step[1];
                if(step2.map!=NULL)
                {
                    unsigned int contentDisplayed=0;
                    std::string subgraph;
                    subgraph+="subgraph "+std::to_string((uint64_t)mapServer)+" {\n";
                    subgraph+="label=\""+mapServer->map_file+"\";\n";
                    unsigned int indexLayer=0;
                    while(indexLayer<step2.layers.size())
                    {
                        const MapParsedForBot::Layer &layer=step2.layers.at(indexLayer);
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
                        bool haveValidDestination=false;
                        for(const auto& n:block.links) {
                            const BlockObject * const nextBlock=n.first;
                            if(destinationMaps.find(nextBlock)!=destinationMaps.cend())
                            {
                                haveValidDestination=true;
                                break;
                            }
                        }

                        if(/*layer.name!="Lost layer" || */!contentEmpty || haveValidDestination || destinationMaps.find(&block)!=destinationMaps.cend())
                        {
                            subgraph+="struct"+std::to_string((uint64_t)&block)+" [label=\"";
                            subgraph+="<f0> "+mapServer->map_file+" | "+layer.name+" |";
                            unsigned int index=0;
                            while(index<layer.contentList.size())
                            {
                                const MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                                if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                                    subgraph+=itemEntry.text.toStdString()+"\\n";
                                index++;
                            }
                            subgraph+="\" style=filled fillcolor=\""+block.color.name(QColor::HexRgb).toStdString()+"\"]\n";
                            contentDisplayed++;
                        }

                        for(const auto& n:block.links) {
                            const BlockObject * const nextBlock=n.first;
                            const BlockObject::LinkInformation &linkInformation=n.second;
                            if(linkInformation.type!=BlockObject::LinkType::BothDirection || &block<=nextBlock)
                            {
                                if(validMaps.find(block.map)!=validMaps.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                                {
                                    contentDisplayed++;
                                    subgraph+="struct"+std::to_string((uint64_t)&block)+" -> struct"+std::to_string((uint64_t)nextBlock);
                                    switch(linkInformation.type)
                                    {
                                        case BlockObject::LinkType::BothDirection:
                                            subgraph+=" [dir=both];\n";
                                        break;
                                        default:
                                            subgraph+=";\n";
                                        break;
                                    }
                                }
                            }
                        }

                        indexLayer++;
                    }
                    subgraph+="}\n";
                    if(contentDisplayed>0)
                        overall_graphvizText+=subgraph;
                }
            }
        }

        overall_graphvizText+="}";
        return overall_graphvizText;
    }
    return "";
}
