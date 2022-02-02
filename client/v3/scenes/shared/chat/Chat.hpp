// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_CHAT_CHAT_HPP_
#define CLIENT_V3_SCENES_SHARED_CHAT_CHAT_HPP_

#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QObject>
#include <QTimer>

#include "../../../../../general/base/GeneralStructures.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/Button.hpp"
#include "../../../ui/Label.hpp"

namespace Scenes {
class Chat : public Node {
 public:
  ~Chat();
  static Chat *Create(Node *parent);
  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;

 private:
  std::string last_message_;
  ConnectionManager *connexionManager;
  PlayerInfo *player_info_;
  uint16_t input_height_;
  QPixmap *log_;
  QString log_content_;
  uint8_t flood_;

  bool pressed_;
  bool focus_;
  QTimer stop_flood_;
  UI::Button *button_;
  bool mobile_mode_;

  QGraphicsProxyWidget *proxy_;
  QLineEdit *line_edit_;

  Chat(Node *parent);
  void SendMessage(std::string message);
  void RefreshChat();
  void OnSystemMessageReceived(const CatchChallenger::Chat_type &type,
                               const std::string &message);
  void OnChatTextReceived(CatchChallenger::Chat_type chat_type,
                          std::string text, std::string pseudo,
                          CatchChallenger::Player_type player_type);
  void LastReplyTime(const uint32_t &time);
  void RemoveNumberForFlood();
  void OnReturnPressed();
  void DrawContent();
  void ReDrawContent();
};
}  // namespace Scenes

#endif  // CLIENT_V3_SCENES_SHARED_CHAT_CHAT_HPP_
