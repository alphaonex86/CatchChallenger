// Copyright 2021 CatchChallenger
#include "SubServer.hpp"

#include <algorithm>
#include <iostream>
#include <string>

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../core/StackedScene.hpp"
#include "Loading.hpp"
#include "SubServerItem.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

using Scenes::SubServer;
using std::placeholders::_1;

SubServer::SubServer() {
  connection_ = ConnectionManager::GetInstance();
  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< QtDatapackClientLoader::datapackLoader << std::endl;
  connection_->SetOnLogged(std::bind(&SubServer::OnLogged, this, _1));
  auto loader = Loading::GetInstance();
  loader->Reset();
  AddChild(loader);
  loader->SetNotifier(connection_);

  is_loaded_ = false;
  character_selector_ = nullptr;
}

SubServer::~SubServer() {
  delete wdialog;
  delete server_select;
  delete back;

  if (character_selector_) {
    delete character_selector_;
    character_selector_ = nullptr;
  }
}

SubServer *SubServer::Create() { return new (std::nothrow) SubServer(); }

void SubServer::Initialize() {
  wdialog = Sprite::Create(":/CC/images/interface/message.png", this);
  serverList = UI::ListView::Create(this);
  serverList->SetItemSpacing(5);

  server_select = UI::Button::Create(":/CC/images/interface/next.png", this);
  back = UI::Button::Create(":/CC/images/interface/back.png", this);
  server_select->SetEnabled(false);

  averagePlayedTime = 0;
  averageLastConnect = 0;

  server_select->SetOnClick(std::bind(&SubServer::server_select_clicked, this));
  back->SetOnClick(std::bind(&SubServer::backMulti, this));
  server_selected_ = -1;

  newLanguage();
}

void SubServer::backMulti() {
  static_cast<StackedScene *>(Parent())->PopForeground();
}

void SubServer::server_select_clicked() {
  if (character_selector_ == nullptr) {
    character_selector_ = CharacterSelector::Create();
  }
  static_cast<StackedScene *>(Parent())->PushForeground(character_selector_);

  character_selector_->connectToSubServer(server_selected_, characters_);
}

void SubServer::newLanguage() {}

void SubServer::OnScreenResize() {
  if (!is_loaded_) return;
  unsigned int space = 10;
  unsigned int fontSize = 20;
  unsigned int multiItemH = 100;
  if (BoundingRect().height() > 300) fontSize = BoundingRect().height() / 6;
  if (BoundingRect().width() < 600 || BoundingRect().height() < 600) {
    server_select->SetSize(56, 62);
    back->SetSize(56, 62);
    multiItemH = 50;
  } else if (BoundingRect().width() < 900 || BoundingRect().height() < 600) {
    server_select->SetSize(84, 93);
    back->SetSize(84, 93);
    multiItemH = 75;
  } else {
    space = 30;
    server_select->SetSize(84, 93);
    back->SetSize(84, 93);
  }

  unsigned int tWidthTButton = server_select->Width() + space + back->Width();
  unsigned int y = BoundingRect().height() - space - server_select->Height();
  unsigned int tXTButton = BoundingRect().width() / 2 - tWidthTButton / 2;
  back->SetPos(tXTButton, y);
  server_select->SetPos(tXTButton + back->Width() + space, y);

  y = BoundingRect().height() - space - server_select->Height();
  int height =
      BoundingRect().height() - space - server_select->Height() - space - space;
  int width = 0;
  if ((unsigned int)BoundingRect().width() < (800 + space * 2)) {
    width = BoundingRect().width() - space * 2;
    wdialog->SetPos(space, space);
  } else {
    width = 800;
    wdialog->SetPos(BoundingRect().width() / 2 - width / 2, space);
  }
  wdialog->Strech(46, width, height);
  serverList->SetPos(wdialog->X() + space, wdialog->Y() + space);
  serverList->SetSize(wdialog->Width() - space - space,
                      wdialog->Height() - space - space);
}

void SubServer::OnEnter() {
  if (!is_loaded_) return;
  server_select->RegisterEvents();
  back->RegisterEvents();
  serverList->RegisterEvents();
}

