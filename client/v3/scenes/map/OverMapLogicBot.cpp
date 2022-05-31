// Copyright 2021 CatchChallenger
#include <QDesktopServices>
#include <iostream>

#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Globals.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../entities/PlayerInfo.hpp"
#include "OverMapLogic.hpp"

using Scenes::OverMapLogic;
using std::placeholders::_1;

bool OverMapLogic::botHaveQuest(const uint16_t &botId) const {
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  // do the not started quest here
  if (QtDatapackClientLoader::GetInstance()->get_botToQuestStart().find(botId) ==
      QtDatapackClientLoader::GetInstance()->get_botToQuestStart().cend()) {
    std::cerr << "OverMapLogic::botHaveQuest(), botId not found: "
              << std::to_string(botId) << std::endl;
    return false;
  }
  const std::vector<uint8_t> &botQuests =
      QtDatapackClientLoader::GetInstance()->get_botToQuestStart().at(botId);
  unsigned int index = 0;
  while (index < botQuests.size()) {
    const uint16_t &questId = botQuests.at(index);
    const CatchChallenger::Quest &currentQuest =
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_quests().at(questId);
    if (playerInformations.quests.find(botQuests.at(index)) ==
        playerInformations.quests.cend()) {
      // quest not started
      if (haveStartQuestRequirement(currentQuest))
        return true;
      else {
      }  // have not the requirement
    } else {
      if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().find(botQuests.at(index)) ==
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().cend())
        qDebug() << "internal bug: have quest registred, but no quest found "
                    "with this id";
      else {
        if (playerInformations.quests.at(botQuests.at(index)).step == 0) {
          if (currentQuest.repeatable) {
            if (playerInformations.quests.at(botQuests.at(index))
                    .finish_one_time) {
              // quest already done but repeatable
              if (haveStartQuestRequirement(currentQuest))
                return true;
              else {
              }  // have not the requirement
            } else {
            }  // bug: can't be !finish_one_time && currentQuest.steps==0
          } else {
          }  // quest already done
        } else {
          const std::vector<uint16_t> &bots =
              currentQuest.steps
                  .at(playerInformations.quests.at(questId).step - 1)
                  .bots;
          if (vectorcontainsAtLeastOne(bots, botId))
            return true;  // in progress
          else {
          }  // Need got to another bot to progress, this it's just the starting
             // bot
        }
      }
    }
    index++;
  }
  // do the started quest here
  auto i = playerInformations.quests.begin();
  while (i != playerInformations.quests.cend()) {
    if (!vectorcontainsAtLeastOne(botQuests, i->first) && i->second.step > 0) {
      CatchChallenger::Quest currentQuest =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().at(i->first);
      auto bots = currentQuest.steps.at(i->second.step - 1).bots;
      if (vectorcontainsAtLeastOne(bots, botId))
        return true;  // in progress, but not the starting bot
      else {
      }  // it's another bot
    }
    ++i;
  }
  return false;
}

