// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_CHAT_CHATDIALOG_HPP_
#define CLIENT_V3_SCENES_SHARED_CHAT_CHATDIALOG_HPP_

#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QObject>
#include <QTimer>

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Input.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/Backdrop.hpp"

namespace Scenes {
class ChatDialog : public UI::Dialog {
 public:
  ~ChatDialog();
  static ChatDialog *Create();

  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;

 private:
  std::string last_message_;
  ConnectionManager *connexionManager;
  PlayerInfo *player_info_;
  uint16_t inner_height_;
  QPixmap *log_;
  QString log_content_;
  uint8_t flood_;

  bool pressed_;
  bool focus_;
  QTimer stop_flood_;
  UI::Input *input_;
  UI::Backdrop *log_container_;

  ChatDialog();
  void SendMessage(std::string message);
  void RefreshChat();
  void OnSystemMessageReceived(const CatchChallenger::Chat_type &type,
                               const std::string &message);
  void OnChatTextReceived(CatchChallenger::Chat_type chat_type,
                          std::string text, std::string pseudo,
                          CatchChallenger::Player_type player_type);
  void LastReplyTime(const uint32_t &time);
  void RemoveNumberForFlood();
  void DrawContent();
  void ReDrawContent();
};
}  // namespace Scenes

#endif  // CLIENT_V3_SCENES_SHARED_CHAT_CHAT_HPP_
