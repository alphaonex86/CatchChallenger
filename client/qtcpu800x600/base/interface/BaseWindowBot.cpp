#include "BaseWindow.h"
#include "../../../libqtcatchchallenger/ChatParsing.hpp"
#include "ui_BaseWindow.h"
#include "../LanguagesSelect.h"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

#include <QDesktopServices>
#include <QInputDialog>
#include <QString>
#include <iostream>

using namespace CatchChallenger;

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
        std::string textToShow;
        if(!CatchChallenger::Api_protocol::botStepText(stepXml,LanguagesSelect::languagesSelect->getCurrentLanguages(),textToShow))
        {
            showTip(tr("Bot text not found, repport this error please").toStdString());
            return;
        }
        textToShow=CatchChallenger::ChatParsing::botTextToDisplay(textToShow,client->botHaveQuest(actualBotMapId,actualBot.botId));
        ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
        ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
        ui->IG_dialog->setVisible(true);
        //mirror to the QLocalServer GETDIALOG channel (read-only obs)
        if(mapController!=nullptr)
            mapController->setRemoteDialogText(QString::fromStdString(textToShow));
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"shop")==0)
    {
        //no server packet for the shop content: build the item list directly
        //from the bot step's inline <product> list in the datapack (see
        //Api_protocol::shopItemsFromStepXml). The shop="id" attribute is the
        //legacy form and is sent nowhere (buyObject re-resolves via the tile).
        const std::vector<ItemToSellOrBuy> items=CatchChallenger::Api_protocol::shopItemsFromStepXml(stepXml);
        if(items.empty())
        {
            showTip(tr("Shop called but missing informations").toStdString());
            return;
        }
        shopId=0;
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
        haveShopList(items);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"sell")==0)
    {
        //a sell step needs no shop info: the server resolves the shop from the
        //faced bot position; the shop="id" attribute is the legacy form
        shopId=0;
        if(stepXml->Attribute("shop")!=NULL)
        {
            bool ok;
            shopId=stringtouint16(stepXml->Attribute("shop"),&ok);
            if(!ok)
            {
                showTip(tr("Shop called but wrong id").toStdString());
                return;
            }
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
    else if(strcmp(stepXml->Attribute("type"),"heal")==0)
    {
        client->healAllMonsters();
        client->heal();
        load_monsters();
        showTip(tr("You are healed").toStdString());
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"quests")==0)
    {
        std::string textToShow;
        const std::vector<std::pair<uint16_t,std::string> > &quests=getQuestList();
        if(step==1)
        {
            if(quests.size()==1)
            {
                const std::pair<uint16_t,std::string> &quest=quests.at(0);
                if(QtDatapackClientLoader::datapackLoader->get_questExtra(quest.first).autostep)
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
            if(playerInformations.allow_create_clan)
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
            pixmap=QPixmap(QStringLiteral(":/images/player_default/front.png"));
        }
        ui->stackedWidget->setCurrentWidget(ui->page_warehouse);
        updateTheWareHouseContent();
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
        if(factoryId>=QtDatapackClientLoader::datapackLoader->getMap(actualBotMapId).industries.size())
        {
            showTip(tr("The factory is not found").toStdString());
            return;
        }
        factoryMapId=actualBotMapId;
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
        client->getFactoryList();
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
        if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(zone))
        {
            zonecatchName=QtDatapackClientLoader::datapackLoader->get_zoneExtra(zone).name;
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
        if(mapController->haveBeatBot(mapController->currentMap(), static_cast<CATCHCHALLENGER_TYPE_BOTID>(fightId)))
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                goToBotStep(step+1);
            else
                showTip(tr("Already beaten!").toStdString());
            return;
        }
        client->requestFight();
        botFight(fightId);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please").toStdString());
        return;
    }
}

//the actual bot's quests with their display name
std::vector<std::pair<uint16_t,std::string> > BaseWindow::getQuestList() const
{
    std::vector<std::pair<uint16_t,std::string> > entryList;
    const std::vector<std::pair<CATCHCHALLENGER_TYPE_QUEST,bool> > quests=client->questsForBot(actualBotMapId,actualBot.botId);
    unsigned int index=0;
    while(index<quests.size())
    {
        const CATCHCHALLENGER_TYPE_QUEST questId=quests.at(index).first;
        QString name("???");
        if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
            name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name);
        if(quests.at(index).second)
            name=tr("%1 (in progress)").arg(name);
        entryList.push_back(std::pair<uint16_t,std::string>(questId,name.toStdString()));
        index++;
    }
    return entryList;
}