// bot
void OverMapLogic::goToBotStep(const uint8_t &step) {
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  lastStepUsed = step;
  isInQuest = false;
  if (actualBot.step.find(step) == actualBot.step.cend()) {
    showTip(tr("Error into the bot, report this error please").toStdString());
    return;
  }
  const tinyxml2::XMLElement *stepXml = actualBot.step.at(step);
  if (strcmp(stepXml->Attribute("type"), "text") == 0) {
#ifndef CATCHCHALLENGER_BOT
    const std::string &language =
        Language::language.getLanguage().toStdString();
#else
    const std::string language("en");
#endif
    auto text = stepXml->FirstChildElement("text");
    if (!language.empty() && language != "en") {
      while (text != NULL) {
        if (text->Attribute("lang") != NULL &&
            text->Attribute("lang") == language && text->GetText() != NULL) {
          std::string textToShow = text->GetText();
          textToShow = parseHtmlToDisplay(textToShow);
          npc_talk_->SetData(QString::fromStdString(textToShow),
                             QString::fromStdString(actualBot.name));
          AddChild(npc_talk_);
          return;
        }
        text = text->NextSiblingElement("text");
      }
    }
    text = stepXml->FirstChildElement("text");
    while (text != NULL) {
      if (text->Attribute("lang") == NULL ||
          text->Attribute("lang") == language)
        if (text->GetText() != NULL) {
          std::string textToShow = text->GetText();
          textToShow = parseHtmlToDisplay(textToShow);
          npc_talk_->SetData(QString::fromStdString(textToShow),
                             QString::fromStdString(actualBot.name));
          AddChild(npc_talk_);
          return;
        }
      text = text->NextSiblingElement("text");
    }
    showTip(tr("Bot text not found, report this error please").toStdString());
    return;
  } else if (strcmp(stepXml->Attribute("type"), "shop") == 0) {
    if (stepXml->Attribute("shop") == NULL) {
      showTip(tr("Shop called but missing informations").toStdString());
      return;
    }
    bool ok;
    uint16_t shopId = stringtouint16(stepXml->Attribute("shop"), &ok);
    if (!ok) {
      showTip(tr("Shop called but wrong id").toStdString());
      return;
    }
    if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_shops().find(shopId) ==
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_shops().cend()) {
      showTip(tr("Shop not found").toStdString());

      return;
    }
    if (shop_ == nullptr) {
      shop_ = Shop::Create();
      shop_->SetOnTransactionFinish(
          std::bind(&OverMapLogic::showTip, this, _1));
    }
    shop_->SetSeller(actualBot, shopId);
    AddChild(shop_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "sell") == 0) {
    if (stepXml->Attribute("shop") == NULL) {
      showTip(tr("Shop called but missing informations").toStdString());
      return;
    }
    bool ok;
    uint16_t shopId = stringtouint16(stepXml->Attribute("shop"), &ok);
    if (!ok) {
      showTip(tr("Shop called but wrong id").toStdString());
      return;
    }

    if (shop_ == nullptr) {
      shop_ = Shop::Create();
      shop_->SetOnTransactionFinish(
          std::bind(&OverMapLogic::showTip, this, _1));
    }
    shop_->SetMeAsSeller(shopId);
    AddChild(shop_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "learn") == 0) {
    if (learn_ == nullptr) {
      learn_ = Learn::Create();
    }
    AddChild(learn_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "heal") == 0) {
    connexionManager->client->healAllMonsters();
    connexionManager->client->heal();
    PlayerInfo::GetInstance()->Notify();
    showTip(tr("You are healed").toStdString());
    return;
  } else if (strcmp(stepXml->Attribute("type"), "quests") == 0) {
    std::string textToShow;
    const std::vector<std::pair<uint16_t, std::string> > &quests =
        OverMapLogic::getQuestList(actualBot.botId);
    if (step == 1) {
      if (quests.size() == 1) {
        const std::pair<uint16_t, std::string> &quest = quests.at(0);
        if (QtDatapackClientLoader::GetInstance()->get_questsExtra().at(quest.first)
                .autostep) {
          IG_dialog_text_linkActivated("quest_" + std::to_string(quest.first));
          return;
        }
      }
    }
    if (quests.empty()) {
      if (actualBot.step.find(step + 1) != actualBot.step.cend())
        on_IG_dialog_text_linkActivated("next");
      else
        textToShow +=
            tr("No quests at the moment or you don't meat the requirements")
                .toStdString();
    } else {
      textToShow += "<ul>";
      unsigned int index = 0;
      while (index < quests.size()) {
        const std::pair<uint32_t, std::string> &quest = quests.at(index);
        textToShow += QString("<li><a href=\"quest_%1\">%2</a></li>")
                          .arg(quest.first)
                          .arg(QString::fromStdString(quest.second))
                          .toStdString();
        index++;
      }
      if (quests.empty())
        textToShow +=
            tr("No quests at the moment or you don't meat the requirements")
                .toStdString();
      textToShow += "</ul>";
    }
    npc_talk_->SetData(QString::fromStdString(textToShow),
                       QString::fromStdString(actualBot.name));
    AddChild(npc_talk_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "clan") == 0) {
    std::string textToShow;
    if (playerInformations.clan == 0) {
      if (playerInformations.allow.find(CatchChallenger::ActionAllow_Clan) !=
          playerInformations.allow.cend())
        textToShow =
            QStringLiteral("<center><a href=\"clan_create\">%1</a></center>")
                .arg(tr("Clan create"))
                .toStdString();
      else
        textToShow =
            QStringLiteral("<center>You can't create your clan</center>")
                .toStdString();
    } else
      textToShow = QStringLiteral("<center>%1</center>")
                       .arg(tr("You are already into a clan. Use the clan "
                               "dongle into the player information."))
                       .toStdString();
    npc_talk_->SetData(QString::fromStdString(textToShow),
                       QString::fromStdString(actualBot.name));
    AddChild(npc_talk_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "warehouse") == 0) {
    if (warehouse_ == nullptr) {
      warehouse_ = Warehouse::Create();
    }
    AddChild(warehouse_);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "market") == 0) {
    std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
              << "market" << std::endl;
    /*ui->marketMonster->clear();
    ui->marketObject->clear();
    ui->marketOwnMonster->clear();
    ui->marketOwnObject->clear();
    ui->marketWithdraw->setVisible(false);
    ui->marketStat->setText(tr("In waiting of market list"));
    connexionManager->client->getMarketList();
    ui->stackedWidget->setCurrentWidget(ui->page_market);
    return;*/
    abort();
  } else if (strcmp(stepXml->Attribute("type"), "industry") == 0) {
    if (stepXml->Attribute("industry") == NULL) {
      showTip(tr("Industry called but missing informations").toStdString());
      return;
    }

    bool ok;
    uint16_t factoryId = stringtouint16(stepXml->Attribute("industry"), &ok);
    if (!ok) {
      showTip(tr("Industry called but wrong id").toStdString());
      return;
    }
    if (CatchChallenger::CommonDatapack::commonDatapack.get_industriesLink().find(
            factoryId) ==
        CatchChallenger::CommonDatapack::commonDatapack.get_industriesLink().cend()) {
      showTip(tr("The factory is not found").toStdString());
      return;
    }
    if (industry_ == nullptr) {
      industry_ = Industry::Create();
    }
    AddChild(industry_);
    industry_->SetBot(actualBot, factoryId);
    return;
  } else if (strcmp(stepXml->Attribute("type"), "zonecapture") == 0) {
    if (zonecatch_ == nullptr) {
      zonecatch_ = ZoneCatch::Create();
    }

    if (stepXml->Attribute("zone") == NULL) {
      showTip(tr("Missing attribute for the step").toStdString());
      return;
    }
    if (playerInformations.clan == 0) {
      showTip(
          tr("You can't try capture if you are not in a clan").toStdString());
      return;
    }
    if (!zonecatch_->IsNextCityCatchActive()) {
      showTip(tr("City capture disabled").toStdString());
      return;
    }

    const std::string zone = stepXml->Attribute("zone");
    AddChild(zonecatch_);
    zonecatch_->SetZone(zone);
    zonecatchName = zonecatch_->GetZoneName();
  } else if (strcmp(stepXml->Attribute("type"), "fight") == 0) {
    if (stepXml->Attribute("fightid") == NULL) {
      showTip(tr("Bot step missing data error, repport this error please")
                  .toStdString());
      return;
    }
    bool ok;
    const uint16_t &fightId =
        stringtouint16(stepXml->Attribute("fightid"), &ok);
    if (!ok) {
      showTip(tr("Bot step wrong data type error, repport this error please")
                  .toStdString());
      return;
    }
    if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_botFights().find(fightId) ==
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_botFights().cend()) {
      showTip(tr("Bot fight not found").toStdString());
      return;
    }
    if (connexionManager->client->haveBeatBot(fightId)) {
      if (actualBot.step.find(step + 1) != actualBot.step.cend())
        goToBotStep(step + 1);
      else
        showTip(tr("Already beaten!").toStdString());
      return;
    }
    connexionManager->client->requestFight(fightId);
    botFight(fightId);
    return;
  } else {
    showTip(tr("Bot step type error, repport this error please").toStdString());
    return;
  }
}

