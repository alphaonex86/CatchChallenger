// Copyright 2021 CatchChallenger
#include "Login.hpp"

#include <QDesktopServices>
#include <iostream>
#include <string>

#include "../../entities/BlacklistPassword.hpp"
#include "../../ui/ThemedButton.hpp"

using Scenes::Login;
using std::placeholders::_1;

Login::Login(): UI::Dialog(false) {
  validated = false;

  loginText = UI::Label::Create(this);
  login = UI::Input::Create(this);
  passwordText = UI::Label::Create(this);
  password = UI::Input::Create(this);
  password->SetMode(UI::Input::kPassword);
  remember = UI::Checkbox::Create(this);
  remember->SetChecked(true);
  rememberText = UI::Label::Create(this);
  htmlText = UI::Column::Create(this);

  server_select =
      UI::Button::Create(":/CC/images/interface/validate.png", this);
  back = UI::Button::Create(":/CC/images/interface/cancel.png", this);
  back->SetOnClick(std::bind(&Login::OnActionClick, this, _1));
  server_select->SetOnClick(std::bind(&Login::OnActionClick, this, _1));

  AddActionButton(back);
  AddActionButton(server_select);

  newLanguage();
}

Login::~Login() {
  delete loginText;
  delete login;
  delete passwordText;
  delete password;
  delete remember;
  delete rememberText;
  delete htmlText;

  delete server_select;
  delete back;
}

Login *Login::Create() { return new (std::nothrow) Login(); }

void Login::setAuth(const QStringList &list) {
  m_auth = list;
  if (list.size() < 3) return;
  QString n = list.first();
  int i = n.toUInt();
  if (i < (list.size() - 1) / 2) {
    login->SetValue(list.at(1 + i * 2));
    password->SetValue(list.at(1 + i * 2 + 1));
    remember->SetChecked(!password->Value().isEmpty());
  }
}

void Login::setLinks(const QString &site_page, const QString &register_page) {
  htmlText->RemoveAllChildrens();
  if (!site_page.isEmpty()) {
    auto btn = UI::GreenButton::Create();
    btn->SetText(tr("Web site"));
    btn->SetHeight(30);
    btn->SetPixelSize(12);
    btn->SetData(99, site_page.toStdString());
    btn->SetOnClick(std::bind(&Login::OnLinkClick, this, _1));
    htmlText->AddChild(btn);
  }
  if (!register_page.isEmpty()) {
    auto btn = UI::GreenButton::Create();
    btn->SetText(tr("Register"));
    btn->SetHeight(30);
    btn->SetPixelSize(12);
    btn->SetData(99, register_page.toStdString());
    btn->SetOnClick(std::bind(&Login::OnLinkClick, this, _1));
    htmlText->AddChild(btn);
  }
}

void Login::OnLinkClick(Node *node) {
  if (!QDesktopServices::openUrl(
          QUrl(QString::fromStdString(node->DataStr(99)))))
    std::cerr << "Link() failed" << std::endl;
}

QStringList Login::getAuth() {
  QString l = login->Value();
  QString p = password->Value();
  QStringList list = m_auth;
  int index = 1;
  while (index < list.size()) {
    if (list.at(index) == l) {
      if (remember->IsChecked())
        list[index + 1] = p;
      else
        list[index + 1] = QString();
      list.first() = QString::number((index - 1) / 2);
      break;
    }
    index += 2;
  }
  if (index >= list.size()) {
    QString Si = QString::number((list.size() - 1) / 2 - 1);
    if (!list.isEmpty())
      list[0] = Si;
    else
      list.append(Si);
    list.append(l);
    list.append(p);
  }
  if (m_auth != list)
    return list;
  else
    return QStringList();
}

QString Login::getLogin() { return login->Value(); }

QString Login::getPass() { return password->Value(); }

bool Login::isOk() { return validated; }

