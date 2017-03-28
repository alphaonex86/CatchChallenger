#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../LanguagesSelect.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../fight/interface/ClientFightEngine.h"

#include <QScriptValue>
#include <QScriptEngine>
#include <QDesktopServices>
#include <QInputDialog>

using namespace CatchChallenger;

bool BaseWindow::botHaveQuest(const uint32_t &botId)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    //do the not started quest here
    QList<uint32_t> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const uint32_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at BaseWindow::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
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
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
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
                    auto bots=currentQuest.steps.at(quests.at(questId).step-1).bots;
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
    auto i=quests.begin();
    while(i!=quests.cend())
    {
        if(!botQuests.contains(i->first) && i->second.step>0)
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
    lastStepUsed=step;
    isInQuest=false;
    if(actualBot.step.find(step)==actualBot.step.cend())
    {
        showTip(tr("Error into the bot, repport this error please"));
        return;
    }
    if(*actualBot.step.at(step)->Attribute(std::string("type"))=="text")
    {
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        auto text = actualBot.step.at(step)->FirstChildElement(std::string("text"));
        if(!language.isEmpty() && language!=BaseWindow::text_en)
            while(text!=NULL)
            {
                if(text->Attribute(std::string("lang"))!=NULL && *text->Attribute(std::string("lang"))==language.toStdString())
                {
                    QString textToShow=QString::fromStdString(text->GetText());
                    textToShow=parseHtmlToDisplay(textToShow);
                    ui->IG_dialog_text->setText(textToShow);
                    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
                    ui->IG_dialog->setVisible(true);
                    return;
                }
                text = text->NextSiblingElement(std::string("text"));
            }
        text = actualBot.step.at(step)->FirstChildElement(std::string("text"));
        while(text!=NULL)
        {
            if(text->Attribute(std::string("lang"))==NULL || *text->Attribute(std::string("lang"))==language.toStdString())
            {
                QString textToShow=text->GetText();
                textToShow=parseHtmlToDisplay(textToShow);
                ui->IG_dialog_text->setText(textToShow);
                ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
                ui->IG_dialog->setVisible(true);
                return;
            }
            text = text->NextSiblingElement(std::string("text"));
        }
        showTip(tr("Bot text not found, repport this error please"));
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="shop")
    {
        if(actualBot.step.at(step)->Attribute(std::string("shop"))==NULL)
        {
            showTip(tr("Shop called but missing informations"));
            return;
        }
        bool ok;
        shopId=stringtouint32(*actualBot.step.at(step)->Attribute(std::string("shop")),&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id"));
            return;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shopId)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.cend())
        {
            showTip(tr("Shop not found"));
            return;
        }
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin=getFrontSkinPath(QString::fromStdString(actualBot.properties.at("skin")));
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
            QList<ItemToSellOrBuy> items;
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="sell")
    {
        if(actualBot.step.at(step)->Attribute(std::string("shop"))==NULL)
        {
            showTip(tr("Shop called but missing informations"));
            return;
        }
        bool ok;
        shopId=stringtouint32(*actualBot.step.at(step)->Attribute(std::string("shop")),&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id"));
            return;
        }
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin=getFrontSkinPath(QString::fromStdString(actualBot.properties.at("skin")));
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="learn")
    {
        selectObject(ObjectType_MonsterToLearn);
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="heal")
    {
        fightEngine.healAllMonsters();
        client->heal();
        load_monsters();
        showTip(tr("You are healed"));
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="quests")
    {
        QString textToShow;
        const QList<QPair<uint32_t,QString> > &quests=BaseWindow::getQuestList(actualBot.botId);
        if(step==1)
        {
            if(quests.size()==1)
            {
                const QPair<uint32_t,QString> &quest=quests.at(0);
                if(DatapackClientLoader::datapackLoader.questsExtra.value(quest.first).autostep)
                {
                    on_IG_dialog_text_linkActivated(QString("quest_%1").arg(quest.first));
                    return;
                }
            }
        }
        if(quests.isEmpty())
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                on_IG_dialog_text_linkActivated("next");
            else
                textToShow+=tr("No quests at the moment or you don't meat the requirements");
        }
        else
        {
            textToShow+=QStringLiteral("<ul>");
            int index=0;
            while(index<quests.size())
            {
                const QPair<uint32_t,QString> &quest=quests.at(index);
                textToShow+=QStringLiteral("<li><a href=\"quest_%1\">%2</a></li>").arg(quest.first).arg(quest.second);
                index++;
            }
            if(quests.isEmpty())
                textToShow+=tr("No quests at the moment or you don't meat the requirements");
            textToShow+=QStringLiteral("</ul>");
        }
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="clan")
    {
        QString textToShow;
        if(clan==0)
        {
            if(allow.find(ActionAllow_Clan)!=allow.cend())
                textToShow=QStringLiteral("<center><a href=\"clan_create\">%1</a></center>").arg(tr("Clan create"));
            else
                textToShow=QStringLiteral("<center>You can't create your clan</center>");
        }
        else
            textToShow=QStringLiteral("<center>%1</center>").arg(tr("You are already into a clan. Use the clan dongle into the player information."));
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="warehouse")
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
            pixmap=getFrontSkinPath(QString::fromStdString(actualBot.properties.at("skin")));
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="market")
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="industry")
    {
        if(actualBot.step.at(step)->Attribute("industry")==NULL)
        {
            showTip(tr("Industry called but missing informations"));
            return;
        }

        bool ok;
        factoryId=stringtouint32(*actualBot.step.at(step)->Attribute(std::string("industry")),&ok);
        if(!ok)
        {
            showTip(tr("Industry called but wrong id"));
            return;
        }
        if(CommonDatapack::commonDatapack.industriesLink.find(factoryId)==CommonDatapack::commonDatapack.industriesLink.cend())
        {
            showTip(tr("The factory is not found"));
            return;
        }
        ui->factoryResources->clear();
        ui->factoryProducts->clear();
        ui->factoryStatus->setText(tr("Waiting of status"));
        if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        {
            QPixmap skin=getFrontSkinPath(QString::fromStdString(actualBot.properties.at("skin")));
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="zonecapture")
    {
        if(!actualBot.step.at(step)->Attribute(std::string("zone")))
        {
            showTip(tr("Missing attribute for the step"));
            return;
        }
        if(clan==0)
        {
            showTip(tr("You can't try capture if you are not in a clan"));
            return;
        }
        if(!nextCityCatchTimer.isActive())
        {
            showTip(tr("City capture disabled"));
            return;
        }
        QString zone=QString::fromStdString(*actualBot.step.at(step)->Attribute(std::string("zone")));
        if(DatapackClientLoader::datapackLoader.zonesExtra.contains(zone))
        {
            zonecatchName=DatapackClientLoader::datapackLoader.zonesExtra.value(zone).name;
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture %1").arg(QStringLiteral("<b>%1</b>").arg(zonecatchName)));
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
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="script")
    {
        QScriptEngine engine;
        QString contents = QString::fromStdString(actualBot.step.at(step)->GetText());
        contents=QStringLiteral("function getTextEntryPoint()\n{\n")+contents+QStringLiteral("\n}");
        QScriptValue result = engine.evaluate(contents);
        if (result.isError()) {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(result.property("lineNumber").toInt32())
                        .arg(result.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(result.property("lineNumber").toInt32())
            .arg(result.toString()));
            return;
        }

        QScriptValue getTextEntryPoint = engine.globalObject().property(QStringLiteral("getTextEntryPoint"));
        if(getTextEntryPoint.isError())
        {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(getTextEntryPoint.property("lineNumber").toInt32())
                        .arg(getTextEntryPoint.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(getTextEntryPoint.property("lineNumber").toInt32())
            .arg(getTextEntryPoint.toString()));
            return;
        }
        QScriptValue returnValue=getTextEntryPoint.call();
        uint32_t textEntryPoint=returnValue.toNumber();
        if(returnValue.isError())
        {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(returnValue.property("lineNumber").toInt32())
                        .arg(returnValue.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(returnValue.property(QStringLiteral("lineNumber")).toInt32())
            .arg(returnValue.toString()));
            return;
        }
        qDebug() << QStringLiteral("textEntryPoint:") << textEntryPoint;
        return;
    }
    else if(*actualBot.step.at(step)->Attribute(std::string("type"))=="fight")
    {
        if(actualBot.step.at(step)->Attribute(std::string("fightid"))==NULL)
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        uint32_t fightId=stringtouint32(*actualBot.step.at(step)->Attribute(std::string("fightid")),&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please"));
            return;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
        {
            showTip(tr("Bot fight not found"));
            return;
        }
        if(mapController->haveBeatBot(fightId))
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                goToBotStep(step+1);
            else
                showTip(tr("Already beaten!"));
            return;
        }
        client->requestFight(fightId);
        botFight(fightId);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please"));
        return;
    }
}

bool BaseWindow::tryValidateQuestStep(const uint16_t &questId, const uint32_t &botId, bool silent)
{
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
    {
        if(!silent)
            showTip(tr("Quest not found"));
        return
                false;
    }
    const Quest &quest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);

    if(quests.find(questId)==quests.cend())
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
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    else if(quests.at(questId).step==0)
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
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    if(!haveNextStepQuestRequirements(quest))
    {
        if(!silent)
            showTip(tr("You don't have the requirement to continue this quest"));
        return false;
    }
    if(quests.at(questId).step>=(quest.steps.size()))
    {
        if(!silent)
            showTip(tr("You have finish the quest <b>%1</b>").arg(DatapackClientLoader::datapackLoader.questsExtra.value(questId).name));
        client->finishQuest(questId);
        nextStepQuest(quest);
        updateDisplayedQuests();
        return true;
    }
    if(vectorcontainsAtLeastOne(quest.steps.at(quests.at(questId).step).bots,botId))
    {
        if(!silent)
            showTip(tr("You need talk to another bot"));
        return false;
    }
    client->nextQuestStep(questId);
    nextStepQuest(quest);
    updateDisplayedQuests();
    return true;
}

