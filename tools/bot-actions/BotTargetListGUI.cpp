#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"
#include <chrono>
#include <math.h>
#include <iostream>

std::vector<std::string> BotTargetList::contentToGUI(const MapServerMini::BlockObject * const blockObject, const CatchChallenger::Api_protocol * const api, QListWidget *listGUI)
{
    std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> resolvedBlockList;
    MapServerMini::BlockObjectPathFinding pathFinding;
    pathFinding.weight=0;
    resolvedBlockList[blockObject]=pathFinding;
    ActionsBotInterface::GlobalTarget bestTarget;
    return contentToGUI_internal(api,listGUI,resolvedBlockList,true,true,true,true,true,true,true,bestTarget,NULL,0,0);
}

uint16_t BotTargetList::mapPointDistanceNormalised(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2)
{
    uint16_t squaredistancex=0,squaredistancey=0;
    if(x1>x2)
        squaredistancex=x1-x2;
    else
        squaredistancex=x2-x1;
    if(y1>y2)
        squaredistancey=y1-y2;
    else
        squaredistancey=y2-y1;
    if((squaredistancex==1 && squaredistancey==0) || (squaredistancex==0 && squaredistancey==1))
        return 1;
    if(squaredistancex<2 && squaredistancey<2)
        return 2;
    return 2+log(squaredistancex+squaredistancey);
}

