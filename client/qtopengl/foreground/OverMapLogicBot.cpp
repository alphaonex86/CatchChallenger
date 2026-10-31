#include "OverMapLogic.hpp"
#include "../../libqtcatchchallenger/ChatParsing.hpp"
#include "../ConnexionManager.hpp"
#include "../background/CCMap.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libqtcatchchallenger/maprender/QMap_client.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../above/Shop.hpp"
#include "../above/Factory.hpp"
#include "../above/Warehouse.hpp"
#include "../above/TextInput.hpp"
#include <iostream>
#include <QDesktopServices>
#include <QUrl>

//bot
uint8_t OverMapLogic::resolveBotSkinId() const
{
    if(actualBot.properties.find("skin")==actualBot.properties.cend())
        return 0;
    //the "skin" property is a skin FOLDER NAME (e.g. "oldman"/"nurse"); look up its
    //index in the skin list. std::stoi on a name throws (and we forbid exceptions).
    const std::string &skinName=actualBot.properties.at("skin");
    const std::vector<std::string> &skins=QtDatapackClientLoader::datapackLoader->get_skins();
    unsigned int i=0;
    while(i<skins.size())
    {
        if(skins.at(i)==skinName)
            return static_cast<uint8_t>(i);
        i++;
    }
    return 0;
}

