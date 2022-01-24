#include "BaseWindow.hpp"
#include "ui_BaseWindow.h"
#include "../LanguagesSelect.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../fight/interface/ClientFightEngine.hpp"

#include <QDesktopServices>
#include <QInputDialog>
#include <QString>
#include <iostream>

using namespace CatchChallenger;

bool BaseWindow::botHaveQuest(const uint16_t &botId) const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    //do the not started quest here
    if(QtDatapackClientLoader::datapackLoader->botToQuestStart.find(botId)==
            QtDatapackClientLoader::datapackLoader->botToQuestStart.cend())
    {
        std::cerr << "BaseWindow::botHaveQuest(), botId not found: " << std::to_string(botId) << std::endl;
        return false;
    }
    const std::vector<uint16_t> &botQuests=QtDatapackClientLoader::datapackLoader->botToQuestStart.at(botId);
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint16_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at BaseWindow::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
        if(playerInformations.quests.find(botQuests.at(index))==playerInformations.quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
                return true;
            else
                {}//have not the requirement
        }
        else
        {
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(botQuests.at(index))==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(playerInformations.quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(playerInformations.quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                                return true;
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    const std::vector<uint16_t> &bots=currentQuest.steps.at(playerInformations.quests.at(questId).step-1).bots;
                    if(vectorcontainsAtLeastOne(bots,botId))
                        return true;//in progress
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    auto i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        if(!vectorcontainsAtLeastOne(botQuests,i->first) && i->second.step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(i->first);
            auto bots=currentQuest.steps.at(i->second.step-1).bots;
            if(vectorcontainsAtLeastOne(bots,botId))
                return true;//in progress, but not the starting bot
            else
                {}//it's another bot
        }
        ++i;
    }
    return false;
}

//bot
void BaseWindow::goToBotStep(const uint8_t &step)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    lastStepUsed=step;
    isInQuest=false;
    if(actualBot.step.find(step)==actualBot.step.cend())
    {
        showTip(tr("Error into the bot, repport this error please").toStdString());
        return;
    }
    const tinyxml2::XMLElement * stepXml=actualBot.step.at(step);
    if(strcmp(stepXml->Attribute("type"),"text")==0)
    {
        #ifndef CATCHCHALLENGER_BOT
        const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        #else
        const std::string language("en");
        #endif
        auto text = stepXml->FirstChildElement("text");
        if(!language.empty() && language!=BaseWindow::text_en)
            while(text!=NULL)
            {
                if(text->Attribute("lang")!=NULL && text->Attribute("lang")==language && text->GetText()!=NULL)
                {
                    std::string textToShow=text->GetText();
                    textToShow=parseHtmlToDisplay(textToShow);
                    ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
                    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
                    ui->IG_dialog->setVisible(true);
                    return;
                }
                text = text->NextSiblingElement("text");
            }
        text = stepXml->FirstChildElement("text");
        while(text!=NULL)
        {
            if(text->Attribute("lang")==NULL || text->Attribute("lang")==language)
                if(text->GetText()!=NULL)
                {
                    std::string textToShow=text->GetText();
                    textToShow=parseHtmlToDisplay(textToShow);
                    ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
                    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
                    ui->IG_dialog->setVisible(true);
                    return;
                }
            text = text->NextSiblingElement("text");
        }
        showTip(tr("Bot text not found, repport this error please").toStdString());
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"shop")==0)
    {
        if(stepXml->Attribute("shop")==NULL)
        {
            showTip(tr("Shop called but missing informations").toStdString());
            return;
        }
        bool ok;
        shopId=stringtouint16(stepXml->Attribute("shop"),&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id").toStdString());
            return;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.cend())
        {
            showTip(tr("Shop not found").toStdString());
            return;
        }
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin(QString::fromStdString(getFrontSkinPath(actualBot.properties.at("skin"))));
            if(!skin.isNull())
            {
                ui->shopSellerImage->setPixmap(skin.scaled(160,160));
                ui->shopSellerImage->setVisible(true);
            }
            else
                ui->shopSellerImage->setVisible(false);
        }
        else
            ui->shopSellerImage->setVisible(false);
        ui->stackedWidget->setCurrentWidget(ui->page_shop);
        ui->shopItemList->clear();
        on_shopItemList_itemSelectionChanged();
        #ifndef CATCHCHALLENGER_CLIENT_INSTANT_SHOP
        ui->shopDescription->setText(tr("Waiting the shop content"));
        ui->shopBuy->setVisible(false);
        qDebug() << "goToBotStep(), client->getShopList(shopId): " << shopId;
        client->getShopList(shopId);
        #else
        {
            std::vector<ItemToSellOrBuy> items;
            const Shop &shop=CommonDatapackServerSpec::commonDatapackServerSpec.shops.at(shopId);
            unsigned int index=0;
            while(index<shop.items.size())
            {
                if(shop.prices.at(index)>0)
                {
                    ItemToSellOrBuy newItem;
                    newItem.object=shop.items.at(index);
                    newItem.price=shop.prices.at(index);
                    newItem.quantity=0;
                    items << newItem;
                }
                index++;
            }
            haveShopList(items);
        }
        #endif
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"sell")==0)
    {
        if(stepXml->Attribute("shop")==NULL)
        {
            showTip(tr("Shop called but missing informations").toStdString());
            return;
        }
        bool ok;
        shopId=stringtouint16(stepXml->Attribute("shop"),&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id").toStdString());
            return;
        }
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin=QString::fromStdString(getFrontSkinPath(actualBot.properties.at("skin")));
            if(!skin.isNull())
            {
                ui->shopSellerImage->setPixmap(skin.scaled(160,160));
                ui->shopSellerImage->setVisible(true);
            }
            else
                ui->shopSellerImage->setVisible(false);
        }
        else
            ui->shopSellerImage->setVisible(false);
        waitToSell=true;
        selectObject(ObjectType_Sell);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"learn")==0)
    {
        selectObject(ObjectType_MonsterToLearn);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"heal")==0)
    {
        fightEngine.healAllMonsters();
        client->heal();
        load_monsters();
        showTip(tr("You are healed").toStdString());
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"quests")==0)
    {
        std::string textToShow;
        const std::vector<std::pair<uint16_t,std::string> > &quests=BaseWindow::getQuestList(actualBot.botId);
        if(step==1)
        {
            if(quests.size()==1)
            {
                const std::pair<uint16_t,std::string> &quest=quests.at(0);
                if(QtDatapackClientLoader::datapackLoader->questsExtra.at(quest.first).autostep)
                {
                    IG_dialog_text_linkActivated("quest_"+std::to_string(quest.first));
                    return;
                }
            }
        }
        if(quests.empty())
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                on_IG_dialog_text_linkActivated("next");
            else
                textToShow+=tr("No quests at the moment or you don't meat the requirements").toStdString();
        }
        else
        {
            textToShow+="<ul>";
            unsigned int index=0;
            while(index<quests.size())
            {
                const std::pair<uint32_t,std::string> &quest=quests.at(index);
                textToShow+=QString("<li><a href=\"quest_%1\">%2</a></li>")
                        .arg(quest.first)
                        .arg(QString::fromStdString(quest.second))
                        .toStdString();
                index++;
            }
            if(quests.empty())
                textToShow+=tr("No quests at the moment or you don't meat the requirements").toStdString();
            textToShow+="</ul>";
        }
        ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
        ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"clan")==0)
    {
        std::string textToShow;
        if(playerInformations.clan==0)
        {
            if(playerInformations.allow.find(ActionAllow_Clan)!=playerInformations.allow.cend())
                textToShow=QStringLiteral("<center><a href=\"clan_create\">%1</a></center>").arg(tr("Clan create")).toStdString();
            else
                textToShow=QStringLiteral("<center>You can't create your clan</center>").toStdString();
        }
        else
            textToShow=QStringLiteral("<center>%1</center>").arg(tr("You are already into a clan. Use the clan dongle into the player information.")).toStdString();
        ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
        ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"warehouse")==0)
    {
        monster_to_withdraw.clear();
        monster_to_deposit.clear();
        change_warehouse_items.clear();
        temp_warehouse_cash=0;
        QPixmap pixmap;
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            ui->warehousePlayerImage->setVisible(true);
            ui->warehousePlayerPseudo->setVisible(true);
            ui->warehouseBotImage->setVisible(true);
            ui->warehouseBotPseudo->setVisible(true);
            pixmap=QString::fromStdString(getFrontSkinPath(actualBot.properties.at("skin")));
            if(pixmap.isNull())
            {
                ui->warehousePlayerImage->setVisible(false);
                ui->warehousePlayerPseudo->setVisible(false);
                ui->warehouseBotImage->setVisible(false);
                ui->warehouseBotPseudo->setVisible(false);
            }
            else
                ui->warehouseBotImage->setPixmap(pixmap);
        }
        else
        {
            ui->warehousePlayerImage->setVisible(false);
            ui->warehousePlayerPseudo->setVisible(false);
            ui->warehouseBotImage->setVisible(false);
            ui->warehouseBotPseudo->setVisible(false);
            pixmap=QPixmap(QStringLiteral(":/CC/images/player_default/front.png"));
        }
        ui->stackedWidget->setCurrentWidget(ui->page_warehouse);
        updateTheWareHouseContent();
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"market")==0)
    {
        ui->marketMonster->clear();
        ui->marketObject->clear();
        ui->marketOwnMonster->clear();
        ui->marketOwnObject->clear();
        ui->marketWithdraw->setVisible(false);
        ui->marketStat->setText(tr("In waiting of market list"));
        client->getMarketList();
        ui->stackedWidget->setCurrentWidget(ui->page_market);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"industry")==0)
    {
        if(stepXml->Attribute("industry")==NULL)
        {
            showTip(tr("Industry called but missing informations").toStdString());
            return;
        }

        bool ok;
        factoryId=stringtouint16(stepXml->Attribute("industry"),&ok);
        if(!ok)
        {
            showTip(tr("Industry called but wrong id").toStdString());
            return;
        }
        if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
        {
            showTip(tr("The factory is not found").toStdString());
            return;
        }
        ui->factoryResources->clear();
        ui->factoryProducts->clear();
        ui->factoryStatus->setText(tr("Waiting of status"));
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin=QString::fromStdString(getFrontSkinPath(actualBot.properties.at("skin")));
            if(!skin.isNull())
            {
                ui->factoryBotImage->setPixmap(skin.scaled(80,80));
                ui->factoryBotImage->setVisible(true);
            }
            else
                ui->factoryBotImage->setVisible(false);
        }
        else
            ui->factoryBotImage->setVisible(false);
        ui->stackedWidget->setCurrentWidget(ui->page_factory);
        client->getFactoryList(factoryId);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"zonecapture")==0)
    {
        if(stepXml->Attribute("zone")==NULL)
        {
            showTip(tr("Missing attribute for the step").toStdString());
            return;
        }
        if(playerInformations.clan==0)
        {
            showTip(tr("You can't try capture if you are not in a clan").toStdString());
            return;
        }
        if(!nextCityCatchTimer.isActive())
        {
            showTip(tr("City capture disabled").toStdString());
            return;
        }
        const std::string zone=stepXml->Attribute("zone");
        if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(zone)!=QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
        {
            zonecatchName=QtDatapackClientLoader::datapackLoader->zonesExtra.at(zone).name;
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture %1").arg(QString("<b>%1</b>").arg(QString::fromStdString(zonecatchName))));
        }
        else
        {
            zonecatchName.clear();
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture a zone"));
        }
        updater_page_zonecatch.start(1000);
        nextCatchOnScreen=nextCatch;
        zonecatch=true;
        ui->stackedWidget->setCurrentWidget(ui->page_zonecatch);
        client->waitingForCityCapture(false);
        updatePageZoneCatch();
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"fight")==0)
    {
        if(stepXml->Attribute("fightid")==NULL)
        {
            showTip(tr("Bot step missing data error, repport this error please").toStdString());
            return;
        }
        bool ok;
        const uint16_t &fightId=stringtouint16(stepXml->Attribute("fightid"),&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please").toStdString());
            return;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
        {
            showTip(tr("Bot fight not found").toStdString());
            return;
        }
        if(mapController->haveBeatBot(fightId))
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                goToBotStep(step+1);
            else
                showTip(tr("Already beaten!").toStdString());
            return;
        }
        client->requestFight(fightId);
        botFight(fightId);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please").toStdString());
        return;
    }
}