void SubServer::OnExit() {
  if (!is_loaded_) return;
  server_select->UnRegisterEvents();
  back->UnRegisterEvents();
  serverList->UnRegisterEvents();
}

void SubServer::itemSelectionChanged(Node *node) {
  server_selected_ = node->Data(99);
  auto items = serverList->Items();
  for (auto it = begin(items); it != end(items); it++) {
    if ((*it) == node) continue;
    static_cast<SubServerItem *>((*it))->SetSelected(false);
  }

  server_select->SetEnabled(true);
}

void SubServer::logged(
    std::vector<CatchChallenger::ServerFromPoolForDisplay> servers) {
  // do the grouping for characterGroup count
  {
    serverByCharacterGroup.clear();
    unsigned int index = 0;
    uint8_t serverByCharacterGroupTempIndexToDisplay = 1;
    while (index < servers.size()) {
      const CatchChallenger::ServerFromPoolForDisplay &server =
          servers.at(index);
      if (server.charactersGroupIndex > servers.size()) abort();
      if (serverByCharacterGroup.find(server.charactersGroupIndex) !=
          serverByCharacterGroup.cend()) {
        serverByCharacterGroup[server.charactersGroupIndex].first++;
      } else {
        serverByCharacterGroup[server.charactersGroupIndex].first = 1;
        serverByCharacterGroup[server.charactersGroupIndex].second =
            serverByCharacterGroupTempIndexToDisplay;
        serverByCharacterGroupTempIndexToDisplay++;
      }
      index++;
    }
  }

  // clear and determine what kind of view
  serverList->Clear();
  CatchChallenger::LogicialGroup logicialGroup =
      connection_->client->getLogicialGroup();
  bool fullView = true;
  if (servers.size() > 10) fullView = false;
  const uint64_t &current__date = QDateTime::currentDateTime().toTime_t();

  // reload, bug if before init
  if (icon_server_list_star1.isNull()) {
    SubServer::icon_server_list_star1 =
        ":/CC/images/interface/server_list/star1.png";
    if (SubServer::icon_server_list_star1.isNull()) abort();
    SubServer::icon_server_list_star2 =
        ":/CC/images/interface/server_list/star2.png";
    SubServer::icon_server_list_star3 =
        ":/CC/images/interface/server_list/star3.png";
    SubServer::icon_server_list_star4 =
        ":/CC/images/interface/server_list/star4.png";
    SubServer::icon_server_list_star5 =
        ":/CC/images/interface/server_list/star5.png";
    SubServer::icon_server_list_star6 =
        ":/CC/images/interface/server_list/star6.png";
    SubServer::icon_server_list_stat1 =
        ":/CC/images/interface/server_list/stat1.png";
    SubServer::icon_server_list_stat2 =
        ":/CC/images/interface/server_list/stat2.png";
    SubServer::icon_server_list_stat3 =
        ":/CC/images/interface/server_list/stat3.png";
    SubServer::icon_server_list_stat4 =
        ":/CC/images/interface/server_list/stat4.png";
    SubServer::icon_server_list_bug =
        ":/CC/images/interface/server_list/bug.png";
    if (SubServer::icon_server_list_bug.isNull()) abort();
    icon_server_list_color.push_back(":/CC/images/colorflags/0.png");
    icon_server_list_color.push_back(":/CC/images/colorflags/1.png");
    icon_server_list_color.push_back(":/CC/images/colorflags/2.png");
    icon_server_list_color.push_back(":/CC/images/colorflags/3.png");
  }
  // do the average value
  {
    averagePlayedTime = 0;
    averageLastConnect = 0;
    int entryCount = 0;
    unsigned int index = 0;
    while (index < servers.size()) {
      const CatchChallenger::ServerFromPoolForDisplay &server =
          servers.at(index);
      if (server.playedTime > 0 && server.lastConnect <= current__date) {
        averagePlayedTime += server.playedTime;
        averageLastConnect += server.lastConnect;
        entryCount++;
      }
      index++;
    }
    if (entryCount > 0) {
      averagePlayedTime /= entryCount;
      averageLastConnect /= entryCount;
    }
  }
  addToServerList(logicialGroup, current__date, fullView, 0);
  if (servers.size() == 1) {
    server_selected_ = logicialGroup.servers.at(0).serverOrdenedListIndex;
    server_select_clicked();
  }
}