bool OverMapLogic::tryValidateQuestStep(const uint16_t &questId,
                                        const uint16_t &botId, bool silent) {
  if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests()
          .find(questId) ==
      CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests()
          .cend()) {
    if (!silent) showTip(tr("Quest not found").toStdString());
    return false;
  }
  const CatchChallenger::Quest &quest =
      CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests()
          .at(questId);
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();

  if (playerInformations.quests.find(questId) ==
      playerInformations.quests.cend()) {
    // start for the first time the quest
    if (vectorcontainsAtLeastOne(quest.steps.at(0).bots, botId) &&
        haveStartQuestRequirement(quest)) {
      connexionManager->client->startQuest(questId);
      startQuest(quest);
      updateDisplayedQuests();
      return true;
    } else {
      if (!silent)
        showTip(tr("You don't have the requirement to start this quest")
                    .toStdString());
      return false;
    }
  } else if (playerInformations.quests.at(questId).step == 0) {
    // start again the quest if can be repeated
    if (quest.repeatable &&
        vectorcontainsAtLeastOne(quest.steps.at(0).bots, botId) &&
        haveStartQuestRequirement(quest)) {
      connexionManager->client->startQuest(questId);
      startQuest(quest);
      updateDisplayedQuests();
      return true;
    } else {
      if (!silent)
        showTip(tr("You don't have the requirement to start this quest")
                    .toStdString());
      return false;
    }
  }
  if (!haveNextStepQuestRequirements(quest)) {
    if (!silent)
      showTip(tr("You don't have the requirement to continue this quest")
                  .toStdString());
    return false;
  }
  if (playerInformations.quests.at(questId).step >= (quest.steps.size())) {
    if (!silent)
      showTip(tr("You have finish the quest <b>%1</b>")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::GetInstance()->get_questsExtra()
                          .at(questId)
                          .name))
                  .toStdString());
    connexionManager->client->finishQuest(questId);
    nextStepQuest(quest);
    updateDisplayedQuests();
    return true;
  }
  if (vectorcontainsAtLeastOne(
          quest.steps.at(playerInformations.quests.at(questId).step).bots,
          botId)) {
    if (!silent) showTip(tr("You need talk to another bot").toStdString());
    return false;
  }
  connexionManager->client->nextQuestStep(questId);
  nextStepQuest(quest);
  updateDisplayedQuests();
  return true;
}

