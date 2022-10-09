// Copyright 2021 CatchChallenger
#include "NpcTalk.hpp"

#include <iostream>

#include "../../../core/AssetsLoader.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../Constants.hpp"

using Scenes::NpcTalk;
using std::placeholders::_1;

NpcTalk::NpcTalk() {
  SetDialogSize(Constants::DialogMediumSize());

  on_item_click_ = nullptr;
  portrait_ = Sprite::Create(this);
  content_ = UI::ListView::Create(this);
}

NpcTalk::~NpcTalk() {
  delete portrait_;
  portrait_ = nullptr;
  delete content_;
  content_ = nullptr;
}

NpcTalk *NpcTalk::Create() { return new (std::nothrow) NpcTalk(); }

void NpcTalk::OnEnter() {
  UI::Dialog::OnEnter();
  content_->RegisterEvents();
}

void NpcTalk::OnExit() {
  content_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void NpcTalk::SetData(const QString &message, const QString &title) {
  SetData(message, title, "OK");
}

void NpcTalk::SetData(const QString &message, const QString &title,
                      const QString &button) {
  title_ = title;
  content_->Clear();

  auto parsed = Utils::HTML2Node(
      message, std::bind(&NpcTalk::OnItemContentClick, this, _1));

  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< message.toStdString() << std::endl;
  content_->AddItems(parsed.nodes);
  ReDraw();
}

void NpcTalk::SetTitle(const QString &text) {}

void NpcTalk::SetMessage(const QString &text) {}

void NpcTalk::SetButtonLabel(const QString &text) {}

void NpcTalk::SetPortrait(const QPixmap image) {}

void NpcTalk::Show() { SetVisible(true); }

void NpcTalk::Close() { Parent()->RemoveChild(this); }

void NpcTalk::OnItemContentClick(Node *node) {
  std::string data = node->DataStr(99);
  if (on_item_click_) {
    on_item_click_(data);
  }
}

void NpcTalk::SetOnItemClick(std::function<void (std::string)> callback) {
  on_item_click_ = callback;
}