void BaseWindow::getTextEntryPoint()
{
    if(!isInQuest)
    {
        showTip(QStringLiteral("Internal error: Is not in quest"));
        return;
    }
    QScriptEngine engine;

    const QString &client_logic=client->datapackPathMain()+DATAPACK_BASE_PATH_QUESTS2+"/"+QString::number(questId)+"/client_logic.js";
    if(!QFile(client_logic).exists())
    {
        showTip(tr("Client file missing"));
        qDebug() << "client_logic file is missing:" << client_logic;
        return;
    }

    QFile scriptFile(client_logic);
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream stream(&scriptFile);
    QString contents = stream.readAll();
    contents="function getTextEntryPoint()\n{\n"+contents+"\n}";
    uint8_t currentQuestStepVar;
    bool haveNextStepQuestRequirementsVar;
    bool finishOneTimeVar;
    scriptFile.close();
    if(quests.find(questId)==quests.cend())
    {
        contents.replace("currentQuestStep()","0");
        contents.replace("currentBot()","0");
        contents.replace("finishOneTime()","false");
        contents.replace("haveQuestStepRequirements()","false");//bug if use that's
        currentQuestStepVar=0;
        haveNextStepQuestRequirementsVar=false;
        finishOneTimeVar=false;
    }
    else
    {
        PlayerQuest quest=quests.at(questId);
        contents.replace("currentQuestStep()",QString::number(quest.step));
        contents.replace("currentBot()",QString::number(actualBot.botId));
        if(quest.finish_one_time)
            contents.replace("finishOneTime()","true");
        else
            contents.replace("finishOneTime()","false");
        if(quest.step<=0)
        {
            contents.replace("haveQuestStepRequirements()","false");
            haveNextStepQuestRequirementsVar=false;
        }
        else if(haveNextStepQuestRequirements(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId)))
        {
            contents.replace("haveQuestStepRequirements()","true");
            haveNextStepQuestRequirementsVar=true;
        }
        else
        {
            contents.replace("haveQuestStepRequirements()","false");
            haveNextStepQuestRequirementsVar=false;
        }
        currentQuestStepVar=quest.step;
        finishOneTimeVar=quest.finish_one_time;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "currentQuestStep:" << currentQuestStepVar << ", haveNextStepQuestRequirements:" << haveNextStepQuestRequirementsVar << ", finishOneTime:" << finishOneTimeVar << ", contents:" << contents;
    #endif

    QScriptValue result = engine.evaluate(contents, client_logic);
    if (result.isError()) {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(result.property("lineNumber").toInt32())
                    .arg(result.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(result.property("lineNumber").toInt32())
        .arg(result.toString()));
        return;
    }

    QScriptValue getTextEntryPoint = engine.globalObject().property("getTextEntryPoint");
    if(getTextEntryPoint.isError())
    {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(getTextEntryPoint.property("lineNumber").toInt32())
                    .arg(getTextEntryPoint.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(getTextEntryPoint.property("lineNumber").toInt32())
        .arg(getTextEntryPoint.toString()));
        return;
    }
    QScriptValue returnValue=getTextEntryPoint.call();
    uint32_t textEntryPoint=(uint32_t)returnValue.toInt32();
    if(returnValue.isError())
    {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(returnValue.property("lineNumber").toInt32())
                    .arg(returnValue.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(returnValue.property("lineNumber").toInt32())
        .arg(returnValue.toString()));
        return;
    }
    qDebug() << "textEntryPoint:" << textEntryPoint;
    showQuestText(textEntryPoint);

    Q_UNUSED(currentQuestStepVar);
    Q_UNUSED(haveNextStepQuestRequirementsVar);
    Q_UNUSED(finishOneTimeVar);
}

