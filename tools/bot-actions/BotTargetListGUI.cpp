#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

std::vector<std::string> BotTargetList::contentToGUI(const MapServerMini::BlockObject * const blockObject, QListWidget *listGUI)
{
    std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> resolvedBlockList;
    MapServerMini::BlockObjectPathFinding pathFinding;
    pathFinding.weight=0;
    resolvedBlockList[blockObject]=pathFinding;
    return contentToGUI(listGUI,resolvedBlockList);
}

std::vector<std::string> BotTargetList::contentToGUI(QListWidget *listGUI,
                                                     const std::unordered_map<const MapServerMini::BlockObject *, MapServerMini::BlockObjectPathFinding> &resolvedBlockList)
{
    if(listGUI==ui->localTargets)
        mapIdListLocalTarget.clear();
    std::vector<std::string> itemToReturn;
    QColor alternateColorValue(230,230,230,255);

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
        if(listGUI==ui->localTargets)
        {
            for(const auto& n:blockObject->links) {
                const MapServerMini::BlockObject * const nextBlock=n.first;
                const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                if(nextBlock->map!=blockObject->map)
                {
                    unsigned int index=0;
                    while(index<linkInformation.types.size())
                    {
                        const MapServerMini::BlockObject::LinkType &type=linkInformation.types.at(index);
                        switch(type)
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
                                switch(type)
                                {
                                    case MapServerMini::BlockObject::LinkType::SourceTeleporter:
                                        newItem->setText(QString("Teleporter to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkInformation.x).arg(linkInformation.y));
                                    break;
                                    case MapServerMini::BlockObject::LinkType::SourceTopMap:
                                        newItem->setText(QString("Top border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkInformation.x).arg(linkInformation.y));
                                    break;
                                    case MapServerMini::BlockObject::LinkType::SourceRightMap:
                                        newItem->setText(QString("Right border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkInformation.x).arg(linkInformation.y));
                                    break;
                                    case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                                        newItem->setText(QString("Bottom border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkInformation.x).arg(linkInformation.y));
                                    break;
                                    case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                                        newItem->setText(QString("Left border to %1, go to %2,%3").arg(QString::fromStdString(nextBlock->map->map_file)).arg(linkInformation.x).arg(linkInformation.y));
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

        //item on map
        {
            unsigned int index=0;
            while(index<blockObject->pointOnMap_Item.size())
            {
                const MapServerMini::ItemOnMap &itemOnMap=blockObject->pointOnMap_Item.at(index);
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
                index++;
            }
        }
        //fight
        {
            unsigned int index=0;
            while(index<blockObject->botsFight.size())
            {
                const uint32_t &fightId=blockObject->botsFight.at(index);
                const CatchChallenger::BotFight &fight=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId);
                ActionsBotInterface::GlobalTarget globalTarget;
                globalTarget.blockObject=blockObject;
                globalTarget.extra=fightId;
                globalTarget.bestPath=resolvedBlock.bestPath;
                globalTarget.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight;

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
                            itemToReturn.push_back(newItem->text().toStdString());
                            if(listGUI==ui->globalTargets)
                            {
                                targetListGlobalTarget.push_back(globalTarget);
                                if(alternateColor)
                                    newItem->setBackgroundColor(alternateColorValue);
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
                            itemToReturn.push_back(newItem->text().toStdString());
                            if(listGUI==ui->globalTargets)
                            {
                                targetListGlobalTarget.push_back(globalTarget);
                                if(alternateColor)
                                    newItem->setBackgroundColor(alternateColorValue);
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
                index++;
            }
        }
        //shop
        {
            unsigned int index=0;
            while(index<blockObject->shops.size())
            {
                const uint32_t &shopId=blockObject->shops.at(index);
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
                            if(alternateColor)
                                newItem->setBackgroundColor(alternateColorValue);
                            newItem->setText(newItem->text()+QString::fromStdString(pathFindingToString(resolvedBlock)));
                        }
                        if(listGUI!=NULL)
                            listGUI->addItem(newItem);
                        else
                            delete newItem;
                    }
                    sub_index++;
                }
                if(!shop.prices.empty())
                    if(listGUI==ui->globalTargets)
                        alternateColor=!alternateColor;
                index++;
            }
        }
        //heal
        if(blockObject->heal)
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

                    itemToReturn.push_back(newItem->text().toStdString());
                    if(listGUI==ui->globalTargets)
                    {
                        targetListGlobalTarget.push_back(globalTarget);
                        if(alternateColor)
                            newItem->setBackgroundColor(alternateColorValue);
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