bool BaseWindow::tryValidateQuestStep(const uint16_t &questId, const uint16_t &botId, bool silent)
{
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        if(!silent)
            showTip(tr("Quest not found").toStdString());
        return
                false;
    }
    const Quest &quest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
    Player_private_and_public_informations &playerInformations=client->get_player_informations();

    if(playerInformations.quests.find(questId)==playerInformations.quests.cend())
    {
        //start for the first time the quest
        if(vectorcontainsAtLeastOne(quest.steps.at(0).bots,botId)
                && haveStartQuestRequirement(quest))
        {
            client->startQuest(questId);
            startQuest(quest);
            updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest").toStdString());
            return false;
        }
    }
    else if(playerInformations.quests.at(questId).step==0)
    {
        //start again the quest if can be repeated
        if(quest.repeatable &&
                vectorcontainsAtLeastOne(quest.steps.at(0).bots,botId)
                && haveStartQuestRequirement(quest))
        {
            client->startQuest(questId);
            startQuest(quest);
            updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest").toStdString());
            return false;
        }
    }
    if(!haveNextStepQuestRequirements(quest))
    {
        if(!silent)
            showTip(tr("You don't have the requirement to continue this quest").toStdString());
        return false;
    }
    if(playerInformations.quests.at(questId).step>=(quest.steps.size()))
    {
        if(!silent)
            showTip(tr("You have finish the quest <b>%1</b>")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name))
                    .toStdString()
                    );
        client->finishQuest(questId);
        nextStepQuest(quest);
        updateDisplayedQuests();
        return true;
    }
    if(vectorcontainsAtLeastOne(quest.steps.at(playerInformations.quests.at(questId).step).bots,botId))
    {
        if(!silent)
            showTip(tr("You need talk to another bot").toStdString());
        return false;
    }
    client->nextQuestStep(questId);
    nextStepQuest(quest);
    updateDisplayedQuests();
    return true;
}

