// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_MESSAGEBAR_HPP_
#define CLIENT_V3_UI_MESSAGEBAR_HPP_

#include "../core/Node.hpp"

namespace UI {
class MessageBar : public Node {
 public:
  static MessageBar *Create(Node *parent = nullptr);
  ~MessageBar();

  void SetOnHide(const std::function<void()> &on_hide);
  void Show();
  void Hide();
  void SetMessages(const std::vector<std::string> &messages);
  void SetMessage(const std::string &message);
  void ShowNext(bool next);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void Draw(QPainter *painter) override;

 protected:
  explicit MessageBar(Node *parent = nullptr);

 private:
  std::vector<std::string> messages_;
  bool disabled_;
  std::function<void()> on_hide_;
  QString current_message_;
  QFont *font_;
  QFont *font_icon_;
  bool show_next_;

  void UpdateCurrentMessage();
};
}  // namespace UI
#endif  // CLIENT_V3_UI_MESSAGEBAR_HPP_