bool BaseWindow::tryValidateQuestStep(const uint16_t &questId, bool silent)
{
    std::vector<CatchChallenger::Api_protocol::ReputationLevelChange> reputationChanges;
    switch(client->validateQuestStep(static_cast<CATCHCHALLENGER_TYPE_QUEST>(questId),actualBotMapId,actualBot.botId,reputationChanges))
    {
        case CatchChallenger::Api_protocol::QuestStep_Started:
        case CatchChallenger::Api_protocol::QuestStep_NextStep:
            updateDisplayedQuests();
            return true;
        case CatchChallenger::Api_protocol::QuestStep_Finished:
            if(!silent)
            {
                QString name("???");
                if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
                    name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_questExtra(questId).name);
                showTip(tr("You have finish the quest <b>%1</b>").arg(name).toStdString());
            }
            showReputationChanges(reputationChanges);
            updateDisplayedQuests();
            return true;
        case CatchChallenger::Api_protocol::QuestStep_UnknownQuest:
            if(!silent)
                showTip(tr("Quest not found").toStdString());
            return false;
        case CatchChallenger::Api_protocol::QuestStep_CantStart:
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest").toStdString());
            return false;
        case CatchChallenger::Api_protocol::QuestStep_OtherBot:
            if(!silent)
                showTip(tr("You need talk to another bot").toStdString());
            return false;
        case CatchChallenger::Api_protocol::QuestStep_CantContinue:
        default:
            if(!silent)
                showTip(tr("You don't have the requirement to continue this quest").toStdString());
            return false;
    }
}

void BaseWindow::showQuestText(const uint32_t &textId)
{
    std::string text;
    switch(QtDatapackClientLoader::datapackLoader->questText(questId,textId,*client,text))
    {
        case DatapackClientLoader::QuestText_NoQuestExtra:
            qDebug() << QStringLiteral("No quest text for this quest: %1").arg(questId);
            showTip(tr("No quest text for this quest").toStdString());
            return;
        case DatapackClientLoader::QuestText_NoEntryPoint:
            qDebug() << "No quest text entry point";
            showTip(tr("No quest text entry point").toStdString());
            return;
        case DatapackClientLoader::QuestText_NoText:
            text=tr("No text found").toStdString();
        break;
        default:
        break;
    }
    const std::string textToShow=CatchChallenger::ChatParsing::botTextToDisplay(text,client->botHaveQuest(actualBotMapId,actualBot.botId));
    ui->IG_dialog_text->setText(QString::fromStdString(textToShow));
    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
    ui->IG_dialog->setVisible(true);
}

void BaseWindow::on_IG_dialog_text_linkActivated(const QString &rawlink)
{
    IG_dialog_text_linkActivated(rawlink.toStdString());
}

void BaseWindow::IG_dialog_text_linkActivated(const std::string &rawlink)
{
    ui->IG_dialog->setVisible(false);
    const std::vector<std::string> stringList=stringsplit(rawlink,';');
    unsigned int index=0;
    while(index<stringList.size())
    {
        const std::string &link=stringList.at(index);
        const CatchChallenger::Api_protocol::BotLink botLink=CatchChallenger::Api_protocol::parseBotLink(link,isInQuest,lastStepUsed);
        switch(botLink.type)
        {
            case CatchChallenger::Api_protocol::BotLink_Url:
                if(!QDesktopServices::openUrl(QUrl(QString::fromStdString(link))))
                    showTip(QStringLiteral("Unable to open the url: %1").arg(QString::fromStdString(link)).toStdString());
            break;
            case CatchChallenger::Api_protocol::BotLink_Quest:
                if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(botLink.value))
                    showTip(tr("Quest not found").toStdString());
                else if(!QtDatapackClientLoader::datapackLoader->has_questExtra(botLink.value))
                    showTip(tr("Quest extra not found").toStdString());
                else
                {
                    isInQuest=true;
                    questId=botLink.value;
                    if(QtDatapackClientLoader::datapackLoader->get_questExtra(questId).autostep)
                    {
                        int tries=0;
                        bool validated=true;
                        while(validated && tries<99)
                        {
                            validated=tryValidateQuestStep(questId);
                            tries++;
                        }
                        if(validated)
                        {
                            emit error("Infinity loop into autostep for quest: "+std::to_string(questId));
                            return;
                        }
                    }
                    showQuestText(1);
                }
            break;
            case CatchChallenger::Api_protocol::BotLink_ClanCreate:
            {
                bool ok;
                std::string text = QInputDialog::getText(this,tr("Give the clan name"),tr("Clan name:"),QLineEdit::Normal,QString(), &ok).toStdString();
                if(ok && !text.empty())
                {
                    actionClan.push_back(ActionClan_Create);
                    client->createClan(text);
                }
            }
            break;
            case CatchChallenger::Api_protocol::BotLink_Close:
                return;
            case CatchChallenger::Api_protocol::BotLink_NextQuestStep:
                tryValidateQuestStep(questId);
            break;
            case CatchChallenger::Api_protocol::BotLink_Step:
                goToBotStep(static_cast<uint8_t>(botLink.value));
            break;
            case CatchChallenger::Api_protocol::BotLink_QuestText:
                showQuestText(botLink.value);
            break;
            default:
                showTip(QStringLiteral("Unable to open the link: %1").arg(QString::fromStdString(link)).toStdString());
            break;
        }
        index++;
    }
}

