// Copyright 2021 CatchChallenger
#include <QPainter>
#include <QTextDocument>
#include <iostream>

#include "../../../../libcatchchallenger/ChatParsing.hpp"
#include "../../../Constants.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../core/FontManager.hpp"
#include "../../../core/SceneManager.hpp"
#include "../../../entities/Utils.hpp"
#include "Chat.hpp"

using Scenes::Chat;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

Chat::Chat(Node *parent) : Node(parent) {
  button_ = UI::Button::Create(":/CC/images/interface/chat.png", this);
  button_->SetSize(UI::Button::kRoundMedium);
  button_->SetOnClick([&](Node *node) { Parent()->AddChild(dialog_); });

  dialog_ = Scenes::ChatDialog::Create(); 

  SetSize(button_->Width(), button_->Height());
}

Chat::~Chat() {
  delete dialog_;
  dialog_ = nullptr;
}

Chat *Chat::Create(Node *parent) { return new (std::nothrow) Chat(parent); }

void Chat::Draw(QPainter *painter) {}

void Chat::DrawContent() {}

void Chat::OnResize() {}

void Chat::RegisterEvents() { button_->RegisterEvents(); }

void Chat::UnRegisterEvents() { button_->UnRegisterEvents(); }

void Chat::SendMessage(std::string message) {}

void Chat::RefreshChat() {}

void Chat::OnSystemMessageReceived(const CatchChallenger::Chat_type &type,
                                   const std::string &message) {}

void Chat::OnChatTextReceived(CatchChallenger::Chat_type chat_type,
                              std::string text, std::string pseudo,
                              CatchChallenger::Player_type player_type) {}

void Chat::ReDrawContent() {}

void Chat::LastReplyTime(const uint32_t &time) {}

void Chat::MousePressEvent(const QPointF &point, bool &press_validated) {
  (void)point;
  (void)press_validated;
}

void Chat::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  (void)point;
  (void)prev_validated;
}

void Chat::MouseMoveEvent(const QPointF &point) { (void)point; }

void Chat::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Chat::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Chat::RemoveNumberForFlood() {}

void Chat::OnReturnPressed() {}
