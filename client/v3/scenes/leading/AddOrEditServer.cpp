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
#include "../../Ultimate.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/EventManager.hpp"

using Scenes::AddOrEditServer;
using std::placeholders::_1;

AddOrEditServer::AddOrEditServer() : UI::Dialog(false) {
  ok = false;
  edit = false;

  quit = UI::Button::Create(":/CC/images/interface/cancel.png", this);
  validate = UI::Button::Create(":/CC/images/interface/validate.png", this);

  protocol_ = UI::Combo::Create(this);
  protocol_->AddItem(QString("Tcp"));
  protocol_->AddItem(QString("WS"));
  serverText = UI::Label::Create(this);
  serverText->SetVisible(false);
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

  auto font = serverText->GetFont();
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    font.setPixelSize(30 / 2);
  } else {
    font.setPixelSize(30);
  }
  serverText->SetFont(font);
  nameText->SetFont(font);
  proxyText->SetFont(font);

  unsigned int nameBackgroundNewHeight = 50;
  unsigned int space = 30;
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    font.setPixelSize(30 * 0.75 / 2);
    serverInput->SetFont(font);
    portInput->SetFont(font);
    nameInput->SetFont(font);
    proxyInput->SetFont(font);
    proxyPortInput->SetFont(font);
    space = 10;
    nameBackgroundNewHeight = 50 / 2;
  } else {
    font.setPixelSize(30 * 0.75);
    serverInput->SetFont(font);
    portInput->SetFont(font);
    nameInput->SetFont(font);
    proxyInput->SetFont(font);
    proxyPortInput->SetFont(font);
  }
  int top = 36;
  int bottom = 94 / 2;
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    top /= 2;
    bottom /= 2;
  }

  const QRectF &serverTextRect = serverText->BoundingRect();
  const QRectF &nameTextRect = nameText->BoundingRect();
  const QRectF &proxyTextRect = proxyText->BoundingRect();

  int border_size = 46;
  int idealW = background_->Width();

  const unsigned int &serverBackgroundNewWidth =
      idealW - nameTextRect.width() - border_size * 4;
  const unsigned int &nameBackgroundNewWidth =
      idealW - nameTextRect.width() - border_size * 4;
  const unsigned int &proxyBackgroundNewWidth =
      idealW - nameTextRect.width() - border_size * 4;
  if ((unsigned int)nameInput->Width() != nameBackgroundNewWidth ||
      (unsigned int)nameInput->Height() != nameBackgroundNewHeight) {
    serverInput->SetHeight(nameBackgroundNewHeight);
    portInput->SetHeight(nameBackgroundNewHeight);
    if (portInput->IsVisible()) {
      serverInput->SetWidth(serverBackgroundNewWidth * 3 / 4);
      portInput->SetWidth(serverBackgroundNewWidth * 1 / 4);
    } else {
      serverInput->SetWidth(serverBackgroundNewWidth);
    }
    nameInput->SetSize(nameBackgroundNewWidth, nameBackgroundNewHeight);
    proxyInput->SetSize(proxyBackgroundNewWidth * 3 / 4,
                        nameBackgroundNewHeight);
    proxyPortInput->SetSize(proxyBackgroundNewWidth * 1 / 4,
                            nameBackgroundNewHeight);
  }
  {
    protocol_->SetPos(x_ + border_size * 2, y_ + top * 1.5);
    // TODO(lanstat): Verify which is the best height for the combo
    protocol_->SetSize(100, 32);
    serverText->SetPos(x_ + border_size * 2, y_ + top * 1.5);
    const unsigned int serverBackgroundW =
        serverText->X() + serverTextRect.width();
    serverInput->SetPos(
        serverBackgroundW,
        serverText->Y() +
            (serverTextRect.height() - serverInput->BoundingRect().height()) /
                2);
    if (portInput->IsVisible())
      portInput->SetPos(
          serverBackgroundW + serverInput->Width(),
          serverText->Y() +
              (serverTextRect.height() - serverInput->BoundingRect().height()) /
                  2);
  }
  {
    nameText->SetPos(x_ + border_size * 2,
                     serverText->Y() + serverTextRect.height() + space);
    const unsigned int nameBackgroundW = nameText->X() + nameTextRect.width();
    nameInput->SetPos(
        nameBackgroundW,
        nameText->Y() +
            (nameTextRect.height() - nameInput->BoundingRect().height()) / 2);
  }
  {
    proxyText->SetPos(x_ + border_size * 2,
                      nameText->Y() + nameTextRect.height() + space);
    const unsigned int proxyBackgroundW =
        proxyText->X() + proxyTextRect.width();
    proxyInput->SetPos(
        proxyBackgroundW,
        proxyText->Y() +
            (proxyTextRect.height() - proxyInput->BoundingRect().height()) / 2);
    proxyPortInput->SetPos(
        proxyBackgroundW + proxyInput->Width(),
        proxyText->Y() +
            (proxyTextRect.height() - proxyInput->BoundingRect().height()) / 2);
  }
}

void AddOrEditServer::newLanguage() {
  proxyText->SetText(tr("Proxy: "));
  nameText->SetText(tr("Name: "));
  serverText->SetText(tr("Server: "));
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
