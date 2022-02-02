// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_ADDOREDITSERVER_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_ADDOREDITSERVER_HPP_

#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Combo.hpp"
#include "../../ui/Dialog.hpp"

namespace Scenes {
class AddOrEditServer : public UI::Dialog {
 public:
  ~AddOrEditServer();
  static AddOrEditServer* Create();

  int type() const;
  void on_ok_clicked();
  QString server() const;
  uint16_t port() const;
  QString proxyServer() const;
  uint16_t proxyPort() const;
  QString name() const;
  void setType(const int &type);
  void setEdit(const bool &edit);
  void setServer(const QString &server);
  void setPort(const uint16_t &port);
  void setName(const QString &name);
  void setProxyServer(const QString &proxyServer);
  void setProxyPort(const uint16_t &proxyPort);
  bool isOk() const;
  void on_type_currentIndexChanged(uint8_t index);
  void on_cancel_clicked();
  void OnEnter() override;
  void OnExit() override;
  void OnActionClick(Node *node);

 protected:
  void OnScreenSD();
  void OnScreenHD();
  void OnScreenHDR();
  void OnScreenResize();

 private:
  AddOrEditServer();

  bool edit;
  UI::Button *quit;
  UI::Button *validate;
  UI::Combo *protocol_;

  UI::Label *serverText;
  UI::Input *serverInput;
  UI::Input *portInput;

  UI::Label *nameText;
  UI::Input *nameInput;

  UI::Label *proxyText;
  UI::Input *proxyInput;
  UI::Input *proxyPortInput;

  UI::Label *warning;

  bool ok;

  QString serverPrevious;
  QString portPrevious;
  QString namePrevious;
  QString proxyPrevious;
  QString proxyPortPrevious;

  void newLanguage();
};
}  // namespace Scenes
#endif  // CLIENT_QTOPENGL_SCENES_LEADING_ADDOREDITSERVER_HPP_
