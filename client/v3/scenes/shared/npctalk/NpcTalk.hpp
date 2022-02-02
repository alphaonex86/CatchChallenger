// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_NPCTALK_NPCTALK_HPP_
#define CLIENT_V3_SCENES_SHARED_NPCTALK_NPCTALK_HPP_

#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/ThemedButton.hpp"
#include "../../../ui/Dialog.hpp"
#include "../../../ui/Label.hpp"
#include "../../../ui/ListView.hpp"

namespace Scenes {
class NpcTalk : public UI::Dialog {
 public:
  static NpcTalk *Create();
  ~NpcTalk();

  void Draw(QPainter *painter) override;
  void OnEnter() override;
  void OnExit() override;
  void OnScreenResize() override;

  void SetData(const QString &message, const QString &title);
  void SetData(const QString &message, const QString &title, const QString &button);
  void SetTitle(const QString &text);
  void SetMessage(const QString &text);
  void SetButtonLabel(const QString &text);
  void SetPortrait(const QPixmap image);
  void Show();
  void Close();
  void SetOnItemClick(std::function<void(std::string)> callback);

 private:
  NpcTalk();
  void OnItemContentClick(Node *node);

  Sprite *portrait_;
  UI::Button *quit_;
  QString title_;
  UI::ListView *content_;
  std::function<void(std::string)> on_item_click_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_NPCTALK_NPCTALK_HPP_
