// Copyright 2021 <CatchChallenger>
#ifndef CLIENT_V3_UI_INPUTDIALOG_HPP_
#define CLIENT_V3_UI_INPUTDIALOG_HPP_

#include "Button.hpp"
#include "Dialog.hpp"
#include "Input.hpp"
#include "Label.hpp"

namespace UI {
class InputDialog : public Dialog {
 public:
  InputDialog();
  ~InputDialog();
  static InputDialog *Create();

  void Draw(QPainter *painter) override;
  void OnScreenResize() override;
  void OnAcceptClick();
  void newLanguage();
  void ShowInputNumber(const QString &title, const QString &message,
                       std::function<void(QString)> callback, int min, int max,
                       const QString &default_value);
  void ShowInputNumber(const QString &message,
                       std::function<void(QString)> callback, int min, int max,
                       const QString &default_value);

 private:
  UI::Label *message_;
  UI::Input *input_;
  UI::Button *accept_;
  std::function<void(QString)> callback_;

  uint8_t type_;
  int min_;
  int max_;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_MESSAGEDIALOG_HPP_
