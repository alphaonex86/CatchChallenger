#include "BaseWindow.h"
#include "GetPrice.h"
#include "ui_BaseWindow.h"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

#include <QInputDialog>
#include <iostream>

using namespace CatchChallenger;

//return ok, itemId
void BaseWindow::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    switch(objectType)
    {
        case ObjectType_Seed:
            ui->stackedWidget->setCurrentWidget(ui->page_plants);
            on_listPlantList_itemSelectionChanged();
        break;
        case ObjectType_Sell:
            ui->stackedWidget->setCurrentWidget(ui->page_shop);
            displaySellList();
        break;
        case ObjectType_MonsterToTrade:
        case ObjectType_MonsterToLearn:
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
            ui->selectMonster->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_monster);
            load_monsters();
        break;
        case ObjectType_All:
        case ObjectType_Trade:
            ui->inventoryUse->setText(tr("Select"));
            ui->inventoryUse->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_inventory);
            on_listCraftingList_itemSelectionChanged();
        break;
        case ObjectType_UseInFight:
            ui->inventoryUse->setText(tr("Select"));
            ui->inventoryUse->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_inventory);
            load_inventory();
        break;
        default:
            emit error("unknown selection type");
        return;
    }
}

void BaseWindow::objectSelection(const bool &ok, const uint16_t &itemId, const uint32_t &quantity)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    inSelection=false;
    ObjectType tempWaitedObjectType=waitedObjectType;
    waitedObjectType=ObjectType_All;
    switch(tempWaitedObjectType)
    {
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        {
            const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
            const uint16_t item=objectInUsing.back();
            objectInUsing.erase(objectInUsing.cbegin());
            if(!ok)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_inventory);
                ui->inventoryUse->setText(tr("Select"));
                if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().item.cend())
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(item).consumeAtUse)
                        add_to_inventory(item,1,false);
                break;
            }
            const PlayerMonster * const monster=client->monsterByPosition(monsterPosition);
            if(monster==NULL)
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().item.cend())
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(item).consumeAtUse)
                        add_to_inventory(item,1,false);
                break;
            }
            const Monster &monsterInformations=CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
            const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster->monster);
            if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.cend())
            {
                monsterEvolutionPostion=0;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.get_monsters().at(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(item).at(monster->monster));
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=
                        QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.at(item).at(monster->monster)
                            );
                //create animation widget
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                client->useObjectOnMonsterByPosition(item,monsterPosition);
                if(!client->useObjectOnMonsterByPosition(item,monsterPosition))
                {
                    std::cerr << "client->useObjectOnMonsterByPosition() Bug at " << __FILE__ << ":" << __LINE__ << std::endl;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    abort();
                    #endif
                }
                load_monsters();
                return;
            }
            else
            {
                ui->stackedWidget->setCurrentWidget(ui->page_inventory);
                ui->inventoryUse->setText(tr("Select"));
                if(client->useObjectOnMonsterByPosition(item,monsterPosition))
                {
                    showTip(tr("Using <b>%1</b> on <b>%2</b>")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(item).name))
                            .arg(QString::fromStdString(monsterInformationsExtra.name))
                            .toStdString());
                    client->useObjectOnMonsterByPosition(item,monsterPosition);
                    load_monsters();
                    checkEvolution();
                }
                else
                {
                    showTip(tr("Failed to use <b>%1</b> on <b>%2</b>")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(item).name))
                            .arg(QString::fromStdString(monsterInformationsExtra.name))
                            .toStdString());
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.find(item)!=
                            CatchChallenger::CommonDatapack::commonDatapack.get_items().item.cend())
                        if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(item).consumeAtUse)
                            add_to_inventory(item,1,false);
                }
            }
        }
        break;
        case ObjectType_Sell:
        {
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            if(!ok)
                break;
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(playerInformations.items.at(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            remove_to_inventory(itemId,quantity);
            ItemToSellOrBuy tempItem;
            tempItem.object=itemId;
            tempItem.quantity=quantity;
            tempItem.price=CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(itemId).price/2;
            itemsToSell.push_back(tempItem);
            client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        }
        break;
        case ObjectType_Trade:
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(playerInformations.items.at(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            client->addObject(itemId,quantity);
            playerInformations.items[itemId]-=quantity;
            if(playerInformations.items.at(itemId)==0)
                playerInformations.items.erase(itemId);
            if(tradeCurrentObjects.find(itemId)!=tradeCurrentObjects.cend())
                tradeCurrentObjects[itemId]+=quantity;
            else
                tradeCurrentObjects[itemId]=quantity;
            load_inventory();
            load_plant_inventory();
            tradeUpdateCurrentObject();
        break;
        case ObjectType_MonsterToLearn:
        {
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            load_monsters();
            if(!ok)
                return;
            ui->stackedWidget->setCurrentWidget(ui->page_learn);
            monsterPositionToLearn=static_cast<uint8_t>(itemId);
            if(!showLearnSkillByPosition(monsterPositionToLearn))
            {
                newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Unable to load the right monster");
                return;
            }
        }
        break;
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_battle);
            load_monsters();
            if(!ok)
                return;
            resetPosition(true,false,true);
            //do copie here because the call of changeOfMonsterInFight apply the skill/buff effect
            const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
            const PlayerMonster *tempMonster=client->monsterByPosition(monsterPosition);
            if(tempMonster==NULL)
            {
                qDebug() << "Monster not found";
                return;
            }
            PlayerMonster copiedMonster=*tempMonster;
            if(!client->changeOfMonsterInFight(monsterPosition))
                return;
            client->changeOfMonsterInFightByPosition(monsterPosition);
            PlayerMonster * playerMonster=client->getCurrentMonster();
            init_current_monster_display(&copiedMonster);
            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
            if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(playerMonster->monster)!=
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
            {
                ui->labelFightEnter->setText(tr("Go %1")
                                             .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(playerMonster->monster).name)));
                ui->labelFightMonsterBottom->setPixmap(QtDatapackClientLoader::datapackLoader->getMonsterExtra(playerMonster->monster).back.scaled(160,160));
            }
            else
            {
                ui->labelFightEnter->setText(tr("You change of monster"));
                ui->labelFightMonsterBottom->setPixmap(QPixmap(":/images/monsters/default/back.png"));
            }
            ui->pushButtonFightEnterNext->setVisible(false);
            moveType=MoveType_Enter;
            battleStep=BattleStep_Presentation;
            monsterBeforeMoveForChangeInWaiting=true;
            moveFightMonsterBottom();
        }
        break;
        case ObjectType_MonsterToTrade:
        {
            ui->inventoryUse->setText(tr("Select"));
            load_monsters();
            const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
            if(waitedObjectType==ObjectType_MonsterToLearn)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_learn);
                monsterPositionToLearn=monsterPosition;
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            std::vector<PlayerMonster> playerMonster=client->getPlayerMonster();
            if(playerMonster.size()<=1)
            {
                QMessageBox::warning(this,tr("Warning"),tr("You can't trade your last monster"));
                break;
            }
            if(!client->remainMonstersToFightWithoutThisMonster(monsterPosition))
            {
                QMessageBox::warning(this,tr("Warning"),tr("You don't have more monster valid"));
                break;
            }
            //get the right monster
            tradeCurrentMonstersPosition.push_back(monsterPosition);
            tradeCurrentMonsters.push_back(playerMonster.at(monsterPosition));
            client->removeMonsterByPosition(monsterPosition);
            client->addMonsterByPosition(monsterPosition);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(tradeCurrentMonsters.back().monster).name));
            item->setToolTip(tr("Level: %1").arg(tradeCurrentMonsters.back().level));
            item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(tradeCurrentMonsters.back().monster).front);
            ui->tradePlayerMonsters->addItem(item);
        }
        break;
        case ObjectType_Seed:
        {
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            ui->plantUse->setVisible(false);
            if(!ok)
            {
                seed_in_waiting.pop_back();
                break;
            }
            if(QtDatapackClientLoader::datapackLoader->get_itemToPlants().find(itemId)==
                    QtDatapackClientLoader::datapackLoader->get_itemToPlants().cend())
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(this,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                seed_in_waiting.pop_back();
                return;
            }
            const uint8_t &plantId=QtDatapackClientLoader::datapackLoader->get_itemToPlants().at(itemId);
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plantId).requirements.reputation))
            {
                qDebug() << "You don't have the requirements to plant the seed";
                QMessageBox::critical(this,tr("Error"),tr("You don't have the requirements to plant the seed"));
                seed_in_waiting.pop_back();
                return;
            }
            if(havePlant(&mapController->getMap(mapController->current_map)->logicalMap,mapController->getX(),mapController->getY())>=0)
            {
                qDebug() << "Too slow to select a seed, have plant now";
                showTip(tr("Sorry, but now the dirt is not free to plant").toStdString());
                seed_in_waiting.pop_back();
                return;
            }
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
            {
                qDebug() << "item id is not into the inventory";
                seed_in_waiting.pop_back();
                break;
            }
            remove_to_inventory(itemId);

            const SeedInWaiting seedInWaiting=seed_in_waiting.back();
            seed_in_waiting.back().seedItemId=itemId;
            insert_plant(mapController->getMap(seedInWaiting.map)->logicalMap.id,
                         seedInWaiting.x,seedInWaiting.y,plantId,
                         static_cast<uint16_t>(CommonDatapack::commonDatapack.get_plants().at(plantId).fruits_seconds)
                         );
            addQuery(QueryType_Seed);
            load_plant_inventory();
            load_inventory();
            qDebug() << QStringLiteral("send seed for: %1").arg(plantId);
            emit useSeed(plantId);
            //if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
            {
                //debug
#ifdef CATCHCHALLENGER_EXTRA_CHECK
                {
                    std::unordered_set<int> detectDuplicate;
                    Map_full * m=mapController->getMap(seedInWaiting.map);
                    std::cerr << "debug: seedInWaiting.map: " << seedInWaiting.map << " mapController->getMap(seedInWaiting.map)->logicalMap.id: " << m->logicalMap.id << std::endl;
                    std::cerr << "debug: map list: " << std::endl;
                    unsigned int index=0;
                    while(index<QtDatapackClientLoader::datapackLoader->get_maps().size())
                    {
                        const std::string m=QtDatapackClientLoader::datapackLoader->getDatapackPath()+QtDatapackClientLoader::datapackLoader->getMainDatapackPath()+QtDatapackClientLoader::datapackLoader->get_maps().at(index);
                        if(mapController->all_map.find(m)!=mapController->all_map.cend())
                        {
                            if(detectDuplicate.find(mapController->all_map.at(m)->logicalMap.id)!=detectDuplicate.cend())
                            {
                                std::cerr << "duplicate: " << mapController->all_map.at(m)->logicalMap.id << std::endl;
                                std::cerr << "need match id mapController->all_map: " << std::endl;
                                for (auto& it: mapController->all_map)
                                {
                                    const Map_full * m2=it.second;
                                    std::cout << it.first << ": " << m2->logicalMap.id << std::endl;
                                }
                                std::cerr << "need match id QtDatapackClientLoader::datapackLoader->get_maps(): " << std::endl;
                                unsigned int index=0;
                                while(index<QtDatapackClientLoader::datapackLoader->get_maps().size())
                                {
                                    std::cout << QtDatapackClientLoader::datapackLoader->get_maps().at(index) << ": " << index << std::endl;
                                    index++;
                                }
                                abort();
                            }
                            detectDuplicate.insert(mapController->all_map.at(m)->logicalMap.id);

                            if(mapController->all_map.find(m)==mapController->all_map.cend())
                            {
                                std::cerr << "all_map.find(m)==all_map.cend() with: " << m << std::endl;
                                for (auto& it: mapController->all_map)
                                    std::cout << it.first << ": " << it.second << std::endl;
                                abort();
                            }
                            if(index!=mapController->all_map.at(m)->logicalMap.id)
                            {
                                std::cerr << "index!=all_map.at(m)->logicalMap.id with: " << m << ", " << index << ", " << mapController->all_map.at(m)->logicalMap.id << std::endl;
                                std::cerr << "need match id mapController->all_map: " << std::endl;
                                for (auto& it: mapController->all_map)
                                {
                                    const Map_full * m2=it.second;
                                    std::cout << it.first << ": " << m2->logicalMap.id << std::endl;
                                }
                                std::cerr << "need match id QtDatapackClientLoader::datapackLoader->get_maps(): " << std::endl;
                                unsigned int index=0;
                                while(index<QtDatapackClientLoader::datapackLoader->get_maps().size())
                                {
                                    std::cout << QtDatapackClientLoader::datapackLoader->get_maps().at(index) << ": " << index << std::endl;
                                    index++;
                                }
                                abort();
                            }
                            std::cerr << m << " " << __FILE__ << ":" << __LINE__ << std::endl;
                        }
                        else
                            std::cerr << m << " not into loaded map! " << __FILE__ << ":" << __LINE__ << std::endl;
                        index++;
                    }
                }
                if(QtDatapackClientLoader::datapackLoader->get_fullMapPathToId().find(seedInWaiting.map)!=QtDatapackClientLoader::datapackLoader->get_fullMapPathToId().cend())
                {
                    if(QtDatapackClientLoader::datapackLoader->get_fullMapPathToId().at(seedInWaiting.map)!=mapController->getMap(seedInWaiting.map)->logicalMap.id)
                    {
                        std::cerr << __FILE__ << ":" << __LINE__ << " data corrupted (abort)" << std::endl;
                        abort();
                    }
                }
                else
                {
                    //check duplicate
                    std::unordered_set<int> detectDuplicate;
                    for (auto& it: mapController->all_map)
                    {
                        const Map_full * m2=it.second;
                        if(detectDuplicate.find(m2->logicalMap.id)!=detectDuplicate.cend())
                        {
                            std::cerr << "duplicate mapController->all_map to use as id" << std::endl;
                            abort();
                        }
                        detectDuplicate.insert(m2->logicalMap.id);
                    }
                }
                #endif
                client->seed_planted(true);
                client->insert_plant(mapController->getMap(seedInWaiting.map)->logicalMap.id,
                                     seedInWaiting.x,seedInWaiting.y,plantId,
                                     static_cast<uint16_t>(CommonDatapack::commonDatapack.get_plants().at(plantId).fruits_seconds)
                                     );
            }
        }
        break;
        case ObjectType_UseInFight:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_battle);
            load_inventory();
            if(!ok)
                break;
            const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
            {
                QMessageBox::warning(this,tr("Error"),tr("You have already the maximum number of monster into you warehouse"));
                break;
            }
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(playerInformations.items.at(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            //it's trap
            if(CommonDatapack::commonDatapack.get_items().trap.find(itemId)!=CommonDatapack::commonDatapack.get_items().trap.cend() && client->isInFightWithWild())
            {
                remove_to_inventory(itemId);
                useTrap(itemId);
            }
            else//else it's to use on current monster
            {
                const uint8_t &monsterPosition=client->getCurrentSelectedMonsterNumber();
                if(client->useObjectOnMonsterByPosition(itemId,monsterPosition))
                {
                    remove_to_inventory(itemId);
                    if(CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(itemId)!=CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend())
                    {
                        client->useObjectOnMonsterByPosition(itemId,monsterPosition);
                        updateAttackList();
                        displayAttackProgression=0;
                        attack_quantity_changed=0;
                        if(battleType!=BattleType_OtherPlayer)
                            doNextAction();
                        else
                        {
                            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                            ui->labelFightEnter->setText(tr("In waiting of the other player action"));
                            ui->pushButtonFightEnterNext->hide();
                        }
                    }
                    else
                        error(tr("You have selected a buggy object").toStdString());
                }
                else
                    QMessageBox::warning(this,tr("Warning"),tr("Can't be used now!"));
            }

        }
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
    waitedObjectType=ObjectType_All;
}