void BaseWindow::showQuestText(const uint32_t &textId)
{
    if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)==
            QtDatapackClientLoader::datapackLoader->questsExtra.cend())
    {
        qDebug() << QStringLiteral("No quest text for this quest: %1").arg(questId);
        showTip(tr("No quest text for this quest").toStdString());
        return;
    }
    const QtDatapackClientLoader::QuestExtra &questExtra=QtDatapackClientLoader::datapackLoader->questsExtra.at(questId);
    if(questExtra.text.find(textId)==questExtra.text.cend())
    {
        qDebug() << "No quest text entry point";
        showTip(tr("No quest text entry point").toStdString());
        return;
    }

    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    uint8_t stepQuest=0;
    if(playerInformations.quests.find(questId)!=playerInformations.quests.cend())
        stepQuest=playerInformations.quests.at(questId).step-1;
    std::string text=tr("No text found").toStdString();
    const QtDatapackClientLoader::QuestTextExtra &questTextExtra=questExtra.text.at(textId);
    unsigned int index=0;
    while(index<questTextExtra.texts.size())
    {
        const QtDatapackClientLoader::QuestStepWithConditionExtra &e=questTextExtra.texts.at(index);
        unsigned int indexCond=0;
        while(indexCond<e.conditions.size())
        {
            const QtDatapackClientLoader::QuestConditionExtra &condition=e.conditions.at(indexCond);
            switch(condition.type)
            {
            case QtDatapackClientLoader::QuestCondition_queststep:
                if((stepQuest+1)!=condition.value)
                    indexCond=e.conditions.size()+999;//not validated condition
            break;
            case QtDatapackClientLoader::QuestCondition_haverequirements:
            {
                const Quest &quest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
                if(!haveNextStepQuestRequirements(quest))
                    indexCond=e.conditions.size()+999;//not validated condition
            }
            break;
            default:
            break;
            }
            indexCond++;
        }
        if(indexCond==e.conditions.size())
        {
            text=e.text;
            break;
        }
        index++;
    }
    std::string textToShow=parseHtmlToDisplay(text);
    ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
    ui->IG_dialog->setVisible(true);
}

