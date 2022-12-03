// Copyright 2022 CatchChallenger
#include "ChatDialog.hpp"

#include <QPainter>
#include <QTextDocument>
#include <iostream>

#include "../../../../libcatchchallenger/ChatParsing.hpp"
#include "../../../Constants.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../core/FontManager.hpp"
#include "../../../core/SceneManager.hpp"
#include "../../../entities/Utils.hpp"

using Scenes::ChatDialog;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

ChatDialog::ChatDialog() {
  SetDialogSize(Constants::DialogMediumSize());
  log_ = nullptr;
  log_container_ = UI::Backdrop::Create(
      [&](QPainter *painter) {
        painter->fillRect(5, -5, Width(), inner_height_, QColor(0, 0, 0));
        if (log_ != nullptr) {
          painter->drawPixmap(5, -5, Width(), inner_height_, *log_, 0,
                              log_->height() - inner_height_, log_->width(),
                              inner_height_);
        }
      },
      this);
  input_ = UI::Input::Create(this);
  input_->SetSize(UI::Input::kMedium);
  input_->SetOnTextChange([&](std::string text) {
    SendMessage(text);
    input_->Clear();
  });

  flood_ = 0;
  focus_ = false;
  pressed_ = false;
  connexionManager = ConnectionManager::GetInstance();
  player_info_ = PlayerInfo::GetInstance();
  stop_flood_.setSingleShot(false);
  stop_flood_.start(1500);

  QObject::connect(connexionManager->client,
                   &CatchChallenger::Api_client_real::QtlastReplyTime,
                   std::bind(&ChatDialog::LastReplyTime, this, _1));
  QObject::connect(
      connexionManager->client,
      &CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,
      std::bind(&ChatDialog::OnChatTextReceived, this, _1, _2, _3, _4));
  QObject::connect(
      connexionManager->client,
      &CatchChallenger::Api_protocol_Qt::Qtnew_system_text,
      std::bind(&ChatDialog::OnSystemMessageReceived, this, _1, _2));
  QObject::connect(&stop_flood_, &QTimer::timeout,
                   std::bind(&ChatDialog::RemoveNumberForFlood, this));
  SetTitle(tr("Chat"));
}

ChatDialog::~ChatDialog() {
  if (log_ != nullptr) {
    delete log_;
    log_ = nullptr;
  }

  delete input_;
  input_ = nullptr;
}

ChatDialog *ChatDialog::Create() { return new (std::nothrow) ChatDialog(); }

void ChatDialog::DrawContent() {
  auto inner = ContentBoundary();
  QTextDocument doc;
  doc.setHtml(log_content_);

  QImage buffer = QImage(inner.width() - 10, 1000, QImage::Format_ARGB32);
  buffer.fill(Qt::transparent);
  auto painter = new QPainter(&buffer);
  painter->setRenderHint(QPainter::Antialiasing);
  doc.setTextWidth(buffer.width());
  doc.drawContents(painter);
  painter->end();
  delete painter;
  log_ = new QPixmap(
      QPixmap::fromImage(Utils::CropToContent(buffer, QColor(0, 255, 224))));

  log_container_->ReDraw();
}

