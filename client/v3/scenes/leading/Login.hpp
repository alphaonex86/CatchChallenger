// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_LOGIN_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_LOGIN_HPP_

#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QString>
#include <vector>

#include "../../ui/Button.hpp"
#include "../../ui/Dialog.hpp"
#include "../../ui/Input.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/Checkbox.hpp"
#include "../../ui/Column.hpp"

namespace Scenes {
class Login : public UI::Dialog {
 public:
  ~Login();

  static Login *Create();
  void setAuth(const QStringList &list);
  void setLinks(const QString &site_page, const QString &register_page); QStringList getAuth();
  QString getLogin();
  QString getPass();
  bool isOk();

  void newLanguage();
  void validate();
  void OnActionClick(Node *node);
  void OnEnter() override;
  void OnExit() override;

 protected:
  void OnScreenResize() override;

 private:
  UI::Label *warning;
  UI::Label *loginText;
  UI::Input *login;
  UI::Label *passwordText;
  UI::Input *password;
  UI::Checkbox *remember;
  UI::Label *rememberText;
  UI::Column *htmlText;

  UI::Button *back;
  UI::Button *server_select;

  QStringList m_auth;
  bool validated;

  Login();
  void OnLinkClick(Node* node);
};
}  // namespace Scenes

#endif  // CLIENT_QTOPENGL_SCENES_LEADING_LOGIN_HPP_
