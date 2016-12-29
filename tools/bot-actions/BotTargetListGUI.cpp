#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

std::vector<std::string> BotTargetList::contentToGUI(const MapServerMini::BlockObject * const blockObject, QListWidget *listGUI)
{
    std::vector<std::string> itemToReturn;
    if(listGUI==ui->localTargets)
        mapIdListLocalTarget.clear();
    QColor alternateColorValue(230,230,230,255);
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
                                    newItem->setText(QString("Teleporter to %1").arg(QString::fromStdString(nextBlock->map->map_file)));
                                break;
                                case MapServerMini::BlockObject::LinkType::SourceTopMap:
                                    newItem->setText(QString("Top border to %1").arg(QString::fromStdString(nextBlock->map->map_file)));
                                break;
                                case MapServerMini::BlockObject::LinkType::SourceRightMap:
                                    newItem->setText(QString("Right border to %1").arg(QString::fromStdString(nextBlock->map->map_file)));
                                break;
                                case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                                    newItem->setText(QString("Bottom border to %1").arg(QString::fromStdString(nextBlock->map->map_file)));
                                break;
                                case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                                    newItem->setText(QString("Left border to %1").arg(QString::fromStdString(nextBlock->map->map_file)));
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
                GlobalTarget globalTarget;
                globalTarget.blockObject=blockObject;
                globalTarget.extra=itemOnMap.item;
                globalTarget.type=GlobalTarget::GlobalTargetType::ItemOnMap;
                targetListGlobalTarget.push_back(globalTarget);
                if(alternateColor)
                    newItem->setBackgroundColor(alternateColorValue);
                alternateColor=!alternateColor;
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
            GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=fightId;
            globalTarget.type=GlobalTarget::GlobalTargetType::Fight;

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
            GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=shopId;
            globalTarget.type=GlobalTarget::GlobalTargetType::Shop;

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
        GlobalTarget globalTarget;
        globalTarget.blockObject=blockObject;
        globalTarget.extra=0;
        globalTarget.type=GlobalTarget::GlobalTargetType::Heal;

        QListWidgetItem * newItem=new QListWidgetItem();
        newItem->setText(QString("Heal"));
        newItem->setIcon(QIcon(":/1.png"));

        itemToReturn.push_back(newItem->text().toStdString());
        if(listGUI==ui->globalTargets)
        {
            targetListGlobalTarget.push_back(globalTarget);
            if(alternateColor)
                newItem->setBackgroundColor(alternateColorValue);
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
        /// \todo do the real code
        //choice the first entry
        const CatchChallenger::MonstersCollisionValue &monsterCollisionValue=*blockObject->monstersCollisionValue;
        if(!monsterCollisionValue.walkOnMonsters.empty())
        {
            const CatchChallenger::MonstersCollisionValue::MonstersCollisionContent &monsterCollisionContent=monsterCollisionValue.walkOnMonsters.at(0);
            GlobalTarget globalTarget;
            globalTarget.blockObject=blockObject;
            globalTarget.extra=0;
            globalTarget.type=GlobalTarget::GlobalTargetType::WildMonster;

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
                    }
                    if(listGUI!=NULL)
                        listGUI->addItem(newItem);
                    else
                        delete newItem;
                }
                sub_index++;
            }
            if(!monsterCollisionContent.defaultMonsters.empty())
                if(listGUI==ui->globalTargets)
                    alternateColor=!alternateColor;
        }
    }
    return itemToReturn;
}