uint32_t BotTargetList::getSeedToPlant(const CatchChallenger::Api_protocol * api,bool *haveSeedToPlant)
{
    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations_ro();
    auto i=player_private_and_public_informations.items.begin();
    bool haveQuantity=false;
    uint32_t quantitySelected=0;
    uint32_t itemSelected=0;
    while (i!=player_private_and_public_informations.items.cend())
    {
        const uint16_t &itemId=i->first;
        const uint32_t &quantity=i->second;
        if(DatapackClientLoader::datapackLoader.itemToPlants.find(itemId)!=DatapackClientLoader::datapackLoader.itemToPlants.cend())
        {
            /// \todo check the requirement
            const uint8_t &plantId=DatapackClientLoader::datapackLoader.itemToPlants.at(itemId);

            if(ActionsAction::haveReputationRequirements(api,CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
            {
                if(!haveQuantity)
                {
                    haveQuantity=true;
                    quantitySelected=quantity;
                    itemSelected=itemId;
                }
                else if(quantity<quantitySelected)
                {
                    quantitySelected=quantity;
                    itemSelected=itemId;
                }
                if(haveSeedToPlant!=NULL)
                    *haveSeedToPlant=true;
            }
        }
        ++i;
    }
    if(haveQuantity)
    {
        if(haveSeedToPlant!=NULL)
            *haveSeedToPlant=true;
        return itemSelected;
    }
    if(haveSeedToPlant!=NULL)
        *haveSeedToPlant=false;
    return 0;
}

std::vector<std::string> BotTargetList::contentToGUI(const CatchChallenger::Api_protocol * const api, QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard,
                                                     bool dirt, bool itemOnMap, bool fight, bool shop, bool heal, bool wildMonster)
{
    ActionsBotInterface::GlobalTarget bestTarget;
    return contentToGUI_internal(api,listGUI,resolvedBlockList,displayTooHard,dirt,itemOnMap,fight,shop,heal,wildMonster,bestTarget,NULL,0,0);
}

std::vector<std::string> BotTargetList::contentToGUI(const CatchChallenger::Api_protocol * const api, QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard,
                                                     bool dirt, bool itemFight, bool fight, bool shop, bool heal, bool wildMonster, ActionsBotInterface::GlobalTarget &bestTarget)
{
    return contentToGUI_internal(api,listGUI,resolvedBlockList,displayTooHard,dirt,itemFight,fight,shop,heal,wildMonster,bestTarget,NULL,0,0);
}

std::vector<std::string> BotTargetList::contentToGUI(const CatchChallenger::Api_protocol * const api, QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard,
                                                     bool dirt, bool itemFight, bool fight, bool shop, bool heal, bool wildMonster, ActionsBotInterface::GlobalTarget &bestTarget,
                                                     const MapServerMini * player_map,const uint8_t &player_x,const uint8_t &player_y)
{
    return contentToGUI_internal(api,listGUI,resolvedBlockList,displayTooHard,dirt,itemFight,fight,shop,heal,wildMonster,bestTarget,player_map,player_x,player_y);
}

std::vector<std::string> BotTargetList::contentToGUI_internal(const CatchChallenger::Api_protocol * const api, QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard,
                                                     bool dirt, bool itemFight, bool fight, bool shop, bool heal, bool wildMonster, ActionsBotInterface::GlobalTarget &bestTarget,
                                                     const MapServerMini * player_map,const uint8_t &player_x,const uint8_t &player_y)
{
    //compute the forbiden direct value
    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations_ro();
    CatchChallenger::Api_client_real * apiReal=const_cast<CatchChallenger::Api_client_real *>(
                static_cast<const CatchChallenger::Api_client_real * const>(api));
    MultipleBotConnection::CatchChallengerClient * catchChallengerClient=apiToCatchChallengerClient.value(apiReal);
    uint32_t maxMonsterLevel=0;
    {
        unsigned int index=0;
        while(index<player_private_and_public_informations.playerMonster.size())
        {
            const CatchChallenger::PlayerMonster &playerMonster=player_private_and_public_informations.playerMonster.at(index);
            if(playerMonster.level>maxMonsterLevel)
                maxMonsterLevel=playerMonster.level;
            index++;
        }
    }

    std::map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> resolvedBlockListOrdered;
    for(const auto& n:resolvedBlockList)
        resolvedBlockListOrdered[n.first]=n.second;

    if(listGUI==ui->localTargets)
        mapIdListLocalTarget.clear();
    std::vector<std::string> itemToReturn;
    QColor alternateColorValue(230,230,230,255);
    QColor redColorValue(255,240,240,255);
    QColor redAlternateColorValue(255,220,220,255);

    QList<QListWidgetItem *> bestItems;
    unsigned int bestPoint=0;

    bestTarget.blockObject=NULL;
    bestTarget.extra=0;
    bestTarget.bestPath.clear();
    bestTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;

    struct BufferMonstersCollisionEntry
    {
        CatchChallenger::MonstersCollisionValue::MonstersCollisionContent bufferMonstersCollisionContent;
        std::vector<const MapServerMini::BlockObject *> blockObject;
        std::vector<MapServerMini::BlockObjectPathFinding> resolvedBlock;
    };
    std::vector<BufferMonstersCollisionEntry> bufferMonstersCollision;

    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        if(listGUI==ui->localTargets)
        {
            for(const auto& n:blockObject->links) {
                const MapServerMini::BlockObject * const nextBlock=n.first;
                const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                if(nextBlock->map!=blockObject->map)
                {
                    const std::vector<MapServerMini::BlockObject::LinkCondition> &linkConditions=linkInformation.linkConditions;
                    unsigned int indexCondition=0;
                    while(indexCondition<linkConditions.size())
                    {
                        const MapServerMini::BlockObject::LinkCondition &condition=linkConditions.at(indexCondition);
                        unsigned int index=0;
                        while(index<condition.points.size())
                        {
                            const MapServerMini::BlockObject::LinkPoint &linkPoint=condition.points.at(index);
                            switch(linkPoint.type)
                            {
                                case MapServerMini::BlockObject::LinkType::SourceTeleporter:
                                case MapServerMini::BlockObject::LinkType::SourceTopMap:
                                case MapServerMini::BlockObject::LinkType::SourceRightMap:
                                case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                                case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                                {
                                    QListWidgetItem * newItem=new QListWidgetItem();
                                    mapIdListLocalTarget.push_back(nextBlock->map->id);
                                    newItem->setIcon(QIcon(":/7.png"));
                                    switch(linkPoint.type)
                                    {
                                        case MapServerMini::BlockObject::LinkType::SourceTeleporter:
                                            newItem->setText(QString("Teleporter to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkPoint.x).arg(linkPoint.y));
                                        break;
                                        case MapServerMini::BlockObject::LinkType::SourceTopMap:
                                            newItem->setText(QString("Top border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkPoint.x).arg(linkPoint.y));
                                        break;
                                        case MapServerMini::BlockObject::LinkType::SourceRightMap:
                                            newItem->setText(QString("Right border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkPoint.x).arg(linkPoint.y));
                                        break;
                                        case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                                            newItem->setText(QString("Bottom border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkPoint.x).arg(linkPoint.y));
                                        break;
                                        case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                                            newItem->setText(QString("Left border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkPoint.x).arg(linkPoint.y));
                                        break;
                                        default:
                                        break;
                                    }
                                    switch(condition.condition.type)
                                    {
                                        case CatchChallenger::MapConditionType::MapConditionType_Clan:
                                            newItem->setText(newItem->text()+"\nIf zone owned by clan");
                                        break;
                                        case CatchChallenger::MapConditionType::MapConditionType_FightBot:
                                            newItem->setText(newItem->text()+"\nIf win fight "+QString::number(condition.condition.data.fightBot));
                                        break;
                                        case CatchChallenger::MapConditionType::MapConditionType_Item:
                                            newItem->setText(newItem->text()+"\nIf have item "+QString::number(condition.condition.data.item));
                                        break;
                                        case CatchChallenger::MapConditionType::MapConditionType_Quest:
                                            newItem->setText(newItem->text()+"\nIf finish the quest "+QString::number(condition.condition.data.quest));
                                        break;
                                        default:
                                        break;
                                    }

                                    listGUI->addItem(newItem);
                                }
                                break;
                                default:
                                break;
                            }
                            index++;
                        }
                        indexCondition++;
                    }
                }
            }
        }
    }
        //not clickable item
        //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> learn;
        //std::unordered_set<std::pair<uint8_t,uint8_t>,pairhash> market;
        //std::unordered_map<std::pair<uint8_t,uint8_t>,std::string,pairhash> zonecapture;
        //insdustry -> skip, no position control on server side
    //wild monster (and their object, day cycle)

    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        const MapServerMini * const map=blockObject->map;
        //dirt
        if(dirt)
        if(!blockObject->block.empty())
        {
            bool haveSeedToPlant=false;
            const uint32_t &itemToUse=getSeedToPlant(api,&haveSeedToPlant);
            if(haveSeedToPlant)
            {
                std::string tempMapFile=DatapackClientLoader::datapackLoader.getDatapackPath()+
                        DatapackClientLoader::datapackLoader.getMainDatapackPath()+
                        map->map_file;
                if(!stringEndsWith(tempMapFile,".tmx"))
                    tempMapFile+=".tmx";
                if(DatapackClientLoader::datapackLoader.plantOnMap.find(tempMapFile)!=DatapackClientLoader::datapackLoader.plantOnMap.cend())
                {
                    unsigned int dirtCount=0;
                    //QListWidgetItem * firstDirtItem=NULL;
                    unsigned int index=0;
                    while(index<blockObject->block.size())
                    {
                        {
                            const std::pair<uint8_t,uint8_t> &dirtPoint=blockObject->block.at(index);
                            const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &positionsList=DatapackClientLoader::datapackLoader.plantOnMap.at(tempMapFile);
                            std::vector<uint8_t,uint8_t> pos;
                            pos.first=dirtPoint.first;
                            pos.second=dirtPoint.second;
                            if(positionsList.find(pos)!=positionsList.cend())
                            {
                                const uint16_t &plantOnMapIndex=positionsList.at(pos);
                                //have plant
                                if(player_private_and_public_informations.plantOnMap.find(plantOnMapIndex)!=player_private_and_public_informations.plantOnMap.cend())
                                {
                                    const CatchChallenger::PlayerPlant &playerMonster=player_private_and_public_informations.plantOnMap.at(plantOnMapIndex);
                                    //have mature plant
                                    const uint64_t &currentTime=(uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000);
                                    bool isMature=playerMonster.mature_at<=currentTime;
                                    {
                                        const uint8_t &plantId=playerMonster.plant;
                                        const CatchChallenger::Plant &plant=CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId);
                                        const uint16_t &itemId=plant.itemUsed;
                                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(itemId);
                                        //const CatchChallenger::Plant::Rewards &rewards=plant.rewards;
                                        QListWidgetItem * newItem=new QListWidgetItem();

                                        newItem->setText(QString("Plant to collect %1 at %2,%3").arg(itemExtra.name).arg(dirtPoint.first).arg(dirtPoint.second));
                                        newItem->setIcon(QIcon(itemExtra.image));

                                        ActionsBotInterface::GlobalTarget globalTarget;
                                        globalTarget.blockObject=blockObject;
                                        globalTarget.extra=plantOnMapIndex;
                                        globalTarget.bestPath=resolvedBlock.bestPath;
                                        globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Plant;
                                        globalTarget.points=0;
                                        unsigned int points=0;
                                        if(isMature)
                                        {
                                            points=2000;//2000 is for mature plant, never be less than 1000
                                            {
                                                //remove the distance point
                                                points-=resolvedBlock.weight;
                                                //add plant value
                                                const CatchChallenger::Item &item=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(itemId);
                                                points+=item.price;
                                                //if not consumable and player don't have it
                                                if(!item.consumeAtUse && player_private_and_public_informations.items.find(itemId)==player_private_and_public_informations.items.cend())
                                                    points=points*14/10;//+40%
                                                if(player_private_and_public_informations.items.find(itemId)!=player_private_and_public_informations.items.cend())
                                                {
                                                    if(!item.consumeAtUse)
                                                        points=0;
                                                    else
                                                    {
                                                        /*Always collect the mature plant
                                                         * const uint32_t &quantity=player_private_and_public_informations.items.at(itemId);
                                                        if(quantity>20)
                                                            points=0;
                                                        else if(quantity>10)
                                                            points/=4;*/
                                                    }
                                                }
                                                if(player_map!=NULL && player_map==map)
                                                {
                                                    const uint16_t &distance=mapPointDistanceNormalised(pos.first,pos.second,player_x,player_y);
                                                    if(distance<3)
                                                        points*=(3+3-distance);
                                                    else if(distance<5)
                                                        points*=2;
                                                    else
                                                        points-=distance;
                                                }

                                                points=points*catchChallengerClient->preferences.plant/100;
                                                globalTarget.points=points;
                                                globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                                                if(bestPoint<points)
                                                    bestPoint=points;
                                            }
                                        }

                                        if(listGUI==ui->globalTargets)
                                        {
                                            if(isMature)
                                            {
                                                targetListGlobalTarget.push_back(globalTarget);
                                                if(alternateColor)
                                                    newItem->setBackgroundColor(alternateColorValue);
                                                newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                                            }
                                            else
                                            {
                                                if(alternateColor)
                                                    newItem->setBackgroundColor(redAlternateColorValue);
                                                else
                                                    newItem->setBackgroundColor(redColorValue);
                                                newItem->setText(newItem->text()+" "+QString::number(playerMonster.mature_at)+">"+QString::number(currentTime));
                                                newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                                            }
                                            alternateColor=!alternateColor;
                                        }
                                        itemToReturn.push_back(newItem->text().toStdString());
                                        if(listGUI!=NULL && isMature)
                                            listGUI->addItem(newItem);
                                        else
                                            delete newItem;
                                    }
                                }
                                //empty dirt
                                else
                                {
                                    if(dirtCount==0)
                                    {
                                        QListWidgetItem * newItem=new QListWidgetItem();
                                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(itemToUse);
                                        newItem->setText(QString("Free dirt (use %1 at %2,%3)").arg(QString::fromStdString(itemExtra.name)).arg(dirtPoint.first).arg(dirtPoint.second));
                                        //firstDirtItem=newItem;

                                        ActionsBotInterface::GlobalTarget globalTarget;
                                        globalTarget.blockObject=blockObject;
                                        globalTarget.extra=plantOnMapIndex;
                                        globalTarget.bestPath=resolvedBlock.bestPath;
                                        globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Dirt;
                                        globalTarget.points=0;

                                        uint32_t points=1400;//2000 is for mature plant, never be less than 1000
                                        {
                                            //remove the distance point
                                            points-=resolvedBlock.weight;

                                            if(player_map!=NULL && player_map==map)
                                            {
                                                const uint16_t &distance=mapPointDistanceNormalised(pos.first,pos.second,player_x,player_y);
                                                if(distance<3)
                                                    points*=(3+3-distance);
                                                else if(distance<5)
                                                    points*=2;
                                                else
                                                    points-=distance;
                                            }

                                            points=points*catchChallengerClient->preferences.plant/100;
                                            globalTarget.points=points;
                                            globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                                            if(bestPoint<points)
                                                bestPoint=points;
                                        }

                                        newItem->setIcon(QIcon(":/dirt.png"));
                                        if(listGUI==ui->globalTargets)
                                        {
                                            targetListGlobalTarget.push_back(globalTarget);
                                            if(alternateColor)
                                                newItem->setBackgroundColor(alternateColorValue);
                                            alternateColor=!alternateColor;
                                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                                        }
                                        itemToReturn.push_back(newItem->text().toStdString());
                                        if(listGUI!=NULL)
                                            listGUI->addItem(newItem);
                                        else
                                            delete newItem;

                                        dirtCount++;
                                    }
                                    else
                                    {
                                        dirtCount++;
                                        //firstDirtItem->setText(QString("%1 free dirts").arg(dirtCount));
                                    }
                                }
                            }
                        }

                        index++;
                    }
                }
            }
        }
    }
    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        //item on map
        if(itemFight)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            {
                std::unordered_set<uint32_t> known_indexOfItemOnMap;
                for ( const auto &item : blockObject->pointOnMap_Item )
                {
                    const MapServerMini::ItemOnMap &itemOnMap=item.second;
                    if(known_indexOfItemOnMap.find(itemOnMap.indexOfItemOnMap)!=known_indexOfItemOnMap.cend())
                        abort();
                    known_indexOfItemOnMap.insert(itemOnMap.indexOfItemOnMap);
                }
            }
            #endif
            for(auto it = blockObject->pointOnMap_Item.begin();it!=blockObject->pointOnMap_Item.cend();++it)
            {
                const std::pair<uint8_t,uint8_t> &point=it->first;
                const MapServerMini::ItemOnMap &itemOnMap=it->second;
                /*if(blockObject->map->map_file=="hidden-place" && itemOnMap.item==101)
                    std::cout << "test tmp" << std::endl;*/

                if(player_private_and_public_informations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==player_private_and_public_informations.itemOnMap.cend())
                {
                    const CatchChallenger::Item &item=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(itemOnMap.item);
                    const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(itemOnMap.item);
                    QListWidgetItem * newItem=new QListWidgetItem();
                    if(itemOnMap.infinite)
                        newItem->setText(QString("Item on map %1 %2$ (infinite)").arg(QString::fromStdString(itemExtra.name)).arg(item.price));
                    else
                        newItem->setText(QString("Item on map %1 %2$").arg(QString::fromStdString(itemExtra.name)).arg(item.price));
                    newItem->setIcon(QIcon(itemExtra.image));

                    ActionsBotInterface::GlobalTarget globalTarget;
                    globalTarget.blockObject=blockObject;
                    globalTarget.extra=itemOnMap.indexOfItemOnMap;
                    globalTarget.bestPath=resolvedBlock.bestPath;
                    globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap;
                    globalTarget.points=0;
                    unsigned int points=2000;//2000 is for mature plant, never be less than 1000
                    if(itemOnMap.infinite)
                        points=1500;
                    {
                        //remove the distance point
                        points-=resolvedBlock.weight;
                        //add plant value, add 0 to 800 ln(price)
                        if(ActionsAction::actionsAction->maxitemprice>0 && item.price>0)
                        {
                            //1 to 10000
                            const double linear10000price=
                                    (item.price-ActionsAction::actionsAction->minitemprice)*
                                    (10000-1)/
                                    ActionsAction::actionsAction->maxitemprice+1;
                            //now with log scale, 0 to 800
                            points+=log(linear10000price)*800/log(10000);
                        }
                        //if not consumable and player don't have it
                        if(!item.consumeAtUse && player_private_and_public_informations.items.find(itemOnMap.item)==
                                player_private_and_public_informations.items.cend())
                            points=points*13/10;//+30%
                        if(player_private_and_public_informations.items.find(itemOnMap.item)!=player_private_and_public_informations.items.cend())
                        {
                            if(!item.consumeAtUse)
                                points=0;
                            else
                            {
                                const uint32_t &quantity=player_private_and_public_informations.items.at(itemOnMap.item);
                                if(quantity>20)
                                    points=0;
                                else if(quantity>10)
                                    points/=4;
                            }
                        }
                        if(player_map==NULL)
                        {
                            const uint16_t &distance=mapPointDistanceNormalised(point.first,point.second,player_x,player_y);
                            if(distance<2)
                                points*=4;
                            else if(distance<5)
                                points*=2;
                        }

                        points=points*catchChallengerClient->preferences.item/100;
                        globalTarget.points=points;
                        globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                        if(bestPoint<points)
                            bestPoint=points;
                    }

                    if(listGUI==ui->globalTargets)
                    {
                        targetListGlobalTarget.push_back(globalTarget);

                        if(alternateColor)
                            newItem->setBackgroundColor(alternateColorValue);
                        alternateColor=!alternateColor;
                        newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                    }
                    itemToReturn.push_back(newItem->text().toStdString());
                    if(listGUI!=NULL)
                        listGUI->addItem(newItem);
                    else
                        delete newItem;
                }
            }
        }
    }
    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        //fight
        if(fight)
        {
            for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
            {
                const std::pair<uint8_t,uint8_t> &point=it->first;
                const std::vector<uint16_t> &botsFightList=it->second;
                unsigned int index=0;
                while(index<botsFightList.size())
                {
                    const uint32_t &fightId=botsFightList.at(index);
                    if(!ActionsAction::haveBeatBot(api,fightId))
                    {
                        const CatchChallenger::BotFight &fight=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId);
                        ActionsBotInterface::GlobalTarget globalTarget;
                        globalTarget.blockObject=blockObject;
                        globalTarget.extra=fightId;
                        globalTarget.bestPath=resolvedBlock.bestPath;
                        globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight;
                        globalTarget.points=0;

                        uint8_t maxFightLevel=0;
                        {
                            unsigned int sub_index=0;
                            while(sub_index<fight.monsters.size())
                            {
                                const CatchChallenger::BotFight::BotFightMonster &monster=fight.monsters.at(sub_index);
                                if(monster.level>maxFightLevel)
                                    maxFightLevel=monster.level;
                                sub_index++;
                            }
                        }

                        QString tips;
                        QColor colorValueL;
                        QColor alternateColorValueL;
                        const bool tooHard=maxFightLevel>(maxMonsterLevel+2);
                        unsigned int points=0;
                        if(tooHard)
                        {
                            colorValueL=redColorValue;
                            alternateColorValueL=redAlternateColorValue;
                            tips="Too hard fight";
                        }
                        else
                        {
                            //colorValue=;
                            alternateColorValueL=alternateColorValue;

                            points=1500;//never be less than 1000
                            {
                                //remove the distance point
                                points-=resolvedBlock.weight;
                                unsigned int sub_index=0;
                                while(sub_index<fight.items.size())
                                {
                                    const CatchChallenger::BotFight::Item &itemFight=fight.items.at(sub_index);
                                    const CatchChallenger::Item &item=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(itemFight.id);
                                    //add plant value
                                    unsigned int addPoint=item.price;
                                    //if not consumable and player don't have it
                                    if(!item.consumeAtUse && player_private_and_public_informations.items.find(itemFight.id)==player_private_and_public_informations.items.cend())
                                        addPoint=addPoint*14/10;//+40%
                                    if(player_private_and_public_informations.items.find(itemFight.id)!=player_private_and_public_informations.items.cend())
                                    {
                                        if(!item.consumeAtUse)
                                            addPoint=0;
                                        else
                                        {
                                            const uint32_t &quantity=player_private_and_public_informations.items.at(itemFight.id);
                                            if(quantity>20)
                                                addPoint=0;
                                            else if(quantity>10)
                                                addPoint/=4;
                                        }
                                    }
                                    points+=addPoint;
                                    sub_index++;
                                }
                                if(player_map==NULL)
                                {
                                    const uint16_t &distance=mapPointDistanceNormalised(point.first,point.second,player_x,player_y);
                                    if(distance<2)
                                        points*=4;
                                    else if(distance<5)
                                        points*=2;
                                }

                                points=points*catchChallengerClient->preferences.fight/100;
                                globalTarget.points=points;
                                if(bestPoint<points)
                                    bestPoint=points;
                            }
                        }

                        if(displayTooHard || !tooHard)
                        {
                            //item
                            {
                                unsigned int sub_index=0;
                                while(sub_index<fight.items.size())
                                {
                                    const CatchChallenger::BotFight::Item &item=fight.items.at(sub_index);
                                    const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(item.id);
                                    const uint32_t &quantity=item.quantity;
                                    {
                                        QListWidgetItem * newItem=new QListWidgetItem();
                                        if(quantity>1)
                                            newItem->setText(QString("Fight %1: %2x %3")
                                                          .arg(fightId)
                                                          .arg(quantity)
                                                          .arg(QString::fromStdString(itemExtra.name))
                                                          );
                                        else
                                            newItem->setText(QString("Fight %1: %2")
                                                          .arg(fightId)
                                                          .arg(QString::fromStdString(itemExtra.name))
                                                          );
                                        newItem->setIcon(QIcon(itemExtra.image));
                                        newItem->setToolTip(tips);
                                        itemToReturn.push_back(newItem->text().toStdString());
                                        if(listGUI==ui->globalTargets)
                                        {
                                            globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                                            targetListGlobalTarget.push_back(globalTarget);
                                            if(alternateColor)
                                                newItem->setBackgroundColor(alternateColorValueL);
                                            else if(colorValueL.isValid())
                                                newItem->setBackgroundColor(colorValueL);
                                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                                        }
                                        if(listGUI!=NULL)
                                            listGUI->addItem(newItem);
                                        else
                                            delete newItem;
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
                                    const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.at(monster.id);
                                    {
                                        QListWidgetItem * newItem=new QListWidgetItem();
                                        newItem->setText(QString("Fight %1: %2 level %3")
                                                .arg(fightId)
                                                .arg(QString::fromStdString(monsterExtra.name))
                                                .arg(monster.level)
                                                );
                                        newItem->setIcon(QIcon(monsterExtra.thumb));
                                        newItem->setToolTip(tips);
                                        itemToReturn.push_back(newItem->text().toStdString());
                                        if(listGUI==ui->globalTargets)
                                        {
                                            globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                                            targetListGlobalTarget.push_back(globalTarget);
                                            if(alternateColor)
                                                newItem->setBackgroundColor(alternateColorValueL);
                                            else if(colorValueL.isValid())
                                                newItem->setBackgroundColor(colorValueL);
                                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                                        }
                                        if(listGUI!=NULL)
                                            listGUI->addItem(newItem);
                                        else
                                            delete newItem;
                                    }
                                    sub_index++;
                                }
                            }
                            if(!fight.items.empty() || !fight.monsters.empty())
                                if(listGUI==ui->globalTargets)
                                    alternateColor=!alternateColor;
                        }
                    }
                    index++;
                }
            }
        }
    }
    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        //shop
        if(shop)
        {
            for(auto it = blockObject->shops.begin();it!=blockObject->shops.cend();++it)
            {
                const std::vector<uint16_t> &shops=it->second;
                unsigned int index=0;
                while(index<shops.size())
                {
                    const uint32_t &shopId=shops.at(index);
                    const CatchChallenger::Shop &shop=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId);
                    ActionsBotInterface::GlobalTarget globalTarget;
                    globalTarget.blockObject=blockObject;
                    globalTarget.extra=shopId;
                    globalTarget.bestPath=resolvedBlock.bestPath;
                    globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Shop;
                    globalTarget.points=0*catchChallengerClient->preferences.shop/100;

                    unsigned int sub_index=0;
                    while(sub_index<shop.prices.size())
                    {
                        const CATCHCHALLENGER_TYPE_ITEM &item=shop.items.at(sub_index);
                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(item);
                        const uint32_t &price=shop.prices.at(sub_index);
                        {
                            const bool tooHard=price>player_private_and_public_informations.cash;
                            if(displayTooHard || !tooHard)
                            {
                                QListWidgetItem * newItem=new QListWidgetItem();
                                newItem->setText(QString("Shop %1: %2 %3$")
                                        .arg(shopId)
                                        .arg(QString::fromStdString(itemExtra.name))
                                        .arg(price)
                                                 );
                                newItem->setIcon(QIcon(itemExtra.image));

                                itemToReturn.push_back(newItem->text().toStdString());
                                if(listGUI==ui->globalTargets)
                                {
                                    targetListGlobalTarget.push_back(globalTarget);
                                    if(tooHard)
                                    {
                                        if(alternateColor)
                                            newItem->setBackgroundColor(redAlternateColorValue);
                                        else
                                            newItem->setBackgroundColor(redColorValue);
                                    }
                                    else
                                        if(alternateColor)
                                            newItem->setBackgroundColor(alternateColorValue);
                                    newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                                }
                                if(listGUI!=NULL)
                                    listGUI->addItem(newItem);
                                else
                                    delete newItem;
                            }
                        }
                        sub_index++;
                    }
                    if(!shop.prices.empty())
                        if(listGUI==ui->globalTargets)
                            alternateColor=!alternateColor;
                    index++;
                }
            }
        }
    }
    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        //heal
        if(heal)
        if(blockObject->heal.size()>0)
        {
            ActionsBotInterface::GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=0;
            globalTarget.bestPath=resolvedBlock.bestPath;
            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal;
            globalTarget.points=0*catchChallengerClient->preferences.fight/100;

            QListWidgetItem * newItem=new QListWidgetItem();
            newItem->setText(QString("Heal"));
            newItem->setIcon(QIcon(":/1.png"));

            itemToReturn.push_back(newItem->text().toStdString());
            if(listGUI==ui->globalTargets)
            {
                targetListGlobalTarget.push_back(globalTarget);
                if(alternateColor)
                    newItem->setBackgroundColor(alternateColorValue);
                newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                alternateColor=!alternateColor;
            }
            if(listGUI!=NULL)
                listGUI->addItem(newItem);
            else
                delete newItem;
        }
    }
    for(const auto& n:resolvedBlockListOrdered) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        /*if(blockObject->map->map_file=="road" && (blockObject->id+1)==6)
            std::cout << std::to_string(blockObject->id+1)<< std::endl;*/
        //the wild monster
        if(wildMonster)
        if(blockObject->monstersCollisionValue!=NULL)
        {
            const CatchChallenger::MonstersCollisionValue &monsterCollisionValue=*blockObject->monstersCollisionValue;
            if(!monsterCollisionValue.walkOnMonsters.empty())
            {
                /// \todo do the real code
                //too hard do after
                //choice the first entry
                const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=monsterCollisionValue.walkOnMonsters.at(0);
                if(!monsterCollisionContent.defaultMonsters.empty())
                {
                    //search
                    unsigned int bufferMonstersCollisionIndex=0;
                    while(bufferMonstersCollisionIndex<bufferMonstersCollision.size())
                    {
                        BufferMonstersCollisionEntry &bufferMonstersCollisionEntry=bufferMonstersCollision[bufferMonstersCollisionIndex];
                        if(isSame(bufferMonstersCollisionEntry.bufferMonstersCollisionContent,monsterCollisionContent))
                        {
                            if(bufferMonstersCollisionEntry.resolvedBlock.back().weight==resolvedBlock.weight)
                            {
                                bufferMonstersCollisionEntry.blockObject.push_back(blockObject);
                                bufferMonstersCollisionEntry.resolvedBlock.push_back(resolvedBlock);
                            }
                            else if(bufferMonstersCollisionEntry.resolvedBlock.back().weight>resolvedBlock.weight)
                            {
                                bufferMonstersCollisionEntry.blockObject.clear();
                                bufferMonstersCollisionEntry.resolvedBlock.clear();
                                bufferMonstersCollisionEntry.blockObject.push_back(blockObject);
                                bufferMonstersCollisionEntry.resolvedBlock.push_back(resolvedBlock);
                            }
                            break;
                        }
                        bufferMonstersCollisionIndex++;
                    }
                    //if not found append
                    if(bufferMonstersCollisionIndex>=bufferMonstersCollision.size())
                    {
                        BufferMonstersCollisionEntry bufferMonstersCollisionEntry;
                        bufferMonstersCollisionEntry.blockObject.push_back(blockObject);
                        bufferMonstersCollisionEntry.resolvedBlock.push_back(resolvedBlock);
                        bufferMonstersCollisionEntry.bufferMonstersCollisionContent=monsterCollisionContent;
                        bufferMonstersCollision.push_back(bufferMonstersCollisionEntry);
                    }
                }
            }
        }
    }

    //flush the monster buffer
    {
        unsigned int buffer_index=0;
        while(buffer_index<bufferMonstersCollision.size())
        {
            const BufferMonstersCollisionEntry &bufferMonstersCollisionEntry=bufferMonstersCollision.at(buffer_index);
            //std::cout << "Choose monster between: " << std::to_string(bufferMonstersCollisionEntry.blockObject.size()) << std::endl;
            const unsigned int &wilZoneIndex=((uint64_t)api)%bufferMonstersCollisionEntry.blockObject.size();
            const MapServerMini::BlockObject * const blockObject=bufferMonstersCollisionEntry.blockObject.at(wilZoneIndex);
            const MapServerMini::BlockObjectPathFinding &resolvedBlock=bufferMonstersCollisionEntry.resolvedBlock.at(wilZoneIndex);
            const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=bufferMonstersCollisionEntry.bufferMonstersCollisionContent;

            ActionsBotInterface::GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=0;
            globalTarget.bestPath=resolvedBlock.bestPath;
            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster;
            globalTarget.points=0;

            uint8_t maxFightLevel=0;
            {
                unsigned int sub_index=0;
                while(sub_index<monsterCollisionContent.defaultMonsters.size())
                {
                    const CatchChallenger::MapMonster &monster=monsterCollisionContent.defaultMonsters.at(sub_index);
                    if(monster.maxLevel>maxFightLevel)
                        maxFightLevel=monster.maxLevel;
                    sub_index++;
                }
            }

            QString tips;
            QColor colorValueL;
            QColor alternateColorValueL;
            const bool tooHard=maxFightLevel>(maxMonsterLevel+2);
            unsigned int points=0;
            if(tooHard)
            {
                colorValueL=redColorValue;
                alternateColorValueL=redAlternateColorValue;
                tips="Too hard wild fight";
            }
            else
            {
                //colorValue=;
                alternateColorValueL=alternateColorValue;

                unsigned int addPoint=0;
                points=1000;//never be less than 1000
                {
                    unsigned int totalLuck=0;
                    //remove the distance point
                    points-=resolvedBlock.weight;
                    unsigned int sub_index=0;
                    while(sub_index<monsterCollisionContent.defaultMonsters.size())
                    {
                        const CatchChallenger::MapMonster &mapMonster=monsterCollisionContent.defaultMonsters.at(sub_index);
                        //const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(mapMonster.id);
                        if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.monsterDrops.find(mapMonster.id)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.monsterDrops.cend())
                        {
                            std::cerr << "drop not found for mapMonster.id: " << mapMonster.id << std::endl;
                            abort();
                        }
                        const std::vector<CatchChallenger::MonsterDrops> &drops=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.monsterDrops.at(mapMonster.id);
                        unsigned int indexDrop=0;
                        unsigned int tempPoint=0;
                        while(indexDrop<drops.size())
                        {
                            const CatchChallenger::MonsterDrops &drop=drops.at(indexDrop);
                            const CatchChallenger::Item &item=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(drop.item);
                            //add plant value
                            tempPoint=(float)item.price*((double)(drop.quantity_min+drop.quantity_max)/2)*drop.luck/100;
                            //if not consumable and player don't have it
                            if(!item.consumeAtUse && player_private_and_public_informations.items.find(drop.item)==player_private_and_public_informations.items.cend())
                                tempPoint=tempPoint*14/10;//+40%
                            if(player_private_and_public_informations.items.find(drop.item)!=player_private_and_public_informations.items.cend())
                            {
                                if(!item.consumeAtUse)
                                    tempPoint=0;
                                else
                                {
                                    const uint32_t &quantity=player_private_and_public_informations.items.at(drop.item);
                                    if(quantity>20)
                                        tempPoint=0;
                                    else if(quantity>10)
                                        tempPoint/=4;
                                }
                            }
                            indexDrop++;
                        }
                        totalLuck+=mapMonster.luck;
                        addPoint+=tempPoint*mapMonster.luck;
                        sub_index++;
                    }
                    addPoint/=totalLuck;
                    points+=addPoint;

                    points=points*catchChallengerClient->preferences.wild/100;
                    globalTarget.points=points;
                    if(bestPoint<points)
                        bestPoint=points;
                }
            }

            if(displayTooHard || !tooHard)
            {
                unsigned int sub_index=0;
                while(sub_index<monsterCollisionContent.defaultMonsters.size())
                {
                    const CatchChallenger::MapMonster &mapMonster=monsterCollisionContent.defaultMonsters.at(sub_index);
                    const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.at(mapMonster.id);
                    {
                        QListWidgetItem * newItem=new QListWidgetItem();
                        newItem->setText(QString("Wild %2 level %3-%4, luck: %5")
                                .arg(QString::fromStdString(monsterExtra.name))
                                .arg(mapMonster.minLevel)
                                .arg(mapMonster.maxLevel)
                                .arg(QString::number(mapMonster.luck)+"%")
                                );
                        newItem->setIcon(QIcon(monsterExtra.thumb));
                        newItem->setToolTip(tips);

                        itemToReturn.push_back(newItem->text().toStdString());
                        if(listGUI==ui->globalTargets)
                        {
                            globalTarget.uiItems=QList<QListWidgetItem *>() << newItem;
                            targetListGlobalTarget.push_back(globalTarget);
                            if(alternateColor)
                                newItem->setBackgroundColor(alternateColorValueL);
                            else if(colorValueL.isValid())
                                newItem->setBackgroundColor(colorValueL);
                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock,points)));
                        }
                        if(listGUI!=NULL)
                            listGUI->addItem(newItem);
                        else
                            delete newItem;
                    }
                    sub_index++;
                }
                if(listGUI==ui->globalTargets)
                    alternateColor=!alternateColor;
            }
            buffer_index++;
        }
    }

    //choose the final target
    {
        unsigned int lowerBestPoints=bestPoint*90/100;//90%
        std::vector<ActionsBotInterface::GlobalTarget> bestTargetListGlobalTarget;
        size_t index=0;
        while(index<targetListGlobalTarget.size())
        {
            const ActionsBotInterface::GlobalTarget &tempTarget=targetListGlobalTarget.at(index);
            if(tempTarget.points>=lowerBestPoints)
                bestTargetListGlobalTarget.push_back(tempTarget);
            index++;
        }

        if(!bestTargetListGlobalTarget.empty())
        {
            const ActionsBotInterface::GlobalTarget &finalTarget=bestTargetListGlobalTarget.at(rand()%bestTargetListGlobalTarget.size());
            bestItems=finalTarget.uiItems;
            bestTarget=finalTarget;
        }
    }

    if(listGUI==ui->globalTargets)
    {
        unsigned int index=0;
        while(index<(unsigned int)bestItems.size())
        {
            bestItems.at(index)->setBackgroundColor(QColor(180,255,180,255));
            bestItems.at(index)->setToolTip("Internal id: "+QString::number(index));
            index++;
        }
    }
    if(listGUI==ui->globalTargets)
        if(targetListGlobalTarget.size()!=(uint32_t)ui->globalTargets->count())
        {
            std::cerr << "The target count not match with visual elements" << std::endl;
            abort();
        }

    ui->PrefFight->setValue(catchChallengerClient->preferences.fight);
    ui->PrefItem->setValue(catchChallengerClient->preferences.item);
    ui->PrefPlant->setValue(catchChallengerClient->preferences.plant);
    ui->PrefShop->setValue(catchChallengerClient->preferences.shop);
    ui->PrefWild->setValue(catchChallengerClient->preferences.wild);

    return itemToReturn;
}