void OverMapLogic::showQuestText(const uint32_t &textId) {
  if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(questId) ==
      QtDatapackClientLoader::GetInstance()->get_questsExtra().cend()) {
    qDebug() << QStringLiteral("No quest text for this quest: %1").arg(questId);
    showTip(tr("No quest text for this quest").toStdString());
    return;
  }
  const QtDatapackClientLoader::QuestExtra &questExtra =
      QtDatapackClientLoader::GetInstance()->get_questsExtra().at(questId);
  if (questExtra.text.find(textId) == questExtra.text.cend()) {
    qDebug() << "No quest text entry point";
    showTip(tr("No quest text entry point").toStdString());
    return;
  }

  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  uint8_t stepQuest = 0;
  if (playerInformations.quests.find(questId) !=
      playerInformations.quests.cend())
    stepQuest = playerInformations.quests.at(questId).step - 1;
  std::string text = tr("No text found").toStdString();
  const QtDatapackClientLoader::QuestTextExtra &questTextExtra =
      questExtra.text.at(textId);
  unsigned int index = 0;
  while (index < questTextExtra.texts.size()) {
    const QtDatapackClientLoader::QuestStepWithConditionExtra &e =
        questTextExtra.texts.at(index);
    unsigned int indexCond = 0;
    while (indexCond < e.conditions.size()) {
      const QtDatapackClientLoader::QuestConditionExtra &condition =
          e.conditions.at(indexCond);
      switch (condition.type) {
        case QtDatapackClientLoader::QuestCondition_queststep:
          if ((stepQuest + 1) != condition.value)
            indexCond = e.conditions.size() + 999;  // not validated condition
          break;
        case QtDatapackClientLoader::QuestCondition_haverequirements: {
          const CatchChallenger::Quest &quest =
              CatchChallenger::CommonDatapackServerSpec::
                  commonDatapackServerSpec.get_quests().at(questId);
          if (!haveNextStepQuestRequirements(quest))
            indexCond = e.conditions.size() + 999;  // not validated condition
        } break;
        default:
          break;
      }
      indexCond++;
    }
    if (indexCond == e.conditions.size()) {
      text = e.text;
      break;
    }
    index++;
  }
  std::string textToShow = parseHtmlToDisplay(text);
  npc_talk_->SetData(QString::fromStdString(textToShow),
                     QString::fromStdString(actualBot.name));
  AddChild(npc_talk_);
}