bool BaseWindow::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(playerInformations.quests.find(quest.id)==playerInformations.quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=playerInformations.quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1, step: %2").arg(questId).arg(step);
    #endif
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(itemQuantity(item.item)<item.quantity)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not the quantity for the item: " << item.item;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const uint16_t &fightId=requirements.fightId.at(index);
        if(!mapController->haveBeatBot(fightId))
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not beat the bot: " << fightId;
            #endif
            return false;
        }
        index++;
    }
    return true;
}

bool BaseWindow::haveStartQuestRequirement(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest requirement for: " << quest.id;
    #endif
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(playerInformations.quests.find(quest.id)!=playerInformations.quests.cend())
    {
        const PlayerQuest &playerquest=playerInformations.quests.at(quest.id);
        if(playerquest.step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << questId;
            #endif
            return false;
        }
        if(playerquest.finish_one_time && !quest.repeatable)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "done one time and no repeatable: " << questId;
            #endif
            return false;
        }
    }
    unsigned int index=0;
    while(index<quest.requirements.quests.size())
    {
        const uint16_t &questId=quest.requirements.quests.at(index).quest;
        if(
                (playerInformations.quests.find(questId)==playerInformations.quests.cend() && !quest.requirements.quests.at(index).inverse)
                ||
                (playerInformations.quests.find(questId)!=playerInformations.quests.cend() && quest.requirements.quests.at(index).inverse)
                )
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have never started the quest: " << questId;
            #endif
            return false;
        }
        if(playerInformations.quests.find(questId)!=playerInformations.quests.cend())
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never started: " << questId;
            #endif
            return false;
        }
        const PlayerQuest &playerquest=playerInformations.quests.at(quest.id);
        if(!playerquest.finish_one_time)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never finished: " << questId;
            #endif
            return false;
        }
        index++;
    }
    return haveReputationRequirements(quest.requirements.reputation);
}

