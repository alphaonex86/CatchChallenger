// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_UI_MESSAGEDIALOG_HPP_
#define CLIENT_V3_UI_MESSAGEDIALOG_HPP_

#include "Button.hpp"
#include "Dialog.hpp"
#include "Label.hpp"

namespace UI {
class MessageDialog : public Dialog {
 public:
  MessageDialog();
  ~MessageDialog();
  static MessageDialog *Create();

  void Draw(QPainter *painter) override;
  void OnScreenResize() override;
  void SetMessage(const QString &text);
  void newLanguage();
  void Show(const QString &title, const QString &message);
  void Show(const QString &message);

 private:
  UI::Label *message_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_MESSAGEDIALOG_HPP_
