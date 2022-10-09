// Copyright 2021 CatchChallenger
#include "Login.hpp"

#include <QDesktopServices>
#include <iostream>
#include <string>

#include "../../entities/BlacklistPassword.hpp"
#include "../../ui/ThemedButton.hpp"
#include "../../Constants.hpp"

using Scenes::Login;
using std::placeholders::_1;

Login::Login(): UI::Dialog(false) {
  validated = false;
  SetDialogSize(Constants::DialogMediumSize());

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
  auto inner_rect = ContentBoundary();
  htmlText->RemoveAllChildrens();
  if (!site_page.isEmpty()) {
    auto btn = UI::GreenButton::Create();
    btn->SetText(tr("Web site"));
    btn->SetSize(UI::Button::kRectSmall);
    btn->SetWidth(inner_rect.width() * 0.4);
    btn->SetData(99, site_page.toStdString());
    btn->SetOnClick(std::bind(&Login::OnLinkClick, this, _1));
    htmlText->AddChild(btn);
  }
  if (!register_page.isEmpty()) {
    auto btn = UI::GreenButton::Create();
    btn->SetText(tr("Register"));
    btn->SetSize(UI::Button::kRectSmall);
    btn->SetWidth(inner_rect.width() * 0.4);
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

  auto x = inner_rect.left();
  auto y = inner_rect.top();
  auto space = Constants::ItemMediumSpacing();
  qreal column_width = 0;
  qreal column_x = 0;
  qreal row_height = 0;
  auto font_size = Constants::TextMediumSize();

  loginText->SetPixelSize(font_size);
  passwordText->SetPixelSize(font_size);
  rememberText->SetPixelSize(font_size);

  {
    login->SetSize(UI::Input::kMedium);

    column_x = inner_rect.right() * 0.35;
    column_width = inner_rect.right() - column_x; 
    row_height = login->Height();

    loginText->SetPos(x, y);
    login->SetPos(column_x, y);
    login->SetWidth(column_width);

    y += row_height + space;
  }
  {
    password->SetSize(UI::Input::kMedium);

    passwordText->SetPos(x, y);
    password->SetPos(column_x, y);
    password->SetWidth(column_width);

    y += row_height + space;
  }
  {
    rememberText->SetPos(x, y);
    remember->SetPos(rememberText->Right(), y);

    y += row_height + space;
  }
  {
    htmlText->SetPos(x, y);
    htmlText->SetSize(inner_rect.width(), inner_rect.height() - htmlText->Y());
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
