#include "MapServerMini.h"
#include <iostream>
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "ActionsAction.h"

std::unordered_set<const MapServerMini *> MapServerMini::getValidMaps(const unsigned int &depth) const
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
            while(indexNearMap<currentMap->linked_map.size())
            {
                const MapServerMini * const nearMap=static_cast<const MapServerMini *>(currentMap->linked_map.at(indexNearMap));
                if(validMaps.find(nearMap)==validMaps.cend())
                    newMapToParse.push_back(nearMap);
                indexNearMap++;
            }
            indexMapToParse++;
        }
        mapToParse=newMapToParse;
        actualDepth++;
    }
    return validMaps;
}

std::unordered_set<const MapServerMini::BlockObject *> MapServerMini::getAccessibleBlock(const std::unordered_set<const MapServerMini *> &validMaps,const BlockObject * const currentNearBlock) const
{
    // only the accessible block
    if(currentNearBlock->map!=this)
        abort();
    std::unordered_set<const BlockObject *> accessibleBlock;
    std::vector<const BlockObject *> blockToParse;
    blockToParse.push_back(currentNearBlock);
    while(!blockToParse.empty())
    {
        std::vector<const BlockObject *> newBlockToParse;
        unsigned int indexBlockToParse=0;
        while(indexBlockToParse<blockToParse.size())
        {
            const BlockObject * const currentBlock=blockToParse.at(indexBlockToParse);
            accessibleBlock.insert(currentBlock);

            for(const auto& n:currentBlock->links) {
                const BlockObject * const nextBlock=n.first;
                if(accessibleBlock.find(nextBlock)==accessibleBlock.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                    newBlockToParse.push_back(nextBlock);
            }
            indexBlockToParse++;
        }
        blockToParse=newBlockToParse;
    }
    return accessibleBlock;
}

void MapServerMini::resolvBlockPath(const BlockObject * blockToExplore,
        std::unordered_map<const BlockObject *,BlockObjectPathFinding> &resolvedBlock,
        const std::unordered_set<const BlockObject *> &accessibleBlock,
        const std::vector<const BlockObject *> &previousBlock) const
{
    if(resolvedBlock.find(blockToExplore)==resolvedBlock.cend())
    {
        BlockObjectPathFinding blockObjectPathFinding;
        blockObjectPathFinding.weight=0;
        //blockObjectPathFinding.bestPath;
        resolvedBlock[blockToExplore]=blockObjectPathFinding;
    }

    std::vector<const BlockObject *> nextBlockList;
    for(const auto& n:blockToExplore->links) {
        const BlockObject * const nextBlock=n.first;
        if(accessibleBlock.find(nextBlock)!=accessibleBlock.cend())
        {
            unsigned int weight=10;
            if(nextBlock->monstersCollisionValue!=NULL)
                if(!nextBlock->monstersCollisionValue->walkOnMonsters.empty())
                    weight+=100;
            if(!nextBlock->botsFight.empty())
                weight+=250;
            //already parsed
            if(resolvedBlock.find(nextBlock)!=resolvedBlock.cend())
            {
                BlockObjectPathFinding &blockObjectPathFinding=resolvedBlock[nextBlock];
                //if the next block weight>weight
                if(blockObjectPathFinding.weight>weight)
                {
                    blockObjectPathFinding.weight=weight;
                    std::vector<const BlockObject *> composedBlock=previousBlock;
                    composedBlock.push_back(nextBlock);
                    blockObjectPathFinding.bestPath=composedBlock;
                    nextBlockList.push_back(nextBlock);
                }
            }
            else
            {
                BlockObjectPathFinding blockObjectPathFinding;
                blockObjectPathFinding.weight=weight;
                std::vector<const BlockObject *> composedBlock=previousBlock;
                composedBlock.push_back(nextBlock);
                blockObjectPathFinding.bestPath=composedBlock;
                nextBlockList.push_back(nextBlock);

                resolvedBlock[nextBlock]=blockObjectPathFinding;
            }
        }
    }
    unsigned int index=0;
    while(index<nextBlockList.size())
    {
        const BlockObject * const nextBlock=nextBlockList.at(index);
        BlockObjectPathFinding &blockObjectPathFinding=resolvedBlock[nextBlock];
        resolvBlockPath(nextBlock,resolvedBlock,accessibleBlock,blockObjectPathFinding.bestPath);
        index++;
    }
}

std::string MapServerMini::graphStepNearMap(const BlockObject * const currentNearBlock,const unsigned int &depth) const
{
    const std::unordered_set<const MapServerMini *> &validMaps=getValidMaps(depth);
    const std::unordered_set<const BlockObject *> &accessibleBlock=getAccessibleBlock(validMaps,currentNearBlock);
    std::unordered_map<const BlockObject *,BlockObjectPathFinding> resolvedBlock;
    resolvBlockPath(currentNearBlock,resolvedBlock,accessibleBlock);

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

        std::string stringLinks;
        for(const auto& n : ActionsAction::actionsAction->map_list) {
            const MapServerMini * const mapServer=static_cast<MapServerMini *>(n.second);
            if(mapServer->step.size()>=2 && validMaps.find(mapServer)!=validMaps.cend())
            {
                const MapParsedForBot &step2=mapServer->step[1];
                if(step2.map!=NULL)
                {
                    unsigned int contentDisplayed=0;
                    std::string subgraph;
                    subgraph+="subgraph cluster_"+std::to_string((uint64_t)mapServer)+" {\n";
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
                        (void)contentEmpty;
                        bool haveValidDestination=false;
                        for(const auto& n:block.links) {
                            const BlockObject * const nextBlock=n.first;
                            if(destinationMaps.find(nextBlock)!=destinationMaps.cend())
                            {
                                haveValidDestination=true;
                                break;
                            }
                        }

                        if(/*layer.name!="Lost layer" || !contentEmpty || */haveValidDestination || destinationMaps.find(&block)!=destinationMaps.cend())
                        {
                            if(accessibleBlock.find(&block)!=accessibleBlock.cend())
                            {
                                subgraph+="struct"+std::to_string((uint64_t)&block)+" [label=\"";
                                //subgraph+="<f0> "+mapServer->map_file+" | "+layer.name+" |";
                                subgraph+="<f0> "+layer.name+" |";
                                subgraph+="<f0> ";
                                unsigned int index=0;
                                while(index<layer.contentList.size())
                                {
                                    const MapParsedForBot::Layer::Content &itemEntry=layer.contentList.at(index);
                                    if(itemEntry.destinationDisplay==MapParsedForBot::Layer::DestinationDisplay::All)
                                        subgraph+=itemEntry.text.toStdString()+"\\n";
                                    index++;
                                }
                                {
                                    const BlockObjectPathFinding &blockObjectPathFinding=resolvedBlock.at(&block);
                                    if(!blockObjectPathFinding.bestPath.empty())
                                    {
                                        subgraph+="|";
                                        unsigned int index=0;
                                        while(index<blockObjectPathFinding.bestPath.size())
                                        {
                                            const BlockObject * const block=blockObjectPathFinding.bestPath.at(index);
                                            subgraph+="Block "+std::to_string(block->id+1)+"("+block->map->map_file+")\\n";
                                            index++;
                                        }
                                    }
                                }
                                subgraph+="\" style=filled fillcolor=\""+block.color.name(QColor::HexRgb).toStdString()+"\"]\n";
                                contentDisplayed++;
                            }
                        }

                        for(const auto& n:block.links) {
                            const BlockObject * const nextBlock=n.first;
                            const BlockObject::LinkInformation &linkInformation=n.second;
                            if(linkInformation.type!=BlockObject::LinkType::BothDirection || &block<=nextBlock)
                            {
                                if(validMaps.find(block.map)!=validMaps.cend() && validMaps.find(nextBlock->map)!=validMaps.cend())
                                    if(accessibleBlock.find(&block)!=accessibleBlock.cend() && accessibleBlock.find(nextBlock)!=accessibleBlock.cend())
                                    {
                                        contentDisplayed++;
                                        stringLinks+="struct"+std::to_string((uint64_t)&block)+" -> struct"+std::to_string((uint64_t)nextBlock);
                                        switch(linkInformation.type)
                                        {
                                            case BlockObject::LinkType::BothDirection:
                                                stringLinks+=" [dir=both];\n";
                                            break;
                                            default:
                                                stringLinks+=";\n";
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

        overall_graphvizText+=stringLinks;
        overall_graphvizText+="}";
        return overall_graphvizText;
    }
    return "";
}