void BaseWindow::on_IG_dialog_text_linkActivated(const QString &rawlink)
{
    IG_dialog_text_linkActivated(rawlink.toStdString());
}

void BaseWindow::IG_dialog_text_linkActivated(const std::string &rawlink)
{
    ui->IG_dialog->setVisible(false);
    std::vector<std::string> stringList=stringsplit(rawlink,';');
    unsigned int index=0;
    while(index<stringList.size())
    {
        const std::string &link=stringList.at(index);
        qDebug() << "parsed link to use: " << QString::fromStdString(link);
        if(stringStartWith(link,"http://") || stringStartWith(link,"https://"))
        {
            if(!QDesktopServices::openUrl(QUrl(QString::fromStdString(link))))
                showTip(QStringLiteral("Unable to open the url: %1").arg(QString::fromStdString(link)).toStdString());
            index++;
            continue;
        }
        bool ok;
        if(stringStartWith(link,"quest_"))
        {
            std::string tempLink=link;
            stringreplaceOne(tempLink,"quest_","");
            const uint16_t &questId=stringtouint16(tempLink,&ok);
            if(!ok)
            {
                showTip(QStringLiteral("Unable to open the link: %1").arg(QString::fromStdString(link)).toStdString());
                index++;
                continue;
            }
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==
                    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
            {
                showTip(tr("Quest not found").toStdString());
                index++;
                continue;
            }
            isInQuest=true;
            this->questId=questId;
            if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)!=
                    QtDatapackClientLoader::datapackLoader->questsExtra.cend())
            {
                if(QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).autostep)
                {
                    int index=0;
                    bool ok;
                    do
                    {
                        ok=tryValidateQuestStep(questId,actualBot.botId);
                        index++;
                    } while(ok && index<99);
                    if(index==99)
                    {
                        emit error("Infinity loop into autostep for quest: "+std::to_string(questId));
                        return;
                    }
                }
                showQuestText(1);
                index++;
                continue;
            }
            else
            {
                showTip(tr("Quest extra not found").toStdString());
                index++;
                continue;
            }
        }
        else if(link=="clan_create")
        {
            bool ok;
            std::string text = QInputDialog::getText(this,tr("Give the clan name"),tr("Clan name:"),QLineEdit::Normal,QString(), &ok).toStdString();
            if(ok && !text.empty())
            {
                actionClan.push_back(ActionClan_Create);
                client->createClan(text);
            }
            index++;
            continue;
        }
        else if(link=="close")
            return;
        else if(link=="next_quest_step" && isInQuest)
        {
            tryValidateQuestStep(questId,actualBot.botId);
            index++;
            continue;
        }
        else if(link=="next" && lastStepUsed>=1)
        {
            goToBotStep(lastStepUsed+1);
            index++;
            continue;
        }
        uint8_t step=stringtouint8(link,&ok);
        if(!ok)
        {
            showTip(QStringLiteral("Unable to open the link: %1").arg(QString::fromStdString(link)).toStdString());
            index++;
            continue;
        }
        if(isInQuest)
        {
            showQuestText(step);
            index++;
            continue;
        }
        goToBotStep(step);
        index++;
    }
}