void Login::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto inner_rect = ContentBoundary();

  auto font = loginText->GetFont();
  if (BoundingRect().width() < 600 || BoundingRect().height() < 640) {
    back->SetSize(83 / 2, 94 / 2);
    server_select->SetSize(83 / 2, 94 / 2);
    font.setPixelSize(30 / 2);
  } else {
    back->SetSize(83, 94);
    server_select->SetSize(83, 94);
    font.setPixelSize(30);
  }
  loginText->SetFont(font);
  passwordText->SetFont(font);
  rememberText->SetFont(font);

  unsigned int productKeyBackgroundNewHeight = 50;
  if (background_->Width() < 600 || background_->Height() < 640) {
    font.setPixelSize(30 * 0.75 / 2);
    productKeyBackgroundNewHeight = 50 / 2;
  } else {
    font.setPixelSize(30 * 0.75);
  }

  const QRectF &loginTextRect = loginText->BoundingRect();
  const QRectF &productKeyTextRect = passwordText->BoundingRect();
  int maxWidth = loginTextRect.width();
  if (maxWidth < productKeyTextRect.width())
    maxWidth = productKeyTextRect.width();

  int border_size = 46;
  int space = 10;

  {
    loginText->SetPos(inner_rect.x() + border_size * 2, inner_rect.y());
    const unsigned int volumeSliderW = loginText->X() + loginTextRect.width();
    login->SetPos(volumeSliderW, loginText->Y());
    login->SetSize(inner_rect.width() - loginTextRect.width() - border_size * 4,
                   loginTextRect.height() + 5);
  }
  {
    passwordText->SetPos(inner_rect.x() + border_size * 2,
                         loginText->Y() + loginTextRect.height() + space);
    const unsigned int productKeyBackgroundW =
        passwordText->X() + productKeyTextRect.width();
    password->SetPos(
        productKeyBackgroundW,
        passwordText->Y() + (productKeyTextRect.height() -
                             passwordText->BoundingRect().height()) /
                                2);
    password->SetSize(
        inner_rect.width() - passwordText->Width() - border_size * 4,
        loginTextRect.height() + 5);
  }
  {
    rememberText->SetPos(
        inner_rect.x() + border_size * 2,
        passwordText->Y() + passwordText->BoundingRect().height() + space);
    remember->SetPos(inner_rect.x() + border_size * 2,
                     rememberText->Y() +
                         rememberText->BoundingRect().height() / 2 -
                         remember->BoundingRect().height() / 2);
  }
  {
    htmlText->SetPos(inner_rect.x() + border_size * 2,
                     rememberText->Y() + rememberText->BoundingRect().height());
  }
}

void Login::validate() {
  if (password->Value().size() < 6) {
    warning->SetText("<dev style=\"background-color:#fcc;\">" +
                     tr("Your password need to be at minimum of 6 characters") +
                     "</dev>");
    warning->SetVisible(true);
    return;
  }
  {
    std::string pass = password->Value().toStdString();
    std::transform(pass.begin(), pass.end(), pass.begin(), ::tolower);
    unsigned int index = 0;
    while (index < BlacklistPassword::list.size()) {
      if (BlacklistPassword::list.at(index) == pass) {
        warning->SetText("<dev style=\"background-color:#fcc;\">" +
                         tr("Your password is into the most common password in "
                            "the world, too easy to crack dude! Change it!") +
                         "</dev>");
        warning->SetVisible(true);
        return;
      }
      index++;
    }
  }
  if (login->Value().size() < 3) {
    warning->SetText("<dev style=\"background-color:#fcc;\">" +
                     tr("Your login need to be at minimum of 3 characters") +
                     "</dev>");
    warning->SetVisible(true);
    return;
  }
  if (password->Value() == login->Value()) {
    warning->SetText("<dev style=\"background-color:#fcc;\">" +
                     tr("Your login can't be same as your login") + "</dev>");
    warning->SetVisible(true);
    return;
  }

  validated = true;
  Close();
}

void Login::newLanguage() {
  loginText->SetText(tr("Email: "));
  passwordText->SetText(tr("Password: "));
  rememberText->SetText(tr("Remember the password"));
  SetTitle(tr("LOGIN"));
}

void Login::OnActionClick(Node *node) {
  if (node == back) {
    Close();
  } else if (node == server_select) {
    validate();
  }
}

void Login::OnEnter() {
  UI::Dialog::OnEnter();
  htmlText->RegisterEvents();
  login->RegisterEvents();
  password->RegisterEvents();
}

void Login::OnExit() {
  htmlText->UnRegisterEvents();
  login->UnRegisterEvents();
  password->UnRegisterEvents();
  UI::Dialog::OnExit();
}