bool OverMapLogic::haveNextStepQuestRequirements(
    const CatchChallenger::Quest &quest) const {
#ifdef DEBUG_CLIENT_QUEST
  qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1")
                  .arg(questId);
#endif
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  if (playerInformations.quests.find(quest.id) ==
      playerInformations.quests.cend()) {
    qDebug() << "step out of range for: " << quest.id;
    return false;
  }
  uint8_t step = playerInformations.quests.at(quest.id).step;
  if (step <= 0 || step > quest.steps.size()) {
    qDebug() << "step out of range for: " << quest.id;
    return false;
  }
#ifdef DEBUG_CLIENT_QUEST
  qDebug() << QStringLiteral(
                  "haveNextStepQuestRequirements for quest: %1, step: %2")
                  .arg(questId)
                  .arg(step);
#endif
  const CatchChallenger::Quest::StepRequirements &requirements =
      quest.steps.at(step - 1).requirements;
  unsigned int index = 0;
  while (index < requirements.items.size()) {
    const CatchChallenger::Quest::Item &item = requirements.items.at(index);
    if (connexionManager->client->itemQuantity(item.item) < item.quantity) {
#ifdef DEBUG_CLIENT_QUEST
      qDebug() << "quest requirement, have not the quantity for the item: "
               << item.item;
#endif
      return false;
    }
    index++;
  }
  index = 0;
  while (index < requirements.fightId.size()) {
    const uint16_t &fightId = requirements.fightId.at(index);
    if (!connexionManager->client->haveBeatBot(fightId)) return false;
    index++;
  }
  return true;
}

bool OverMapLogic::haveStartQuestRequirement(
    const CatchChallenger::Quest &quest) const {
#ifdef DEBUG_CLIENT_QUEST
  qDebug() << "check quest requirement for: " << quest.id;
#endif
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  if (playerInformations.quests.find(quest.id) !=
      playerInformations.quests.cend()) {
    const CatchChallenger::PlayerQuest &playerquest =
        playerInformations.quests.at(quest.id);
    if (playerquest.step != 0) return false;
    if (playerquest.finish_one_time && !quest.repeatable) return false;
  }
  unsigned int index = 0;
  while (index < quest.requirements.quests.size()) {
    const uint16_t &questId = quest.requirements.quests.at(index).quest;
    if ((playerInformations.quests.find(questId) ==
             playerInformations.quests.cend() &&
         !quest.requirements.quests.at(index).inverse) ||
        (playerInformations.quests.find(questId) !=
             playerInformations.quests.cend() &&
         quest.requirements.quests.at(index).inverse))
      return false;
    if (playerInformations.quests.find(questId) !=
        playerInformations.quests.cend())
      return false;
    const CatchChallenger::PlayerQuest &playerquest =
        playerInformations.quests.at(quest.id);
    if (!playerquest.finish_one_time) return false;
    index++;
  }
  return connexionManager->client->haveReputationRequirements(
      quest.requirements.reputation);
}

void OverMapLogic::on_IG_dialog_text_linkActivated(const QString &rawlink) {
  IG_dialog_text_linkActivated(rawlink.toStdString());
}

