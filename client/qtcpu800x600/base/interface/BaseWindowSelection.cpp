#include "BaseWindow.h"
#include "GetPrice.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

#include <QQmlContext>
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
        case ObjectType_MonsterToTradeToMarket:
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
        case ObjectType_SellToMarket:
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
                if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                        add_to_inventory(item,1,false);
                break;
            }
            const PlayerMonster * const monster=client->monsterByPosition(monsterPosition);
            if(monster==NULL)
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                        add_to_inventory(item,1,false);
                break;
            }
            const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster->monster);
            const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster);
            if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
            {
                monsterEvolutionPostion=0;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.at(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(item).at(monster->monster));
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=
                        QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                            CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(item).at(monster->monster)
                            );
                //create animation widget
                if(animationWidget!=NULL)
                    delete animationWidget;
                if(qQuickViewContainer!=NULL)
                    delete qQuickViewContainer;
                animationWidget=new QQuickView();
                qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
                qQuickViewContainer->setMinimumSize(QSize(800,600));
                qQuickViewContainer->setMaximumSize(QSize(800,600));
                qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
                ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                if(baseMonsterEvolution!=NULL)
                    delete baseMonsterEvolution;
                if(targetMonsterEvolution!=NULL)
                    delete targetMonsterEvolution;
                baseMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformations,monsterInformationsExtra);
                targetMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                if(evolutionControl!=NULL)
                    delete evolutionControl;
                evolutionControl=new EvolutionControl(monsterInformations,monsterInformationsExtra,monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(false));
                animationWidget->rootContext()->setContextProperty("itemEvolution",
                                                                   QUrl::fromLocalFile(
                                                                       QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra
                                                                                              .at(item).imagePath)));
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const std::string datapackQmlFile=client->datapackPathBase()+"qml/evolution-animation.qml";
                if(QFile(QString::fromStdString(datapackQmlFile)).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(QString::fromStdString(datapackQmlFile)));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
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
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(item).name))
                            .arg(QString::fromStdString(monsterInformationsExtra.name))
                            .toStdString());
                    client->useObjectOnMonsterByPosition(item,monsterPosition);
                    load_monsters();
                    checkEvolution();
                }
                else
                {
                    showTip(tr("Failed to use <b>%1</b> on <b>%2</b>")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(item).name))
                            .arg(QString::fromStdString(monsterInformationsExtra.name))
                            .toStdString());
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.item.find(item)!=
                            CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
                        if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
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
            tempItem.price=CatchChallenger::CommonDatapack::commonDatapack.items.item.at(itemId).price/2;
            itemsToSell.push_back(tempItem);
            client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        }
        break;
        case ObjectType_SellToMarket:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_market);
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
            uint32_t suggestedPrice=50;
            if(CommonDatapack::commonDatapack.items.item.find(itemId)!=CommonDatapack::commonDatapack.items.item.cend())
                suggestedPrice=CommonDatapack::commonDatapack.items.item.at(itemId).price;
            GetPrice getPrice(this,suggestedPrice);
            getPrice.exec();
            if(!getPrice.isOK())
                break;
            client->putMarketObject(itemId,quantity,getPrice.price());
            marketPutCashInSuspend=getPrice.price();
            remove_to_inventory(itemId,quantity);
            std::pair<uint16_t,uint32_t> pair;
            pair.first=itemId;
            pair.second=quantity;
            marketPutObjectInSuspendList.push_back(pair);
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
            if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(playerMonster->monster)!=
                    QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
            {
                ui->labelFightEnter->setText(tr("Go %1")
                                             .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(playerMonster->monster).name)));
                ui->labelFightMonsterBottom->setPixmap(QPixmap::fromImage(QtDatapackClientLoader::datapackLoader->ImagemonsterExtra.at(playerMonster->monster).back).scaled(160,160));
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
        case ObjectType_MonsterToTradeToMarket:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_market);
            load_monsters();
            if(!ok)
                break;
            std::vector<PlayerMonster> playerMonster=client->getPlayerMonster();
            if(playerMonster.size()<=1)
            {
                QMessageBox::warning(this,tr("Warning"),tr("You can't trade your last monster"));
                break;
            }
            const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
            if(!client->remainMonstersToFightWithoutThisMonster(monsterPosition))
            {
                QMessageBox::warning(this,tr("Warning"),tr("You don't have more monster valid"));
                break;
            }
            //get the right monster
            GetPrice getPrice(this,15000);
            getPrice.exec();
            if(!getPrice.isOK())
                break;
            marketPutMonsterList.push_back(playerMonster.at(monsterPosition));
            marketPutMonsterPlaceList.push_back(monsterPosition);
            client->removeMonsterByPosition(monsterPosition);
            client->putMarketMonsterByPosition(monsterPosition,getPrice.price());
            marketPutCashInSuspend=getPrice.price();
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
            item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(tradeCurrentMonsters.back().monster).name));
            item->setToolTip(tr("Level: %1").arg(tradeCurrentMonsters.back().level));
            item->setIcon(QPixmap::fromImage(QtDatapackClientLoader::datapackLoader->ImagemonsterExtra.at(tradeCurrentMonsters.back().monster).front));
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
            if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(itemId)==
                    QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
            {
                qDebug() << "Item is not a plant";
                QMessageBox::critical(this,tr("Error"),tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__));
                seed_in_waiting.pop_back();
                return;
            }
            const uint8_t &plantId=QtDatapackClientLoader::datapackLoader->itemToPlants.at(itemId);
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
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
                         static_cast<uint16_t>(CommonDatapack::commonDatapack.plants.at(plantId).fruits_seconds)
                         );
            addQuery(QueryType_Seed);
            load_plant_inventory();
            load_inventory();
            qDebug() << QStringLiteral("send seed for: %1").arg(plantId);
            emit useSeed(plantId);
            //if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
            {
                client->seed_planted(true);
                client->insert_plant(mapController->getMap(seedInWaiting.map)->logicalMap.id,
                                     seedInWaiting.x,seedInWaiting.y,plantId,
                                     static_cast<uint16_t>(CommonDatapack::commonDatapack.plants.at(plantId).fruits_seconds)
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
            if(CommonDatapack::commonDatapack.items.trap.find(itemId)!=CommonDatapack::commonDatapack.items.trap.cend() && client->isInFightWithWild())
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
                    if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(itemId)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
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
            case ObjectType_SellToMarket:
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
    if(CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes.cend())
    {
        Player_private_and_public_informations informations=client->get_player_informations();
        const uint16_t &recipeId=CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes.at(itemId);

        //check if have the requirements
        const CraftingRecipe &recipe=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(recipeId);
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
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        client->useObject(objectInUsing.back());
    }
    //it's repel
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.repel.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.items.repel.cend())
    {
        mapController->addRepelStep(CatchChallenger::CommonDatapack::commonDatapack.items.repel.at(itemId));
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        client->useObject(objectInUsing.back());
    }
    //it's object with monster effect
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemOnMonster);
    }
    //it's object with monster effect but offline
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemEvolutionOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.find(itemId)!=CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.cend())
    {
        objectInUsing.push_back(itemId);
        if(CommonDatapack::commonDatapack.items.item.at(objectInUsing.back()).consumeAtUse)
            remove_to_inventory(objectInUsing.back());
        selectObject(ObjectType_ItemLearnOnMonster);
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}