void ChatDialog::SendMessage(std::string message) {
  auto &playerInformations =
      connexionManager->client->get_player_informations();
  QString text = QString::fromStdString(message);
  text.remove("\n");
  text.remove("\r");
  text.remove("\t");
  if (text.isEmpty()) return;
  if (text.contains(QRegularExpression("^ +$"))) {
    // chatInput->Clear();
    connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,
                                              "Space text not allowed");
    return;
  }
  if (text.size() > 256) {
    // chatInput->Clear();
    connexionManager->client->add_system_text(CatchChallenger::Chat_type_system,
                                              "Message too long");
    return;
  }
  if (!text.startsWith('/')) {
    if (text.toStdString() == last_message_) {
      // chatInput->Clear();
      connexionManager->client->add_system_text(
          CatchChallenger::Chat_type_system, "Send message like as previous");
      return;
    }
    if (flood_ > 2) {
      // chatInput->Clear();
      connexionManager->client->add_system_text(
          CatchChallenger::Chat_type_system, "Stop flood");
      return;
    }
  }
  flood_++;
  last_message_ = text.toStdString();
  // chatInput->SetValue(QString());
  if (!text.startsWith("/pm ")) {
    CatchChallenger::Chat_type chat_type;
    chat_type = CatchChallenger::Chat_type_all;
    // switch (chatType->ItemData(chatType->CurrentIndex(), 99)) {
    // default:
    // case 0:
    // chat_type = CatchChallenger::ChatDialog_type_all;
    // break;
    // case 1:
    // chat_type = CatchChallenger::ChatDialog_type_local;
    // break;
    // case 2:
    // chat_type = CatchChallenger::ChatDialog_type_clan;
    // break;
    //}
    connexionManager->client->sendChatText(chat_type, text.toStdString());
    if (!text.startsWith('/'))
      connexionManager->client->add_chat_text(
          chat_type, text.toStdString(),
          connexionManager->client->player_informations.public_informations
              .pseudo,
          connexionManager->client->player_informations.public_informations
              .type);
  } else if (text == "/clan_leave") {
    // actionClan.push_back(ActionClan_Leave);
    connexionManager->client->leaveClan();
  } else if (text == "/clan_dissolve") {
    // actionClan.push_back(ActionClan_Dissolve);
    connexionManager->client->dissolveClan();
  } else if (!text.startsWith("/clan_invite ")) {
    text.remove(0, std::string("/clan_invite ").size());
    if (!text.isEmpty()) {
      // actionClan.push_back(ActionClan_Invite);
      connexionManager->client->inviteClan(text.toStdString());
    }
  } else if (!text.startsWith("/clan_eject ")) {
    text.remove(0, std::string("/clan_eject ").size());
    if (!text.isEmpty()) {
      // actionClan.push_back(ActionClan_Eject);
      connexionManager->client->ejectClan(text.toStdString());
    }
  } else if (text == "/clan_informations") {
    if (!player_info_->HaveClanInformation)
      connexionManager->client->add_system_text(
          CatchChallenger::Chat_type::Chat_type_system, "No clan information");
    else if (player_info_->ClanName.empty() || playerInformations.clan == 0)
      connexionManager->client->add_system_text(
          CatchChallenger::Chat_type::Chat_type_system, "No clan");
    else
      connexionManager->client->add_system_text(
          CatchChallenger::Chat_type::Chat_type_system,
          "Name: " + player_info_->ClanName);
  } else if (text.contains(QRegularExpression("^/pm [^ ]+ .+$"))) {
    QString pseudo = text;
    pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
    text.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
    connexionManager->client->sendPM(text.toStdString(), pseudo.toStdString());
    connexionManager->client->add_chat_text(
        CatchChallenger::Chat_type_pm, text.toStdString(),
        QObject::tr("To: ").toStdString() + pseudo.toStdString(),
        CatchChallenger::Player_type_normal);
  }
  RefreshChat();
}

void ChatDialog::RefreshChat() {
  const std::vector<CatchChallenger::Api_protocol::ChatEntry> &chat_list =
      connexionManager->client->getChatContent();
  if (chat_list.empty()) return;
  unsigned int index = 0;
  uint8_t line_height = 22;
  log_content_.clear();
  while (index < chat_list.size()) {
    const CatchChallenger::Api_protocol::ChatEntry &entry = chat_list.at(index);
    bool addPlayerInfo = true;
    if (entry.chat_type == CatchChallenger::Chat_type_system ||
        entry.chat_type == CatchChallenger::Chat_type_system_important)
      addPlayerInfo = false;
    if (!addPlayerInfo) {
      log_content_ +=
          QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
              std::string(), CatchChallenger::Player_type_normal,
              entry.chat_type, entry.text));
    } else {
      log_content_ +=
          QString::fromStdString(CatchChallenger::ChatParsing::new_chat_message(
              entry.player_pseudo, entry.player_type, entry.chat_type,
              entry.text));
    }
    index++;
  }
  log_content_.append(QString(
      "<div style=\"background-color: #00ffe0;width: 100%;height: 2px\"/>"));
  ReDrawContent();
  DrawContent();
}

void ChatDialog::OnSystemMessageReceived(const CatchChallenger::Chat_type &type,
                                         const std::string &message) {
  RefreshChat();
}

void ChatDialog::OnChatTextReceived(CatchChallenger::Chat_type chat_type,
                                    std::string text, std::string pseudo,
                                    CatchChallenger::Player_type player_type) {
  RefreshChat();
}

void ChatDialog::ReDrawContent() {
  if (log_) {
    delete log_;
  }
  log_ = nullptr;
}

void ChatDialog::LastReplyTime(const uint32_t &time) { RefreshChat(); }

void ChatDialog::OnEnter() {
  UI::Dialog::OnEnter();

  input_->RegisterEvents();
}

void ChatDialog::OnExit() {
  input_->UnRegisterEvents();

  UI::Dialog::OnExit();
}

void ChatDialog::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto inner_rect = ContentPlainBoundary();

  inner_height_ = inner_rect.height() - input_->Height() - 50;
  log_container_->SetPos(inner_rect.x(), inner_rect.y() + 40);
  log_container_->SetSize(inner_rect.width(), inner_height_);

  input_->SetPos(inner_rect.x(), inner_rect.bottom() - input_->Height());
  input_->SetWidth(inner_rect.width());
}

void ChatDialog::RemoveNumberForFlood() {
  if (flood_ <= 0) return;
  flood_--;
}