void OverMapLogic::IG_dialog_text_linkActivated(const std::string &rawlink) {
  RemoveChild(npc_talk_);
  std::vector<std::string> stringList = stringsplit(rawlink, ';');
  unsigned int index = 0;
  while (index < stringList.size()) {
    const std::string &link = stringList.at(index);
    qDebug() << "parsed link to use: " << QString::fromStdString(link);
    if (stringStartWith(link, "http://") || stringStartWith(link, "https://")) {
      if (!QDesktopServices::openUrl(QUrl(QString::fromStdString(link))))
        showTip(QStringLiteral("Unable to open the url: %1")
                    .arg(QString::fromStdString(link))
                    .toStdString());
      index++;
      continue;
    }
    bool ok;
    if (stringStartWith(link, "quest_")) {
      std::string tempLink = link;
      stringreplaceOne(tempLink, "quest_", "");
      const uint16_t &questId = stringtouint16(tempLink, &ok);
      if (!ok) {
        showTip(QStringLiteral("Unable to open the link: %1")
                    .arg(QString::fromStdString(link))
                    .toStdString());
        index++;
        continue;
      }
      if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().find(questId) ==
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().cend()) {
        showTip(tr("Quest not found").toStdString());
        index++;
        continue;
      }
      isInQuest = true;
      this->questId = questId;
      if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(questId) !=
          QtDatapackClientLoader::GetInstance()->get_questsExtra().cend()) {
        if (QtDatapackClientLoader::GetInstance()->get_questsExtra().at(questId)
                .autostep) {
          int index = 0;
          bool ok;
          do {
            ok = tryValidateQuestStep(questId, actualBot.botId);
            index++;
          } while (ok && index < 99);
          if (index == 99) {
            emit error("Infinity loop into autostep for quest: " +
                       std::to_string(questId));
            return;
          }
        }
        showQuestText(1);
        index++;
        continue;
      } else {
        showTip(tr("Quest extra not found").toStdString());
        index++;
        continue;
      }
    } else if (link == "clan_create") {
      bool is_allow = PlayerInfo::GetInstance()->IsAllowed(
          CatchChallenger::ActionAllow::ActionAllow_Clan);

      if (is_allow) {
        if (PlayerInfo::GetInstance()->GetInformationRO().clan == 0) {
          Globals::GetInputDialog()->ShowInputText(
              tr("CLAN"), tr("Give the clan name"),
              [&](QString text) {
                if (!text.isEmpty()) {
                  actionClan.push_back(ActionClan_Create);
                  connexionManager->client->createClan(text.toStdString());
                }
              },
              QString());
        } else {
          showTip(tr("You are already in a clan").toStdString());
        }
      } else {
        showTip(tr("You are not allowed to create a clan").toStdString());
      }
      index++;
      continue;
      abort();
    } else if (link == "close")
      return;
    else if (link == "next_quest_step" && isInQuest) {
      tryValidateQuestStep(questId, actualBot.botId);
      index++;
      continue;
    } else if (link == "next" && lastStepUsed >= 1) {
      goToBotStep(lastStepUsed + 1);
      index++;
      continue;
    }
    uint8_t step = stringtouint8(link, &ok);
    if (!ok) {
      showTip(QStringLiteral("Unable to open the link: %1")
                  .arg(QString::fromStdString(link))
                  .toStdString());
      index++;
      continue;
    }
    if (isInQuest) {
      showQuestText(step);
      index++;
      continue;
    }
    goToBotStep(step);
    index++;
  }
}

bool OverMapLogic::startQuest(const CatchChallenger::Quest &quest) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
  if (playerInformations.quests.find(quest.id) ==
      playerInformations.quests.cend()) {
    playerInformations.quests[quest.id].step = 1;
    playerInformations.quests[quest.id].finish_one_time = false;
  } else
    playerInformations.quests[quest.id].step = 1;
  return true;
}

