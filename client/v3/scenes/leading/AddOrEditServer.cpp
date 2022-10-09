// Copyright 2021 CatchChallenger
#include "AddOrEditServer.hpp"

#include <QDesktopServices>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPainter>
#include <QRegularExpression>
#include <QTextDocument>
#include <iostream>

#include "../../../libqtcatchchallenger/Settings.hpp"
#include "../../Constants.hpp"
#include "../../Ultimate.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/EventManager.hpp"

using Scenes::AddOrEditServer;
using std::placeholders::_1;

AddOrEditServer::AddOrEditServer() : UI::Dialog(false) {
  SetDialogSize(Constants::DialogSmallSize());

  ok = false;
  edit = false;

  quit = UI::Button::Create(":/CC/images/interface/cancel.png", this);
  validate = UI::Button::Create(":/CC/images/interface/validate.png", this);

  protocol_ = UI::Combo::Create(this);
  protocol_->AddItem(QString("Tcp"));
  protocol_->AddItem(QString("WS"));
  serverInput = UI::Input::Create(this);
  serverInput->SetPlaceholder("ws://www.server.com:9999/");
  portInput = UI::Input::Create(this);
  /*
  portInput->setMinimum(1);
  portInput->setMaximum(65535);
  */
  portInput->SetValue(QString::number(42489));

  nameText = UI::Label::Create(this);
  nameInput = UI::Input::Create(this);

  proxyText = UI::Label::Create(this);
  proxyInput = UI::Input::Create(this);
  proxyPortInput = UI::Input::Create(this);
  /*
  proxyPortInput->setMinimum(1);
  proxyPortInput->setMaximum(65535);
  */

  warning = UI::Label::Create(this);
  warning->SetVisible(false);

  protocol_->SetOnSelectChange(
      std::bind(&AddOrEditServer::on_type_currentIndexChanged, this, _1));
  on_type_currentIndexChanged(0);

  quit->SetOnClick(std::bind(&AddOrEditServer::on_cancel_clicked, this));
  validate->SetOnClick(std::bind(&AddOrEditServer::on_ok_clicked, this));

  newLanguage();

#if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
  protocol_->SetVisible(false);
#if defined(NOTCPSOCKET)
  proxyPortInput->SetVisible(false);
  serverInput->SetPlaceholder("ws://www.server.com:9999/");
#endif
#endif

  AddActionButton(quit);
  AddActionButton(validate);
}

AddOrEditServer::~AddOrEditServer() {
  delete quit;
  delete validate;
  delete warning;
  delete protocol_;
  delete nameText;
}

AddOrEditServer *AddOrEditServer::Create() {
  return new (std::nothrow) AddOrEditServer();
}

void AddOrEditServer::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto inner_rect = ContentBoundary();

  auto x = inner_rect.left();
  auto y = inner_rect.top();
  auto space = Constants::ItemMediumSpacing();
  qreal column_width = 0;
  qreal column_x = 0;
  qreal row_height = 0;
  auto font_size = Constants::TextMediumSize();

  {
    protocol_->SetSize(UI::Combo::kMedium);
    serverInput->SetSize(UI::Input::kMedium);
    portInput->SetSize(UI::Input::kMedium);


    protocol_->SetPos(x, y);
    column_width = inner_rect.right() - protocol_->Right();
    column_x = protocol_->Right();
    row_height = protocol_->Height();

    serverInput->SetPos(protocol_->Right(), y);
    if (portInput->IsVisible()) {
      serverInput->SetWidth(column_width * 0.75);
      portInput->SetWidth(inner_rect.right() - serverInput->Right());
      portInput->SetPos(serverInput->Right(), y);
    } else {
      serverInput->SetWidth(column_width);
    }

    y += row_height + space;
  }
  {
    nameText->SetPixelSize(font_size);
    nameText->SetPos(x, y);

    nameInput->SetSize(UI::Input::kMedium);

    nameInput->SetPos(column_x, y);
    nameInput->SetWidth(column_width);

    y += row_height + space;
  }
  {
    proxyText->SetPixelSize(font_size);
    proxyText->SetPos(x, y);

    proxyInput->SetSize(UI::Input::kMedium);
    proxyPortInput->SetSize(UI::Input::kMedium);

    proxyInput->SetPos(column_x, y);
    proxyInput->SetWidth(column_width * 0.75);
    proxyPortInput->SetWidth(inner_rect.right() - proxyInput->Right());

    proxyPortInput->SetPos(proxyInput->Right(), y);
  }
}