void BaseWindow::on_inventory_itemActivated(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(items_graphical.find(item)==items_graphical.cend())
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    const uint16_t &itemId=items_graphical.at(item);
    if(inSelection)
    {
        uint32_t tempQuantitySelected;
        bool ok=true;
        switch(waitedObjectType)
        {
            case ObjectType_Sell:
            case ObjectType_Trade:
                if(playerInformations.items.at(itemId)>1)
                    tempQuantitySelected=QInputDialog::getInt(this,tr("Quantity"),tr("Select a quantity"),1,1,playerInformations.items.at(itemId),1,&ok);
                else
                    tempQuantitySelected=1;
            break;
            default:
                tempQuantitySelected=1;
            break;
        }
        if(!ok)
        {
            objectSelection(false);
            return;
        }
        if(playerInformations.items.find(itemId)==playerInformations.items.cend())
        {
            objectSelection(false);
            return;
        }
        if(playerInformations.items.at(itemId)<tempQuantitySelected)
        {
            objectSelection(false);
            return;
        }
        uint16_t objectItem=itemId;
        objectSelection(true,objectItem,tempQuantitySelected);
        return;
    }

    //is crafting recipe
    if(CatchChallenger::CommonDatapack::commonDatapack.get_itemToCraftingRecipes().find(itemId)!=
            CatchChallenger::CommonDatapack::commonDatapack.get_itemToCraftingRecipes().cend())
    {
        Player_private_and_public_informations informations=client->get_player_informations();
        const uint16_t &recipeId=CatchChallenger::CommonDatapack::commonDatapack.get_itemToCraftingRecipes().at(itemId);

        //check if have the requirements
        const CraftingRecipe &recipe=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipes().at(recipeId);
        if(!haveReputationRequirements(recipe.requirements.reputation))
        {
            std::string string;
            unsigned int index=0;
            while(index<recipe.requirements.reputation.size())
            {
                string+=reputationRequirementsToText(recipe.requirements.reputation.at(index));
                index++;
            }
            QMessageBox::information(this,tr("Information"),tr("You don't he the reputation requirements: %1").arg(QString::fromStdString(string)));
            return;
        }

        if(informations.recipes[recipeId/8] & (1<<(7-recipeId%8)))
        {
            QMessageBox::information(this,tr("Information"),tr("You already know this recipe"));
            return;
        }
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        client->useObject(objectInUsing.back());
    }
    //it's repel
    else if(CatchChallenger::CommonDatapack::commonDatapack.get_items().repel.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().repel.cend())
    {
        mapController->addRepelStep(CatchChallenger::CommonDatapack::commonDatapack.get_items().repel.at(itemId));
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        client->useObject(objectInUsing.back());
    }
    //it's object with monster effect
    else if(CatchChallenger::CommonDatapack::commonDatapack.get_items().monsterItemEffect.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().monsterItemEffect.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemOnMonster);
    }
    //it's object with monster effect but offline
    else if(CatchChallenger::CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().monsterItemEffectOutOfFight.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().evolutionItem.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemEvolutionOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.get_items().itemToLearn.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.get_items().item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemLearnOnMonster);
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}