std::vector<std::pair<uint16_t, std::string> > OverMapLogic::getQuestList(
    const uint16_t &botId) const {
  const CatchChallenger::Player_private_and_public_informations
      &playerInformations =
          connexionManager->client->get_player_informations_ro();
  std::vector<std::pair<uint16_t, std::string> > entryList;
  std::pair<uint16_t, std::string> oneEntry;
  // do the not started quest here
  std::vector<uint8_t> botQuests =
      QtDatapackClientLoader::GetInstance()->get_botToQuestStart().at(botId);
  unsigned int index = 0;
  while (index < botQuests.size()) {
    const uint16_t &questId = botQuests.at(index);
#ifdef CATCHCHALLENGER_EXTRA_CHECK
    if (questId != botQuests.at(index))
      qDebug() << "cast error for questId at OverMapLogic::getQuestList()";
#endif
    const CatchChallenger::Quest &currentQuest =
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_quests().at(questId);
    if (playerInformations.quests.find(botQuests.at(index)) ==
        playerInformations.quests.cend()) {
      // quest not started
      if (haveStartQuestRequirement(currentQuest)) {
        oneEntry.first = questId;
        if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(questId) !=
            QtDatapackClientLoader::GetInstance()->get_questsExtra().cend())
          oneEntry.second =
              QtDatapackClientLoader::GetInstance()->get_questsExtra().at(questId)
                  .name;
        else {
          qDebug() << "internal bug: quest extra not found";
          oneEntry.second = "???";
        }
        entryList.push_back(oneEntry);
      } else {
      }  // have not the requirement
    } else {
      if (CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().find(botQuests.at(index)) ==
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().cend())
        qDebug() << "internal bug: have quest registred, but no quest found "
                    "with this id";
      else {
        if (playerInformations.quests.at(botQuests.at(index)).step == 0) {
          if (currentQuest.repeatable) {
            if (playerInformations.quests.at(botQuests.at(index))
                    .finish_one_time) {
              // quest already done but repeatable
              if (haveStartQuestRequirement(currentQuest)) {
                oneEntry.first = questId;
                if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(
                        questId) !=
                    QtDatapackClientLoader::GetInstance()->get_questsExtra().cend())
                  oneEntry.second =
                      QtDatapackClientLoader::GetInstance()->get_questsExtra()
                          .at(questId)
                          .name;
                else {
                  qDebug() << "internal bug: quest extra not found";
                  oneEntry.second = "???";
                }
                entryList.push_back(oneEntry);
              } else {
              }  // have not the requirement
            } else {
            }  // bug: can't be !finish_one_time && currentQuest.steps==0
          } else {
          }  // quest already done
        } else {
          auto bots = currentQuest.steps
                          .at(playerInformations.quests.at(questId).step - 1)
                          .bots;
          if (vectorcontainsAtLeastOne(bots, botId)) {
            oneEntry.first = questId;
            if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(
                    questId) !=
                QtDatapackClientLoader::GetInstance()->get_questsExtra().cend())
              oneEntry.second =
                  tr("%1 (in progress)")
                      .arg(QString::fromStdString(
                          QtDatapackClientLoader::GetInstance()->get_questsExtra()
                              .at(questId)
                              .name))
                      .toStdString();
            else {
              qDebug() << "internal bug: quest extra not found";
              oneEntry.second = tr("??? (in progress)").toStdString();
            }
            entryList.push_back(oneEntry);
          } else {
          }  // Need got to another bot to progress, this it's just the starting
             // bot
        }
      }
    }
    index++;
  }
  // do the started quest here
  auto i = playerInformations.quests.begin();
  while (i != playerInformations.quests.cend()) {
    if (!vectorcontainsAtLeastOne(botQuests, i->first) && i->second.step > 0) {
      CatchChallenger::Quest currentQuest =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().at(i->first);
      auto bots = currentQuest.steps.at(i->second.step - 1).bots;
      if (vectorcontainsAtLeastOne(bots, botId)) {
        // in progress, but not the starting bot
        oneEntry.first = i->first;
        oneEntry.second =
            tr("%1 (in progress)")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::GetInstance()->get_questsExtra()
                        .at(i->first)
                        .name))
                .toStdString();
        entryList.push_back(oneEntry);
      } else {
      }  // it's another bot
    }
    ++i;
  }
  return entryList;
}

