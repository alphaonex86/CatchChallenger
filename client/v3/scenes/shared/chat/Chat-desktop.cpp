// Copyright 2021 CatchChallenger
#include "Chat.hpp"

#include <QPainter>
#include <QTextDocument>
#include <iostream>

#include "../../../../libcatchchallenger/ChatParsing.hpp"
#include "../../../Constants.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../core/SceneManager.hpp"
#include "../../../core/FontManager.hpp"
#include "../../../entities/Utils.hpp"

using Scenes::Chat;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

Chat::Chat(Node *parent) : Node(parent) {
  log_ = nullptr;

  input_height_ = Constants::ButtonSmallHeight() * 0.75;

  proxy_ = new QGraphicsProxyWidget(this, Qt::Widget);
  proxy_->setAutoFillBackground(false);
  proxy_->setAttribute(Qt::WA_NoSystemBackground);
  line_edit_ = new QLineEdit();
  line_edit_->setAutoFillBackground(false);
  line_edit_->setAttribute(Qt::WA_NoSystemBackground);
  QPalette palette;
  palette.setColor(QPalette::Base, Qt::transparent);
  palette.setColor(QPalette::Text, QColor(255, 255, 255));
  palette.setColor(QPalette::Window, Qt::transparent);
  palette.setCurrentColorGroup(QPalette::Active);
  palette.setColor(QPalette::Window, Qt::transparent);
  line_edit_->setPalette(palette);
  line_edit_->setFont(*FontManager::GetInstance()->GetFont("Calibri", Constants::TextSmallSize()));
  proxy_->setWidget(line_edit_);

  flood_ = 0;
  focus_ = false;
  pressed_ = false;
  connexionManager = ConnectionManager::GetInstance();
  player_info_ = PlayerInfo::GetInstance();
  stop_flood_.setSingleShot(false);
  stop_flood_.start(1500);


  QObject::connect(connexionManager->client,
                   &CatchChallenger::Api_client_real::QtlastReplyTime,
                   std::bind(&Chat::LastReplyTime, this, _1));
  QObject::connect(connexionManager->client,
                   &CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,
                   std::bind(&Chat::OnChatTextReceived, this, _1, _2, _3, _4));
  QObject::connect(connexionManager->client,
                   &CatchChallenger::Api_protocol_Qt::Qtnew_system_text,
                   std::bind(&Chat::OnSystemMessageReceived, this, _1, _2));
  QObject::connect(&stop_flood_, &QTimer::timeout,
                   std::bind(&Chat::RemoveNumberForFlood, this));

  SetSize(SceneManager::GetInstance()->width() * 0.25,
          SceneManager::GetInstance()->height() * 0.25);
}

Chat::~Chat() {
  if (log_ != nullptr) {
    delete log_;
    log_ = nullptr;
  }
  delete proxy_;
  proxy_ = nullptr;

  delete line_edit_;
  line_edit_ = nullptr;
}

Chat *Chat::Create(Node *parent) { return new (std::nothrow) Chat(parent); }

void Chat::Draw(QPainter *painter) {
  if (BoundingRect().isEmpty()) return;
  if (!focus_) {
    painter->setOpacity(0.75);
  }
  qreal inner_height = Height() - input_height_;
  painter->fillRect(0, 0, Width(), Height(), QColor(0, 0, 0, 150));
  painter->fillRect(0, inner_height, Width(), input_height_,
                    QColor(1, 1, 1, 50));
  if (log_ == nullptr) {
    DrawContent();
  }
  painter->drawPixmap(5, -5, Width(), inner_height, *log_, 0,
                      log_->height() - inner_height, log_->width(),
                      inner_height);
}

void Chat::DrawContent() {
  QTextDocument doc;
  doc.setHtml(log_content_);
  QImage buffer = QImage(Width() - 10, 1000, QImage::Format_ARGB32);
  buffer.fill(Qt::transparent);
  auto painter = new QPainter(&buffer);
  painter->setRenderHint(QPainter::Antialiasing);
  doc.setTextWidth(buffer.width());
  doc.drawContents(painter);
  painter->end();
  delete painter;
  log_ = new QPixmap(
      QPixmap::fromImage(Utils::CropToContent(buffer, QColor(0, 255, 224))));
}

void Chat::OnResize() {
  if (log_) {
    delete log_;
  }
  log_ = nullptr;
  proxy_->setPos(0, Height() - input_height_);
  proxy_->setMaximumSize(Width(), input_height_);
  proxy_->setMinimumSize(Width(), input_height_);
  ReDraw();
}

void Chat::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
  EventManager::GetInstance()->AddKeyboardListener(this);
}

void Chat::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void Chat::SendMessage(std::string message) {
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
    // chat_type = CatchChallenger::Chat_type_all;
    // break;
    // case 1:
    // chat_type = CatchChallenger::Chat_type_local;
    // break;
    // case 2:
    // chat_type = CatchChallenger::Chat_type_clan;
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

void Chat::RefreshChat() {
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
  ReDraw();
}

void Chat::OnSystemMessageReceived(const CatchChallenger::Chat_type &type,
                                   const std::string &message) {
  RefreshChat();
}

void Chat::OnChatTextReceived(CatchChallenger::Chat_type chat_type,
                              std::string text, std::string pseudo,
                              CatchChallenger::Player_type player_type) {
  RefreshChat();
}

void Chat::ReDrawContent() {
  if (log_) {
    delete log_;
  }
  log_ = nullptr;
}

void Chat::LastReplyTime(const uint32_t &time) { RefreshChat(); }

void Chat::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (focus_) {
    ReDraw();
  }
  focus_ = false;
  proxy_->clearFocus();
  if (press_validated) return;
  if (pressed_) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
    pressed_ = true;
  }
}

void Chat::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) {
    pressed_ = false;
    focus_ = false;
    proxy_->clearFocus();
    ReDraw();
    return;
  }
  if (!pressed_) return;
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  pressed_ = false;
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      prev_validated = true;
      focus_ = true;
      proxy_->setFocus();
      ReDraw();
    }
  }
}

void Chat::MouseMoveEvent(const QPointF &point) { (void)point; }

void Chat::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  if (!focus_) return;

  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    SendMessage(line_edit_->text().toStdString());
    line_edit_->clear();
  }
  ReDraw();
}

void Chat::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Chat::RemoveNumberForFlood() {
  if (flood_ <= 0) return;
  flood_--;
}

void Chat::OnReturnPressed() {}
