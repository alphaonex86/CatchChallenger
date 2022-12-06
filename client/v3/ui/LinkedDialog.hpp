// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_LINKEDDIALOG_HPP_
#define CLIENT_V3_UI_LINKEDDIALOG_HPP_

#include "Button.hpp"
#include "Dialog.hpp"
#include "Label.hpp"

namespace UI {
class LinkedDialog : public Dialog {
 public:
  static LinkedDialog *Create(bool show_navigation = true);
  ~LinkedDialog();

  void AddItem(Node *item, std::string id);
  void RemoveItem(Node *item);
  void SetCurrentItem(std::string id);
  void OnScreenResize() override;
  void ShowNavigation(bool show);
  void SetOnNext(std::function<void(std::string)> callback);
  void SetOnBack(std::function<void(std::string)> callback);
  void SetOnUseItem(std::function<void(uint16_t)> callback);
  void OnEnter() override;
  void OnExit() override;
  Node *CurrentItem();
  Node *Item(std::string id);

 private:
  Button *next_;
  Button *back_;
  std::vector<Node *> items_;
  std::vector<std::string> ids_;
  Node *current_item_;
  int current_index_;
  bool show_navigation_;

  std::function<void(std::string)> on_next_;
  std::function<void(std::string)> on_back_;
  std::function<void(uint16_t)> on_use_item_;

  uint8_t GetIndexById(std::string id);

 protected:
  LinkedDialog(bool show_navigation);
  void OnActionClick(Node *node);
};
}  // namespace UI
#endif  // CLIENT_V3_UI_LINKEDDIALOG_HPP_