bool CatchChallenger::ServerFromPoolForDisplay::operator<(
    const ServerFromPoolForDisplay &serverFromPoolForDisplay) const {
  if (serverFromPoolForDisplay.uniqueKey < this->uniqueKey)
    return true;
  else
    return false;
}

void SubServer::addToServerList(CatchChallenger::LogicialGroup &logicialGroup,
                                const uint64_t &currentDate,
                                const bool &fullView, uint8_t level) {
  if (connection_->client->getServerOrdenedList().empty())
    std::cerr
        << "SubServer::addToServerList(): client->serverOrdenedList.empty()"
        << std::endl;
  if (level > 0) {
    //TODO(lanstat): investigate why this abort the app
    auto header = UI::Label::Create();
    header->SetPixelSize(18 - (level - 1) * 4);
    //header->SetText(logicialGroup.name);
    serverList->AddItem(header);
  }
  {
    // to order the group
    std::vector<std::string> keys;
    for (const auto &n : logicialGroup.logicialGroupList)
      keys.push_back(n.first);
    qSort(keys);
    // list the group
    unsigned int index = 0;
    while (index < keys.size()) {
      addToServerList(*logicialGroup.logicialGroupList[keys.at(index)],
                      currentDate, fullView, level + 1);
      index++;
    }
  }
  {
    qSort(logicialGroup.servers);
    // list the server
    unsigned int index = 0;
    while (index < logicialGroup.servers.size()) {
      const CatchChallenger::ServerFromPoolForDisplay &server =
          logicialGroup.servers.at(index);
      auto *itemServer = SubServerItem::Create();
      std::string text;
      std::string groupText;
      if (serverByCharacterGroup.size() > 1) {
        const uint8_t groupInt =
            serverByCharacterGroup.at(server.charactersGroupIndex).second;
        // comment the if to always show it
        if (groupInt >= icon_server_list_color.size())
          groupText = QStringLiteral(" (%1)").arg(groupInt).toStdString();
        // itemServer->SetSubTitle(tr("Server group: %1, UID: %2")
        //.arg(groupInt)
        //.arg(server.uniqueKey));
        if (!icon_server_list_color.empty())
          itemServer->SetIcon(icon_server_list_color.at(
              groupInt % icon_server_list_color.size()));
      }
      std::string name = server.name;
      if (name.empty()) name = tr("Default server").toStdString();
      if (fullView) {
        text = name + groupText;
        if (server.playedTime > 0) {
          if (!server.description.empty())
            text +=
                " " + tr("%1 played")
                          .arg(QString::fromStdString(
                              CatchChallenger::FacilityLibClient::timeToString(
                                  server.playedTime)))
                          .toStdString();
          else
            text +=
                "\n" + tr("%1 played")
                           .arg(QString::fromStdString(
                               CatchChallenger::FacilityLibClient::timeToString(
                                   server.playedTime)))
                           .toStdString();
        }
        if (!server.description.empty()) text += "\n" + server.description;
      } else {
        if (server.description.empty())
          text = name + groupText;
        else
          text = name + groupText + " - " + server.description;
      }
      itemServer->SetTitle(QString::fromStdString(text));

      // do the icon here
      if (server.playedTime > 5 * 365 * 24 * 3600) {
        itemServer->SetIcon(SubServer::icon_server_list_bug);
        itemServer->SetSubTitle(tr("Played time greater than 5y, bug?"));
      } else if (server.lastConnect > 0 && server.lastConnect < 1420070400) {
        itemServer->SetIcon(SubServer::icon_server_list_bug);
        itemServer->SetSubTitle(tr("Played before 2015, bug?"));
      } else if (server.maxPlayer <= 65533 &&
                 (server.maxPlayer < server.currentPlayer ||
                  server.maxPlayer == 0)) {
        itemServer->SetIcon(SubServer::icon_server_list_bug);
        if (server.maxPlayer < server.currentPlayer)
          itemServer->SetSubTitle(tr("maxPlayer<currentPlayer"));
        else
          itemServer->SetSubTitle(tr("maxPlayer==0"));
      } else if (server.playedTime > 0 || server.lastConnect > 0) {
        uint64_t dateDiff = 0;
        if (currentDate > server.lastConnect)
          dateDiff = currentDate - server.lastConnect;
        if (server.playedTime > 24 * 3600 * 31) {
          if (dateDiff < 24 * 3600) {
            itemServer->SetStar(SubServer::icon_server_list_star1);
            itemServer->SetSubTitle(
                tr("Played time greater than 24h, last "
                   "connect in this last 24h"));
          } else {
            itemServer->SetStar(SubServer::icon_server_list_star2);
            itemServer->SetSubTitle(
                tr("Played time greater than 24h, last "
                   "connect not in this last 24h"));
          }
        } else if (server.lastConnect < averageLastConnect) {
          if (server.playedTime < averagePlayedTime) {
            itemServer->SetStar(SubServer::icon_server_list_star3);
            itemServer->SetSubTitle(
                tr("Into the more recent server used, "
                   "out of the most used server"));
          } else {
            itemServer->SetStar(SubServer::icon_server_list_star4);
            itemServer->SetSubTitle(
                tr("Into the more recent server used, "
                   "into the most used server"));
          }
        } else {
          if (server.playedTime < averagePlayedTime) {
            itemServer->SetStar(SubServer::icon_server_list_star5);
            itemServer->SetSubTitle(
                tr("Out of the more recent server used, "
                   "out of the most used server"));
          } else {
            itemServer->SetStar(SubServer::icon_server_list_star6);
            itemServer->SetSubTitle(
                tr("Out of the more recent server used, "
                   "into the most used server"));
          }
        }
      }
      if (server.maxPlayer <= 65533) {
        // do server.currentPlayer/server.maxPlayer icon
        if (server.maxPlayer <= 0 || server.currentPlayer > server.maxPlayer) {
          itemServer->SetIcon(SubServer::icon_server_list_bug);
        } else {
          // to be very sure
          if (server.maxPlayer > 0) {
            int percent = (server.currentPlayer * 100) / server.maxPlayer;
            if (server.currentPlayer == server.maxPlayer ||
                (server.maxPlayer > 50 && percent > 98))
              itemServer->SetStat(SubServer::icon_server_list_stat4);
            else if (server.currentPlayer > 30 && percent > 50)
              itemServer->SetStat(SubServer::icon_server_list_stat3);
            else if (server.currentPlayer > 5 && percent > 20)
              itemServer->SetStat(SubServer::icon_server_list_stat2);
            else
              itemServer->SetStat(SubServer::icon_server_list_stat1);
          }
        }
        itemServer->SetSubTitle(QStringLiteral("%1/%2")
                                    .arg(server.currentPlayer)
                                    .arg(server.maxPlayer));
      }
      const std::vector<CatchChallenger::ServerFromPoolForDisplay> &servers =
          connection_->client->getServerOrdenedList();
      if (server.serverOrdenedListIndex < servers.size()) {
        itemServer->SetData(99, server.serverOrdenedListIndex);
      } else {
        std::cerr
            << "SubServer::addToServerList(): "
               "server.serverOrdenedListIndex>=serverOrdenedList.size(), " +
                   std::to_string(server.serverOrdenedListIndex) +
                   ">=" + std::to_string(servers.size()) + ", error";
        return;
      }

      itemServer->SetOnClick(
          std::bind(&SubServer::itemSelectionChanged, this, _1));
      serverList->AddItem(itemServer);
      index++;
    }
  }
}

void SubServer::OnLogged(
    std::vector<std::vector<CatchChallenger::CharacterEntry>> characters) {
  RemoveChild(Loading::GetInstance());
  is_loaded_ = true;
  characters_ = characters;

  Initialize();
  OnEnter();
  OnScreenResize();
  logged(connection_->client->getServerOrdenedList());
}