void BaseWindow::showQuestText(const uint32_t &textId)
{
    if(!DatapackClientLoader::datapackLoader.questsText.contains(questId))
    {
        qDebug() << QStringLiteral("No quest text for this quest: %1").arg(questId);
        showTip(tr("No quest text for this quest"));
        return;
    }
    if(!DatapackClientLoader::datapackLoader.questsText.value(questId).text.contains(textId))
    {
        qDebug() << "No quest text entry point";
        showTip(tr("No quest text entry point"));
        return;
    }

    QString textToShow=parseHtmlToDisplay(DatapackClientLoader::datapackLoader.questsText.value(questId).text.value(textId));
    ui->IG_dialog_text->setText(textToShow);
    ui->IG_dialog_name->setText(QString::fromStdString(actualBot.name));
    ui->IG_dialog->setVisible(true);
}

bool BaseWindow::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    if(quests.find(quest.id)==quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=quests.at(quest.id).step;
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
        const uint32_t &fightId=requirements.fightId.at(index);
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
    if(quests.find(quest.id)!=quests.cend())
    {
        const PlayerQuest &playerquest=quests.at(quest.id);
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
                (quests.find(questId)==quests.cend() && !quest.requirements.quests.at(index).inverse)
                ||
                (quests.find(questId)!=quests.cend() && quest.requirements.quests.at(index).inverse)
                )
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have never started the quest: " << questId;
            #endif
            return false;
        }
        if(quests.find(questId)!=quests.cend())
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never started: " << questId;
            #endif
            return false;
        }
        const PlayerQuest &playerquest=quests.at(quest.id);
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
    ui->IG_dialog->setVisible(false);
    QStringList stringList=rawlink.split(";");
    int index=0;
    while(index<stringList.size())
    {
        const QString &link=stringList.at(index);
        qDebug() << "parsed link to use: " << link;
        if(link.startsWith("http://") || link.startsWith("https://"))
        {
            if(!QDesktopServices::openUrl(QUrl(link)))
                showTip(QStringLiteral("Unable to open the url: %1").arg(link));
            index++;
            continue;
        }
        bool ok;
        if(link.startsWith("quest_"))
        {
            QString tempLink=link;
            tempLink.remove("quest_");
            uint32_t questId=tempLink.toUShort(&ok);
            if(!ok)
            {
                showTip(QStringLiteral("Unable to open the link: %1").arg(link));
                index++;
                continue;
            }
            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
            {
                showTip(tr("Quest not found"));
                index++;
                continue;
            }
            isInQuest=true;
            this->questId=questId;
            if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                if(DatapackClientLoader::datapackLoader.questsExtra.value(questId).autostep)
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
                        emit error(QString("Infinity loop into autostep for quest: %1").arg(questId));
                        return;
                    }
                }
            getTextEntryPoint();
            index++;
            continue;
        }
        else if(link=="clan_create")
        {
            bool ok;
            QString text = QInputDialog::getText(this,tr("Give the clan name"),tr("Clan name:"),QLineEdit::Normal,QString(), &ok);
            if(ok && !text.isEmpty())
            {
                actionClan << ActionClan_Create;
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
        uint8_t step=link.toUShort(&ok);
        if(!ok)
        {
            showTip(QStringLiteral("Unable to open the link: %1").arg(link));
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
    if(quests.find(quest.id)==quests.cend())
    {
        quests[quest.id].step=1;
        quests[quest.id].finish_one_time=false;
    }
    else
        quests[quest.id].step=1;
    return true;
}

QList<QPair<uint32_t,QString> > BaseWindow::getQuestList(const uint32_t &botId)
{
    QList<QPair<uint32_t,QString> > entryList;
    QPair<uint32_t,QString> oneEntry;
    //do the not started quest here
    QList<uint32_t> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const uint32_t &questId=botQuests.at(index);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(questId!=botQuests.at(index))
            qDebug() << "cast error for questId at BaseWindow::getQuestList()";
        #endif
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
        if(quests.find(botQuests.at(index))==quests.cend())
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
            {
                oneEntry.first=questId;
                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra.value(questId).name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList << oneEntry;
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
                if(quests.at(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.at(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                            {
                                oneEntry.first=questId;
                                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra.value(questId).name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList << oneEntry;
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
                    auto bots=currentQuest.steps.at(quests.at(questId).step-1).bots;
                    if(vectorcontainsAtLeastOne(bots,botId))
                    {
                        oneEntry.first=questId;
                        if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                            oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra.value(questId).name);
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)");
                        }
                        entryList << oneEntry;
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    auto i=quests.begin();
    while(i!=quests.cend())
    {
        if(!botQuests.contains(i->first) && i->second.step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(i->first);
            auto bots=currentQuest.steps.at(i->second.step-1).bots;
            if(vectorcontainsAtLeastOne(bots,botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=i->first;
                oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra.value(i->first).name);
                entryList << oneEntry;
            }
            else
                {}//it's another bot
        }
        ++i;
    }
    return entryList;
}
