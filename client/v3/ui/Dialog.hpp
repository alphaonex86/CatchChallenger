// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_DIALOG_HPP_
#define CLIENT_V3_UI_DIALOG_HPP_

#include <vector>

#include "../core/Scene.hpp"
#include "../core/Sprite.hpp"
#include "Backdrop.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "Row.hpp"

namespace UI {
class Dialog : public Scene {
 public:
  ~Dialog();

  void SetOnClose(std::function<void()> callback);
  void AddActionButton(Button *action);
  void InsertActionButton(Button *action, int pos);
  void RemoveActionButton(Button *action);
  void RecalculateActionButtons();
  void SetTitle(const QString &text);
  QString Title() const;
  void Close();
  void SetDialogSize(int w, int h);
  void SetDialogSize(qreal w, qreal h);
  void ShowClose(bool show);

  void OnEnter() override;
  void OnExit() override;

 protected:
  Dialog(bool show_close = true);

  int x_;
  int y_;
  QSizeF dialog_size_;
  Sprite *background_;
  Backdrop *backdrop_;

  void OnScreenSD() override;
  void OnScreenHD() override;
  void OnScreenHDR() override;
  void OnScreenResize() override;
  void Draw(QPainter *painter) override;
  QRectF ContentBoundary() const;
  QRectF ContentPlainBoundary() const;

 private:
  Row *actions_;
  Label *title_;
  Sprite *title_background_;
  QRectF content_boundary_;
  QRectF content_plain_boundary_;
  Button *quit_;
  bool show_close_;

  std::function<void()> on_close_;

  void OnActionButton(Node *node);
};
}  // namespace UI
#endif  // CLIENT_V3_UI_DIALOG_HPP_