void AddOrEditServer::newLanguage() {
  proxyText->SetText(tr("Proxy: "));
  nameText->SetText(tr("Name: "));
  if (edit)
    SetTitle(tr("EDIT"));
  else
    SetTitle(tr("ADD"));
}

int AddOrEditServer::type() const {
#if !defined(NOTCPSOCKET) && !defined(NOWEBSOCKET)
  return protocol_->CurrentIndex();
#else
#if defined(NOTCPSOCKET)
  return 1;
#else
#if defined(NOWEBSOCKET)
  return 0;
#else
#error add server but no tcp or web socket defined
  return -1;
#endif
#endif
#endif
}

void AddOrEditServer::setType(const int &type) {
#if !defined(NOTCPSOCKET) && !defined(NOWEBSOCKET)
  protocol_->SetCurrentIndex(type);
#else
#if defined(NOTCPSOCKET)
  protocol_->SetCurrentIndex(1);
#else
#if defined(NOWEBSOCKET)
  protocol_->SetCurrentIndex(0);
#endif
#endif
#endif
}

void AddOrEditServer::setEdit(const bool &edit) {
  this->edit = edit;
  SetTitle(tr("EDIT"));
}

void AddOrEditServer::on_ok_clicked() {
  if (nameText->Text() == QStringLiteral("internal")) {
    warning->SetText(tr("The name can't be \"internal\""));
    warning->SetVisible(true);
    return;
  }
  if (type() == 0) {
    if (!server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$"))) {
      warning->SetText(tr("The host seam don't be a valid hostname or ip"));
      warning->SetVisible(true);
      return;
    }
  } else if (type() == 1) {
    if (!server().startsWith("ws://") && !server().startsWith("wss://")) {
      warning->SetText(
          tr("The web socket url seam wrong, not start with ws:// or wss://"));
      warning->SetVisible(true);
      return;
    }
  }
  ok = true;
  Close();
}

QString AddOrEditServer::server() const { return serverInput->Value(); }

uint16_t AddOrEditServer::port() const { return portInput->Value().toUInt(); }

QString AddOrEditServer::proxyServer() const { return proxyInput->Value(); }

uint16_t AddOrEditServer::proxyPort() const {
  return proxyPortInput->Value().toUInt();
}

QString AddOrEditServer::name() const { return nameInput->Value(); }

void AddOrEditServer::setServer(const QString &server) {
  serverInput->SetValue(server);
}

void AddOrEditServer::setPort(const uint16_t &port) {
  portInput->SetValue(QString::number(port));
}

void AddOrEditServer::setName(const QString &name) {
  nameInput->SetValue(name);
}

void AddOrEditServer::setProxyServer(const QString &proxyServer) {
  proxyInput->SetValue(proxyServer);
}

void AddOrEditServer::setProxyPort(const uint16_t &proxyPort) {
  proxyPortInput->SetValue(QString::number(proxyPort));
}

bool AddOrEditServer::isOk() const { return ok; }

void AddOrEditServer::on_type_currentIndexChanged(uint8_t index) {
  switch (index) {
    case 0:
      portInput->SetVisible(true);
      serverInput->SetPlaceholder("www.server.com");
      break;
    case 1:
      portInput->SetVisible(false);
      serverInput->SetPlaceholder("ws://www.server.com:9999/");
      break;
    default:
      return;
  }
}

void AddOrEditServer::on_cancel_clicked() {
  ok = false;
  Close();
}

void AddOrEditServer::OnScreenSD() {}

void AddOrEditServer::OnScreenHD() {}

void AddOrEditServer::OnScreenHDR() {}

void AddOrEditServer::OnEnter() {
  UI::Dialog::OnEnter();

  quit->RegisterEvents();
  validate->RegisterEvents();
  nameInput->RegisterEvents();
  serverInput->RegisterEvents();
  portInput->RegisterEvents();
  proxyInput->RegisterEvents();
  proxyPortInput->RegisterEvents();
  protocol_->RegisterEvents();
}

void AddOrEditServer::OnExit() {
  quit->UnRegisterEvents();
  validate->UnRegisterEvents();
  nameInput->UnRegisterEvents();
  serverInput->UnRegisterEvents();
  portInput->UnRegisterEvents();
  proxyInput->UnRegisterEvents();
  proxyPortInput->UnRegisterEvents();
  protocol_->UnRegisterEvents();

  UI::Dialog::OnExit();
}

void AddOrEditServer::OnActionClick(Node *node) {
  if (node == quit) {
    on_cancel_clicked();
  } else if (node == validate) {
    on_ok_clicked();
  }
}