std::string OverMapLogic::parseHtmlToDisplay(const std::string &htmlContent) {
  QString newContent = QString::fromStdString(htmlContent);
#ifdef NOREMOTE
  QRegularExpression remote(
      QRegularExpression::escape("<span class=\"remote\">") + ".*" +
      QRegularExpression::escape("</span>"));
  remote.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
  newContent.remove(remote);
#endif
  if (!botHaveQuest(actualBot.botId))  // if have not quest
  {
    QRegularExpression quest(
        QRegularExpression::escape("<span class=\"quest\">") + ".*" +
        QRegularExpression::escape("</span>"));
    quest.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    newContent.remove(quest);
  }
  newContent.replace("href=\"http", "style=\"color:#BB9900;\" href=\"http",
                     Qt::CaseInsensitive);
  newContent.replace(QRegularExpression("(href=\"http[^>]+>[^<]+)</a>"),
                     "\\1 <img src=\":/CC/images/link.png\" alt=\"\" /></a>");
  return newContent.toStdString();
}

bool OverMapLogic::nextStepQuest(const CatchChallenger::Quest &quest) {
  CatchChallenger::Player_private_and_public_informations &playerInformations =
      connexionManager->client->get_player_informations();
#ifdef DEBUG_CLIENT_QUEST
  qDebug() << "drop quest step requirement for: " << quest.id;
#endif
  if (playerInformations.quests.find(quest.id) ==
      playerInformations.quests.cend()) {
    qDebug() << "step out of range for: " << quest.id;
    return false;
  }
  uint8_t step = playerInformations.quests.at(quest.id).step;
  if (step <= 0 || step > quest.steps.size()) {
    qDebug() << "step out of range for: " << quest.id;
    return false;
  }
  const CatchChallenger::Quest::StepRequirements &requirements =
      quest.steps.at(step - 1).requirements;
  unsigned int index = 0;
  while (index < requirements.items.size()) {
    const CatchChallenger::Quest::Item &item = requirements.items.at(index);
    std::unordered_map<uint16_t, uint32_t> items;
    items[item.item] = item.quantity;
    remove_to_inventory(items);
    index++;
  }
  playerInformations.quests[quest.id].step++;
  if (playerInformations.quests.at(quest.id).step > quest.steps.size()) {
#ifdef DEBUG_CLIENT_QUEST
    qDebug() << "finish the quest: " << quest.id;
#endif
    playerInformations.quests[quest.id].step = 0;
    playerInformations.quests[quest.id].finish_one_time = true;
    index = 0;
    while (index < quest.rewards.reputation.size()) {
      appendReputationPoint(
          CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
              .at(quest.rewards.reputation.at(index).reputationId)
              .name,
          quest.rewards.reputation.at(index).point);
      index++;
    }
    // show_reputation();
    index = 0;
    while (index < quest.rewards.allow.size()) {
      playerInformations.allow.insert(quest.rewards.allow.at(index));
      index++;
    }
  }
  return true;
}

void OverMapLogic::updateDisplayedQuests() {
  /*    const CatchChallenger::Player_private_and_public_informations
     &playerInformations=client->get_player_informations_ro(); std::string
     html("<ul>"); ui->questsList->clear(); quests_to_id_graphical.clear(); auto
     i=playerInformations.quests.begin();
      while(i!=playerInformations.quests.cend())
      {
          if(QtDatapackClientLoader::GetInstance()->questsExtra.find(i->first)!=QtDatapackClientLoader::GetInstance()->questsExtra.cend())
          {
              if(i->second.step>0)
              {
                  QListWidgetItem * item=new
     QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::GetInstance()->questsExtra.at(i->first).name));
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
                      html+=QtDatapackClientLoader::GetInstance()->questsExtra.at(i->first).name+"</li>";
                  }
                  else
                      html+="<li>"+QtDatapackClientLoader::GetInstance()->questsExtra.at(i->first).name+"</li>";
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