void OverMapLogic::goToBotStep(const uint8_t &step)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
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
        const std::string &language=Language::language.getLanguage().toStdString();
        #else
        const std::string language("en");
        #endif
        const tinyxml2::XMLElement *text = stepXml->FirstChildElement("text");
        if(!language.empty() && language!="en")
            while(text!=NULL)
            {
                if(text->Attribute("lang")!=NULL && text->Attribute("lang")==language && text->GetText()!=NULL)
                {
                    std::string textToShow=text->GetText();
                    textToShow=CatchChallenger::ChatParsing::botTextToDisplay(textToShow,connexionManager->client->botHaveQuest(actualBotMapId,actualBot.botId));
                    setIG_dialog(QString::fromStdString(textToShow),QString::fromStdString(actualBot.name));
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
                    textToShow=CatchChallenger::ChatParsing::botTextToDisplay(textToShow,connexionManager->client->botHaveQuest(actualBotMapId,actualBot.botId));
                    setIG_dialog(QString::fromStdString(textToShow),QString::fromStdString(actualBot.name));
                    return;
                }
            text = text->NextSiblingElement("text");
        }
        showTip(tr("Bot text not found, repport this error please").toStdString());
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"shop")==0)
    {
        //no server packet for the shop content: build the item list directly
        //from the bot step's inline <product> list in the datapack (see
        //Api_protocol::shopItemsFromStepXml). The shop="id" attribute is the
        //legacy form and is sent nowhere (buyObject re-resolves via the tile).
        const std::vector<CatchChallenger::ItemToSellOrBuy> items=
                CatchChallenger::Api_protocol::shopItemsFromStepXml(stepXml);
        if(items.empty())
        {
            showTip(tr("Shop called but missing informations").toStdString());
            return;
        }
        // open shop screen
        {
            const uint8_t skinId=resolveBotSkinId();
            if(shop==nullptr)
            {
                shop=new Shop();
                if(!connect(shop,&Shop::setAbove,this,&OverMapLogic::setAbove))
                    abort();
                if(!connect(shop,&Shop::add_to_inventory,this,&OverMapLogic::add_to_inventory_slotpair))
                    abort();
                if(!connect(shop,&Shop::remove_to_inventory,this,&OverMapLogic::remove_to_inventory_slotpair))
                    abort();
                if(!connect(shop,&Shop::addCash,this,&OverMapLogic::addCash))
                    abort();
                if(!connect(shop,&Shop::removeCash,this,&OverMapLogic::removeCash))
                    abort();
                if(!connect(shop,&Shop::showTip,this,&OverMapLogic::showTip))
                    abort();
            }
            shop->setVar(connexionManager,actualBot.name,skinId,items);
            setAbove(shop);
        }
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"sell")==0)
    {
        //open the inventory as a "sell" picker; picking an item sells it (price/2)
        //via objectSelection(ObjectType_Sell). No shop id: the server resolves the
        //shop from the faced bot tile (sellObject re-resolves server-side).
        selectObject(ObjectType_Sell);
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"heal")==0)
    {
        connexionManager->client->healAllMonsters();
        connexionManager->client->heal();
        //load_monsters();
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
                const std::pair<uint16_t,std::string> &quest=quests.at(index);
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
        setIG_dialog(QString::fromStdString(textToShow),QString::fromStdString(actualBot.name));
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
        setIG_dialog(QString::fromStdString(textToShow),QString::fromStdString(actualBot.name));
        return;
    }
    else if(strcmp(stepXml->Attribute("type"),"warehouse")==0)
    {
        const uint8_t skinId=resolveBotSkinId();
        if(warehouse==nullptr)
        {
            warehouse=new Warehouse();
            if(!connect(warehouse,&Warehouse::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(warehouse,&Warehouse::showTip,this,&OverMapLogic::showTip))
                abort();
        }
        warehouse->setVar(connexionManager,actualBot.name,skinId);
        setAbove(warehouse);
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
        uint16_t factoryId=stringtouint16(stepXml->Attribute("industry"),&ok);
        if(!ok)
        {
            showTip(tr("Industry called but wrong id").toStdString());
            return;
        }
        //industries are now per-map in BaseMap
        {
            CATCHCHALLENGER_TYPE_MAPID currentMapId=ccmap->mapController.currentMap();
            const CatchChallenger::CommonMap &logicalMap=QtDatapackClientLoader::datapackLoader->getMap(currentMapId);
            if(factoryId>=logicalMap.industries.size())
            {
                showTip(tr("The factory is not found").toStdString());
                return;
            }
        }
        // open factory screen
        {
            if(factory==nullptr)
            {
                factory=new Factory();
                if(!connect(factory,&Factory::setAbove,this,&OverMapLogic::setAbove))
                    abort();
                if(!connect(factory,&Factory::add_to_inventory,this,&OverMapLogic::add_to_inventory_slotpair))
                    abort();
                if(!connect(factory,&Factory::remove_to_inventory,this,&OverMapLogic::remove_to_inventory_slotpair))
                    abort();
                if(!connect(factory,&Factory::addCash,this,&OverMapLogic::addCash))
                    abort();
                if(!connect(factory,&Factory::removeCash,this,&OverMapLogic::removeCash))
                    abort();
                if(!connect(factory,&Factory::showTip,this,&OverMapLogic::showTip))
                    abort();
                if(!connect(factory,&Factory::appendReputationRewards,this,&OverMapLogic::appendReputationRewards))
                    abort();
            }
            factory->setVar(connexionManager,factoryId);
            setAbove(factory);
            connexionManager->client->getFactoryList();
        }
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
        //The zone-capture (TvT) wait screen is not ported to this interface yet.
        //Guards above already handle the common solo cases (no clan / disabled);
        //show an honest tip here instead of crashing (was abort()).
        showTip(tr("Zone capture is not available yet").toStdString());
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
        //botFights are now per-map in BaseMap
        CATCHCHALLENGER_TYPE_MAPID fightMapId=ccmap->mapController.currentMap();
        const CatchChallenger::CommonMap &fightLogicalMap=QtDatapackClientLoader::datapackLoader->getMap(fightMapId);
        if(fightLogicalMap.botFights.find(fightId)==fightLogicalMap.botFights.cend())
        {
            showTip(tr("Bot fight not found").toStdString());
            return;
        }
        if(connexionManager->client->haveBeatBot(fightMapId,fightId))
        {
            if(actualBot.step.find(step+1)!=actualBot.step.cend())
                goToBotStep(step+1);
            else
                showTip(tr("Already beaten!").toStdString());
            return;
        }
        connexionManager->client->requestFight();
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
std::vector<std::pair<uint16_t,std::string> > OverMapLogic::getQuestList() const
{
    std::vector<std::pair<uint16_t,std::string> > entryList;
    const std::vector<std::pair<CATCHCHALLENGER_TYPE_QUEST,bool> > quests=connexionManager->client->questsForBot(actualBotMapId,actualBot.botId);
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

bool OverMapLogic::tryValidateQuestStep(const uint16_t &questId, bool silent)
{
    std::vector<CatchChallenger::Api_protocol::ReputationLevelChange> reputationChanges;
    switch(connexionManager->client->validateQuestStep(static_cast<CATCHCHALLENGER_TYPE_QUEST>(questId),actualBotMapId,actualBot.botId,reputationChanges))
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

void OverMapLogic::showQuestText(const uint32_t &textId)
{
    std::string text;
    switch(QtDatapackClientLoader::datapackLoader->questText(questId,textId,*connexionManager->client,text))
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
    const std::string textToShow=CatchChallenger::ChatParsing::botTextToDisplay(text,connexionManager->client->botHaveQuest(actualBotMapId,actualBot.botId));
    setIG_dialog(QString::fromStdString(textToShow),QString::fromStdString(actualBot.name));
}

void OverMapLogic::on_IG_dialog_text_linkActivated(const QString &rawlink)
{
    IG_dialog_text_linkActivated(rawlink.toStdString());
}

void OverMapLogic::IG_dialog_text_linkActivated(const std::string &rawlink)
{
    setIG_dialog(QString());
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
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
            {
                showTip(tr("Quest not found").toStdString());
                index++;
                continue;
            }
            isInQuest=true;
            this->questId=questId;
            if(QtDatapackClientLoader::datapackLoader->has_questExtra(questId))
            {
                if(QtDatapackClientLoader::datapackLoader->get_questExtra(questId).autostep)
                {
                    int index=0;
                    bool ok;
                    do
                    {
                        ok=tryValidateQuestStep(questId);
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
            //open a name-entry popup; createClan() fires on accept (async, so we
            //return from the link loop here)
            if(textInput==nullptr)
            {
                textInput=new TextInput();
                if(!connect(textInput,&TextInput::setAbove,this,&OverMapLogic::setAbove))
                    abort();
                if(!connect(textInput,&TextInput::accepted,this,&OverMapLogic::clanNameEntered))
                    abort();
            }
            textInput->setVar(tr("Give the clan name"),tr("Clan name"),32);
            setAbove(textInput);
            return;
        }
        else if(link=="close")
            return;
        else if(link=="next_quest_step" && isInQuest)
        {
            tryValidateQuestStep(questId);
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

void OverMapLogic::updateDisplayedQuests()
{
/*    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    std::string html("<ul>");
    ui->questsList->clear();
    quests_to_id_graphical.clear();
    std::map<uint16_t, CatchChallenger::PlayerQuest>::const_iterator i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->questsExtra.find(i->first)!=QtDatapackClientLoader::datapackLoader->questsExtra.cend())
        {
            if(i->second.step>0)
            {
                QListWidgetItem * item=new QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::datapackLoader->questsExtra.at(i->first).name));
                quests_to_id_graphical[item]=i->first;
                ui->questsList->addItem(item);
            }
            if(i->second.step==0 || i->second.finish_one_time)
            {
                if(Ultimate::ultimate.isUltimate())
                {
                    html+="<li>";
                    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(i->first).repeatable)
                        html+=imagesInterfaceRepeatableString;
                    if(i->second.step>0)
                        html+=imagesInterfaceInProgressString;
                    html+=QtDatapackClientLoader::datapackLoader->questsExtra.at(i->first).name+"</li>";
                }
                else
                    html+="<li>"+QtDatapackClientLoader::datapackLoader->questsExtra.at(i->first).name+"</li>";
            }
        }
        ++i;
    }
    html+="</ul>";
    if(html=="<ul></ul>")
        html="<i>None</i>";
    ui->finishedQuests->setHtml(QString::fromStdString(html));
    on_questsList_itemSelectionChanged();*/
}
