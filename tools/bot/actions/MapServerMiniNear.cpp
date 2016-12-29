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

void MapServerMini::targetBlockList(const BlockObject * const currentNearBlock,std::unordered_map<const BlockObject *,BlockObjectPathFinding> &resolvedBlock,const unsigned int &depth) const
{
    const std::unordered_set<const MapServerMini *> &validMaps=getValidMaps(depth);
    const std::unordered_set<const BlockObject *> &accessibleBlock=getAccessibleBlock(validMaps,currentNearBlock);
    resolvBlockPath(currentNearBlock,resolvedBlock,accessibleBlock);
}