bool BaseWindow::startQuest(const Quest &quest)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    if(playerInformations.quests.find(quest.id)==playerInformations.quests.cend())
    {
        playerInformations.quests[quest.id].step=1;
        playerInformations.quests[quest.id].finish_one_time=false;
    }
    else
        playerInformations.quests[quest.id].step=1;
    return true;
}

std::vector<std::pair<uint16_t,std::string> > BaseWindow::getQuestList(const uint16_t &botId) const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    std::vector<std::pair<uint16_t,std::string> > entryList;
    std::pair<uint16_t,std::string> oneEntry;
    //do the not started quest here
    std::vector<uint16_t> botQuests=QtDatapackClientLoader::datapackLoader->botToQuestStart.at(botId);
    unsigned int index=0;
    while(index<botQuests.size())
    {
        const uint16_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at BaseWindow::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
        if(playerInformations.quests.find(botQuests.at(index))==playerInformations.quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
            {
                oneEntry.first=questId;
                if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)!=
                        QtDatapackClientLoader::datapackLoader->questsExtra.cend())
                    oneEntry.second=QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList.push_back(oneEntry);
            }
            else
                {}//have not the requirement
        }
        else
        {
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(botQuests.at(index))==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(playerInformations.quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(playerInformations.quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                            {
                                oneEntry.first=questId;
                                if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)!=
                                        QtDatapackClientLoader::datapackLoader->questsExtra.cend())
                                    oneEntry.second=QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList.push_back(oneEntry);
                            }
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    auto bots=currentQuest.steps.at(playerInformations.quests.at(questId).step-1).bots;
                    if(vectorcontainsAtLeastOne(bots,botId))
                    {
                        oneEntry.first=questId;
                        if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)!=
                                QtDatapackClientLoader::datapackLoader->questsExtra.cend())
                            oneEntry.second=tr("%1 (in progress)")
                                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name)).toStdString();
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)").toStdString();
                        }
                        entryList.push_back(oneEntry);
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    auto i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        if(!vectorcontainsAtLeastOne(botQuests,i->first) &&
                i->second.step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(i->first);
            auto bots=currentQuest.steps.at(i->second.step-1).bots;
            if(vectorcontainsAtLeastOne(bots,botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=i->first;
                oneEntry.second=tr("%1 (in progress)")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->questsExtra.at(i->first).name))
                        .toStdString();
                entryList.push_back(oneEntry);
            }
            else
                {}//it's another bot
        }
        ++i;
    }
    return entryList;
}
