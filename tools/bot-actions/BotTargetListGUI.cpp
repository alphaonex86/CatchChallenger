#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"
#include <chrono>

std::vector<std::string> BotTargetList::contentToGUI(const MapServerMini::BlockObject * const blockObject, const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI)
{
    std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> resolvedBlockList;
    MapServerMini::BlockObjectPathFinding pathFinding;
    pathFinding.weight=0;
    resolvedBlockList[blockObject]=pathFinding;
    return contentToGUI(client,listGUI,resolvedBlockList);
}

uint32_t BotTargetList::getSeedToPlant(CatchChallenger::Api_protocol * api,bool *haveSeedToPlant)
{
    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
    auto i=player_private_and_public_informations.items.begin();
    while (i!=player_private_and_public_informations.items.cend())
    {
        const uint16_t &itemId=i->first;
        if(DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
        {
            /// \todo check the requirement
            const uint8_t &plantId=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);

            if(ActionsAction::haveReputationRequirements(api,CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
            {
                if(haveSeedToPlant!=NULL)
                    *haveSeedToPlant=true;
                return itemId;
            }
        }
        ++i;
    }
    if(haveSeedToPlant!=NULL)
        *haveSeedToPlant=false;
    return 0;
}

std::vector<std::string> BotTargetList::contentToGUI(const MultipleBotConnection::CatchChallengerClient * const client, QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList, const bool &displayTooHard)
{
    //compute the forbiden direct value
    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->api->get_player_informations();
    CatchChallenger::Api_client_real * api=client->api;
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

    if(listGUI==ui->localTargets)
        mapIdListLocalTarget.clear();
    std::vector<std::string> itemToReturn;
    QColor alternateColorValue(230,230,230,255);
    QColor redColorValue(255,240,240,255);
    QColor redAlternateColorValue(255,220,220,255);

    struct BufferMonstersCollisionEntry
    {
        CatchChallenger::MonstersCollisionValue::MonstersCollisionContent bufferMonstersCollisionContent;
        const MapServerMini::BlockObject * blockObject;
        MapServerMini::BlockObjectPathFinding resolvedBlock;
    };
    std::vector<BufferMonstersCollisionEntry> bufferMonstersCollision;

    for(const auto& n:resolvedBlockList) {
        const MapServerMini::BlockObject * const blockObject=n.first;
        const MapServerMini::BlockObjectPathFinding &resolvedBlock=n.second;
        const MapServerMini * const map=blockObject->map;
        if(listGUI==ui->localTargets)
        {
            for(const auto& n:blockObject->links) {
                const MapServerMini::BlockObject * const nextBlock=n.first;
                const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                if(nextBlock->map!=blockObject->map)
                {
                    unsigned int index=0;
                    while(index<linkInformation.points.size())
                    {
                        const MapServerMini::BlockObject::LinkPoint &linkPoint=linkInformation.points.at(index);
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
                                listGUI->addItem(newItem);
                            }
                            break;
                            default:
                            break;
                        }
                        index++;
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

        //dirt
        if(!blockObject->dirt.empty())
        {
            bool haveSeedToPlant=false;
            const uint32_t &itemToUse=getSeedToPlant(api,&haveSeedToPlant);
            if(haveSeedToPlant)
            {
                QString tempMapFile=DatapackClientLoader::datapackLoader.getDatapackPath()+DatapackClientLoader::datapackLoader.getMainDatapackPath()+QString::fromStdString(map->map_file);
                if(!tempMapFile.endsWith(".tmx"))
                    tempMapFile+=".tmx";
                if(DatapackClientLoader::datapackLoader.plantOnMap.contains(tempMapFile))
                {
                    unsigned int dirtCount=0;
                    //QListWidgetItem * firstDirtItem=NULL;
                    unsigned int index=0;
                    while(index<blockObject->dirt.size())
                    {
                        {
                            const std::pair<uint8_t,uint8_t> &dirtPoint=blockObject->dirt.at(index);
                            const QHash<QPair<uint8_t,uint8_t>,uint16_t> &positionsList=DatapackClientLoader::datapackLoader.plantOnMap.value(tempMapFile);
                            QPair<uint8_t,uint8_t> pos;
                            pos.first=dirtPoint.first;
                            pos.second=dirtPoint.second;
                            if(positionsList.contains(pos))
                            {
                                const uint16_t &plantOnMapIndex=positionsList.value(pos);
                                //have plant
                                if(player_private_and_public_informations.plantOnMap.find(plantOnMapIndex)!=player_private_and_public_informations.plantOnMap.cend())
                                {
                                    const CatchChallenger::PlayerPlant &playerMonster=player_private_and_public_informations.plantOnMap.at(plantOnMapIndex);
                                    //have mature plant
                                    if(playerMonster.mature_at>(uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000))
                                    {
                                        const uint8_t &plantId=playerMonster.plant;
                                        const CatchChallenger::Plant &plant=CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId);
                                        const uint16_t &itemId=plant.itemUsed;
                                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(itemId);
                                        //const CatchChallenger::Plant::Rewards &rewards=plant.rewards;
                                        QListWidgetItem * newItem=new QListWidgetItem();

                                        newItem->setText(QString("Plant to collect %1").arg(itemExtra.name));
                                        newItem->setIcon(QIcon(itemExtra.image));
                                        if(listGUI==ui->globalTargets)
                                        {
                                            ActionsBotInterface::GlobalTarget globalTarget;
                                            globalTarget.blockObject=blockObject;
                                            globalTarget.extra=plantOnMapIndex;
                                            globalTarget.bestPath=resolvedBlock.bestPath;
                                            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Plant;
                                            targetListGlobalTarget.push_back(globalTarget);
                                            if(alternateColor)
                                                newItem->setBackgroundColor(alternateColorValue);
                                            alternateColor=!alternateColor;
                                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                                        }
                                        itemToReturn.push_back(newItem->text().toStdString());
                                        if(listGUI!=NULL)
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
                                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(itemToUse);
                                        newItem->setText(QString("Free dirt (use %1)").arg(itemExtra.name));
                                        //firstDirtItem=newItem;

                                        newItem->setIcon(QIcon(":/dirt.png"));
                                        if(listGUI==ui->globalTargets)
                                        {
                                            ActionsBotInterface::GlobalTarget globalTarget;
                                            globalTarget.blockObject=blockObject;
                                            globalTarget.extra=plantOnMapIndex;
                                            globalTarget.bestPath=resolvedBlock.bestPath;
                                            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Dirt;
                                            targetListGlobalTarget.push_back(globalTarget);
                                            if(alternateColor)
                                                newItem->setBackgroundColor(alternateColorValue);
                                            alternateColor=!alternateColor;
                                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
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
        //item on map
        {
            for(auto it = blockObject->pointOnMap_Item.begin();it!=blockObject->pointOnMap_Item.cend();++it)
            {
                const MapServerMini::ItemOnMap &itemOnMap=it->second;

                if(player_private_and_public_informations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==player_private_and_public_informations.itemOnMap.cend())
                {
                    const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(itemOnMap.item);
                    QListWidgetItem * newItem=new QListWidgetItem();
                    if(itemOnMap.infinite)
                        newItem->setText(QString("Item on map %1 (infinite)").arg(itemExtra.name));
                    else
                        newItem->setText(QString("Item on map %1").arg(itemExtra.name));
                    newItem->setIcon(QIcon(itemExtra.image));
                    if(listGUI==ui->globalTargets)
                    {
                        ActionsBotInterface::GlobalTarget globalTarget;
                        globalTarget.blockObject=blockObject;
                        globalTarget.extra=itemOnMap.item;
                        globalTarget.bestPath=resolvedBlock.bestPath;
                        globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap;
                        targetListGlobalTarget.push_back(globalTarget);
                        if(alternateColor)
                            newItem->setBackgroundColor(alternateColorValue);
                        alternateColor=!alternateColor;
                        newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                    }
                    itemToReturn.push_back(newItem->text().toStdString());
                    if(listGUI!=NULL)
                        listGUI->addItem(newItem);
                    else
                        delete newItem;
                }
            }
        }
        //fight
        {
            for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
            {
                const std::vector<uint32_t> &botsFightList=it->second;
                unsigned int index=0;
                while(index<botsFightList.size())
                {
                    const uint32_t &fightId=botsFightList.at(index);
                    const CatchChallenger::BotFight &fight=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId);
                    ActionsBotInterface::GlobalTarget globalTarget;
                    globalTarget.blockObject=blockObject;
                    globalTarget.extra=fightId;
                    globalTarget.bestPath=resolvedBlock.bestPath;
                    globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight;

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
                    }

                    if(displayTooHard || !tooHard)
                    {
                        //item
                        {
                            unsigned int sub_index=0;
                            while(sub_index<fight.items.size())
                            {
                                const CatchChallenger::BotFight::Item &item=fight.items.at(sub_index);
                                const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(item.id);
                                const uint32_t &quantity=item.quantity;
                                {
                                    QListWidgetItem * newItem=new QListWidgetItem();
                                    if(quantity>1)
                                        newItem->setText(QString("Fight %1: %2x %3")
                                                      .arg(fightId)
                                                      .arg(quantity)
                                                      .arg(itemExtra.name)
                                                      );
                                    else
                                        newItem->setText(QString("Fight %1: %2")
                                                      .arg(fightId)
                                                      .arg(itemExtra.name)
                                                      );
                                    newItem->setIcon(QIcon(itemExtra.image));
                                    newItem->setToolTip(tips);
                                    itemToReturn.push_back(newItem->text().toStdString());
                                    if(listGUI==ui->globalTargets)
                                    {
                                        targetListGlobalTarget.push_back(globalTarget);
                                        if(alternateColor)
                                            newItem->setBackgroundColor(alternateColorValueL);
                                        else if(colorValueL.isValid())
                                            newItem->setBackgroundColor(colorValueL);
                                        newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
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
                                const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(monster.id);
                                {
                                    QListWidgetItem * newItem=new QListWidgetItem();
                                    newItem->setText(QString("Fight %1: %2 level %3")
                                            .arg(fightId)
                                            .arg(monsterExtra.name)
                                            .arg(monster.level)
                                            );
                                    newItem->setIcon(QIcon(monsterExtra.thumb));
                                    newItem->setToolTip(tips);
                                    itemToReturn.push_back(newItem->text().toStdString());
                                    if(listGUI==ui->globalTargets)
                                    {
                                        targetListGlobalTarget.push_back(globalTarget);
                                        if(alternateColor)
                                            newItem->setBackgroundColor(alternateColorValueL);
                                        else if(colorValueL.isValid())
                                            newItem->setBackgroundColor(colorValueL);
                                        newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
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
                    index++;
                }
            }
        }
        //shop
        {
            for(auto it = blockObject->shops.begin();it!=blockObject->shops.cend();++it)
            {
                const std::vector<uint32_t> &shops=it->second;
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

                    unsigned int sub_index=0;
                    while(sub_index<shop.prices.size())
                    {
                        const CATCHCHALLENGER_TYPE_ITEM &item=shop.items.at(sub_index);
                        const DatapackClientLoader::ItemExtra &itemExtra=DatapackClientLoader::datapackLoader.itemsExtra.value(item);
                        const uint32_t &price=shop.prices.at(sub_index);
                        {
                            const bool tooHard=price>player_private_and_public_informations.cash;
                            if(displayTooHard || !tooHard)
                            {
                                QListWidgetItem * newItem=new QListWidgetItem();
                                newItem->setText(QString("Shop %1: %2 %3$")
                                        .arg(shopId)
                                        .arg(itemExtra.name)
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
        //heal
        if(blockObject->heal.size()>0)
        {
            ActionsBotInterface::GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=0;
            globalTarget.bestPath=resolvedBlock.bestPath;
            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal;

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

        //the wild monster
        if(blockObject->monstersCollisionValue!=NULL)
        {
            const CatchChallenger::MonstersCollisionValue &monsterCollisionValue=*blockObject->monstersCollisionValue;
            if(!monsterCollisionValue.walkOnMonsters.empty())
            {
                /// \todo do the real code
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
                            if(bufferMonstersCollisionEntry.resolvedBlock.weight>resolvedBlock.weight)
                                bufferMonstersCollisionEntry.resolvedBlock=resolvedBlock;
                            break;
                        }
                        bufferMonstersCollisionIndex++;
                    }
                    //if not found append
                    if(bufferMonstersCollisionIndex>=bufferMonstersCollision.size())
                    {
                        BufferMonstersCollisionEntry bufferMonstersCollisionEntry;
                        bufferMonstersCollisionEntry.blockObject=blockObject;
                        bufferMonstersCollisionEntry.resolvedBlock=resolvedBlock;
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
            const MapServerMini::BlockObject * const blockObject=bufferMonstersCollisionEntry.blockObject;
            const MapServerMini::BlockObjectPathFinding &resolvedBlock=bufferMonstersCollisionEntry.resolvedBlock;
            const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=bufferMonstersCollisionEntry.bufferMonstersCollisionContent;

            ActionsBotInterface::GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=0;
            globalTarget.bestPath=resolvedBlock.bestPath;
            globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster;

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
            }

            if(displayTooHard || !tooHard)
            {
                unsigned int sub_index=0;
                while(sub_index<monsterCollisionContent.defaultMonsters.size())
                {
                    const CatchChallenger::MapMonster &mapMonster=monsterCollisionContent.defaultMonsters.at(sub_index);
                    const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(mapMonster.id);
                    {
                        QListWidgetItem * newItem=new QListWidgetItem();
                        newItem->setText(QString("Wild %2 level %3-%4, luck: %5")
                                .arg(monsterExtra.name)
                                .arg(mapMonster.minLevel)
                                .arg(mapMonster.maxLevel)
                                .arg(QString::number(mapMonster.luck)+"%")
                                );
                        newItem->setIcon(QIcon(monsterExtra.thumb));
                        newItem->setToolTip(tips);

                        itemToReturn.push_back(newItem->text().toStdString());
                        if(listGUI==ui->globalTargets)
                        {
                            targetListGlobalTarget.push_back(globalTarget);
                            if(alternateColor)
                                newItem->setBackgroundColor(alternateColorValueL);
                            else if(colorValueL.isValid())
                                newItem->setBackgroundColor(colorValueL);
                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
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
    return itemToReturn;
}

std::string BotTargetList::pathFindingToString(const MapServerMini::BlockObjectPathFinding &resolvedBlock)
{
    if(resolvedBlock.bestPath.empty())
        return "";
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
    str+=" ("+std::to_string(resolvedBlock.weight)+")";
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