std::string BotTargetList::pathFindingToString(const MapServerMini::BlockObjectPathFinding &resolvedBlock, unsigned int points)
{
    if(resolvedBlock.bestPath.empty())
    {
        if(points>0)
            return "\npoints: "+std::to_string(points);
        else
            return std::string();
    }
    std::string str;
    unsigned int index=0;
    while(index<resolvedBlock.bestPath.size())
    {
        if(!str.empty())
            str+=", ";
        const MapServerMini::BlockObject * const blockObject=resolvedBlock.bestPath.at(index);
        const MapServerMini::MapParsedForBot::Layer * const layer=static_cast<const MapServerMini::MapParsedForBot::Layer *>(blockObject->layer);
        str+=blockObject->map->map_file+"/"+layer->name;
        index++;
    }
    str+="\n";
    str+="distance: "+std::to_string(resolvedBlock.weight);
    if(points>0)
        str+=", points: "+std::to_string(points);
    str+="";
    return "\n"+str;
}

bool BotTargetList::isSame(const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentA,const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monstersCollisionContentB)
{
    if(monstersCollisionContentA.defaultMonsters.size()!=monstersCollisionContentB.defaultMonsters.size())
        return false;
    std::vector<CatchChallenger::MapMonster> defaultMonstersA=monstersCollisionContentA.defaultMonsters;
    std::vector<CatchChallenger::MapMonster> defaultMonstersB=monstersCollisionContentB.defaultMonsters;
    std::sort(defaultMonstersA.begin(),defaultMonstersA.end(), [](CatchChallenger::MapMonster a, CatchChallenger::MapMonster b) {
        if(a.id<b.id)
            return true;
        if(a.luck<b.luck)
            return true;
        if(a.maxLevel<b.maxLevel)
            return true;
        if(a.minLevel<b.minLevel)
            return true;
        return false;
    });
    std::sort(defaultMonstersB.begin(),defaultMonstersB.end(), [](CatchChallenger::MapMonster a, CatchChallenger::MapMonster b) {
        if(a.id<b.id)
            return true;
        if(a.luck<b.luck)
            return true;
        if(a.maxLevel<b.maxLevel)
            return true;
        if(a.minLevel<b.minLevel)
            return true;
        return false;
    });
    unsigned int index=0;
    while(index<defaultMonstersA.size())
    {
        const CatchChallenger::MapMonster &monstersA=defaultMonstersA.at(index);
        const CatchChallenger::MapMonster &monstersB=defaultMonstersB.at(index);
        if(monstersA.id!=monstersB.id)
            return false;
        if(monstersA.luck!=monstersB.luck)
            return false;
        if(monstersA.maxLevel!=monstersB.maxLevel)
            return false;
        if(monstersA.minLevel!=monstersB.minLevel)
            return false;
        index++;
    }
    return true;
}
