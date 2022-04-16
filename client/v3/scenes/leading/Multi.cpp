// Copyright 2021 CatchChallenger
#include "Multi.hpp"

#include <utime.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <iostream>

#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/base/FacilityLibGeneral.hpp"
#include "../../../../general/base/Version.hpp"
#include "../../../../general/base/cpp11addition.hpp"
#include "../../../../general/tinyXML2/tinyxml2.hpp"
#include "../../../libcatchchallenger/ClientVariable.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../../libqtcatchchallenger/PlatformMacro.hpp"
#include "../../../libqtcatchchallenger/Settings.hpp"
#include "../../Ultimate.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/Sprite.hpp"
#include "../../core/StackedScene.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ListView.hpp"
#include "MultiItem.hpp"
#if defined(_WIN32) || defined(Q_OS_MAC)
#include "../../../entities/InternetUpdater.hpp"
#endif

using Scenes::Multi;
using std::placeholders::_1;

Multi::Multi() : reply(nullptr) {
  srand(time(0));
  login = nullptr;

  temp_xmlConnexionInfoList = loadXmlConnexionInfoList();
  temp_customConnexionInfoList = loadConfigConnexionInfoList();
  mergedConnexionInfoList = temp_customConnexionInfoList;
  mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),
                                 temp_xmlConnexionInfoList.begin(),
                                 temp_xmlConnexionInfoList.end());
  std::sort(mergedConnexionInfoList.begin(), mergedConnexionInfoList.end());
  selectedServer.unique_code.clear();
  selectedServer.isCustom = false;

  server_add =
      UI::Button::Create(":/CC/images/interface/greenbutton.png", this);
  server_add->SetOutlineColor(QColor(44, 117, 0));
  server_remove = UI::Button::Create(":/CC/images/interface/delete.png", this);
  server_remove->SetOutlineColor(QColor(125, 0, 0));
  server_edit =
      UI::Button::Create(":/CC/images/interface/greenbutton.png", this);
  server_edit->SetOutlineColor(QColor(44, 117, 0));
  server_refresh =
      UI::Button::Create(":/CC/images/interface/refresh.png", this);

  server_select = UI::Button::Create(":/CC/images/interface/next.png", this);
  back = UI::Button::Create(":/CC/images/interface/back.png", this);

  wdialog = Sprite::Create(":/CC/images/interface/message.png", this);
  warning = UI::Label::Create(this);
  warning->SetAlignment(Qt::AlignCenter);
  serverEmpty = UI::Label::Create(this);
  scrollZone = UI::ListView::Create(wdialog);
  scrollZone->SetItemSpacing(5);

  server_add->SetOnClick(std::bind(&Multi::server_add_clicked, this));
  server_remove->SetOnClick(std::bind(&Multi::server_remove_clicked, this));
  server_edit->SetOnClick(std::bind(&Multi::server_edit_clicked, this));
  server_select->SetOnClick(std::bind(&Multi::server_select_clicked, this));
  back->SetOnClick(std::bind(&Multi::GoBack, this));
  server_refresh->SetOnClick(
      std::bind(&Multi::on_server_refresh_clicked, this));

  newLanguage();

  // need be the last
  //downloadFile();
  displayServerList();
  addServer = nullptr;
  subserver_ = nullptr;
}

Multi::~Multi() {
  if (subserver_) {
    delete subserver_;
    subserver_ = nullptr;
  }
  if (addServer) {
    delete addServer;
    addServer = nullptr;
  }
}

Multi *Multi::Create() { return new (std::nothrow) Multi(); }

void Multi::displayServerList() {
  serverEmpty->SetVisible(mergedConnexionInfoList.empty());
#if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
#error Web socket and tcp socket are both not supported
  return;
#endif
  if (selectedServer.unique_code.isEmpty()) {
    server_remove->SetEnabled(false);
    server_edit->SetEnabled(false);
  }

  // clean the previous content
  scrollZone->Clear();

  server_select->SetEnabled(!selectedServer.unique_code.isEmpty());
  // serverWidget->setVisible(index==0);
  unsigned int index = 0;
  while (index < mergedConnexionInfoList.size()) {
    const ConnectionInfo &connexionInfo = mergedConnexionInfoList.at(index);
    if (connexionInfo.host.isEmpty() && connexionInfo.ws.isEmpty()) {
      index++;
      continue;
    }
    auto *newEntry = MultiItem::Create(connexionInfo);
    newEntry->SetOnClick(std::bind(&Multi::OnSelectServer, this, _1));
    if (selectedServer.unique_code == connexionInfo.unique_code) {
      server_edit->SetEnabled(connexionInfo.isCustom);
      server_remove->SetEnabled(connexionInfo.isCustom);
      // newEntry->SetEnabled(true);
    } else {
      // newEntry->SetEnabled(false);
    }

    scrollZone->AddItem(newEntry);
    index++;
  }
}

void Multi::server_add_clicked() {
  if (addServer == nullptr) {
    addServer = AddOrEditServer::Create();
    addServer->SetOnClose(std::bind(&Multi::server_add_finished, this));
  }
  AddChild(addServer);
}

void Multi::server_add_finished() {
  if (addServer == nullptr) return;
  RemoveChild(addServer);
  if (!addServer->isOk()) return;
  if (!Settings::settings->isWritable()) {
    warning->SetText(tr("Option is not writable") + ": " +
                     QString::number(Settings::settings->status()));
    warning->SetVisible(true);
  }
#ifdef __EMSCRIPTEN__
  std::cerr << "AddOrEditServer returned" << std::endl;
#endif
  ConnectionInfo connexionInfo;
  connexionInfo.connexionCounter = 0;
  connexionInfo.lastConnexion =
      static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);

  connexionInfo.name = addServer->name();
  connexionInfo.unique_code = QString::fromStdString(
      CatchChallenger::FacilityLibGeneral::randomPassword(
          "abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",
          16));
  connexionInfo.isCustom = true;

  if (addServer->type() == 0) {
    connexionInfo.port = addServer->port();
#ifndef NOTCPSOCKET
    connexionInfo.host = addServer->server();
#endif
    connexionInfo.ws.clear();
  } else {
    connexionInfo.port = 0;
    connexionInfo.host.clear();
#ifndef NOWEBSOCKET
    connexionInfo.ws = addServer->server();
#endif
  }

  connexionInfo.proxyHost = addServer->proxyServer();
  connexionInfo.proxyPort = addServer->proxyPort();
  temp_customConnexionInfoList.push_back(connexionInfo);
  mergedConnexionInfoList = temp_customConnexionInfoList;
  mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),
                                 temp_xmlConnexionInfoList.begin(),
                                 temp_xmlConnexionInfoList.end());
  std::sort(mergedConnexionInfoList.begin(), mergedConnexionInfoList.end());
  saveConnexionInfoList();
  displayServerList();
}

void Multi::server_edit_clicked() {
  if (selectedServer.unique_code.isEmpty()) return;
  unsigned int index = 0;
  while (index < mergedConnexionInfoList.size()) {
    ConnectionInfo &connexionInfo = mergedConnexionInfoList[index];
    if (connexionInfo.isCustom == selectedServer.isCustom &&
        connexionInfo.unique_code == selectedServer.unique_code) {
      if (!connexionInfo.isCustom) return;
      if (addServer == nullptr) {
        addServer = AddOrEditServer::Create();
        addServer->SetOnClose(std::bind(&Multi::server_edit_finished, this));
      }
#if !defined(NOTCPSOCKET) && !defined(NOWEBSOCKET)
      if (connexionInfo.ws.isEmpty()) {
        addServer->setType(0);
        addServer->setServer(connexionInfo.host);
        addServer->setPort(connexionInfo.port);
      } else {
        addServer->setType(1);
        addServer->setServer(connexionInfo.ws);
      }
#else
#if defined(NOTCPSOCKET)
      addServer->setType(1);
      addServer->setServer(connexionInfo.ws);
#else
#if defined(NOWEBSOCKET)
      addServer->setType(0);
      addServer->setServer(connexionInfo.host);
      addServer->setPort(connexionInfo.port);
#endif
#endif
#endif
      addServer->setName(connexionInfo.name);
      addServer->setProxyServer(connexionInfo.proxyHost);
      addServer->setProxyPort(connexionInfo.proxyPort);
      addServer->setEdit(true);
      AddChild(addServer);
      return;
    }
    index++;
  }
  std::cerr << "remove server not found" << std::endl;
}

void Multi::server_edit_finished() {
  if (addServer == nullptr) return;
  RemoveChild(addServer);
  if (!addServer->isOk()) return;
#ifdef __EMSCRIPTEN__
  std::cerr << "AddOrEditServer returned" << std::endl;
#endif
  unsigned int index = 0;
  while (index < temp_customConnexionInfoList.size()) {
    ConnectionInfo &connexionInfo = temp_customConnexionInfoList[index];
    if (connexionInfo.isCustom == selectedServer.isCustom &&
        connexionInfo.unique_code == selectedServer.unique_code) {
      if (!connexionInfo.isCustom) return;

      connexionInfo.name = addServer->name();
      connexionInfo.unique_code = QString::fromStdString(
          CatchChallenger::FacilityLibGeneral::randomPassword(
              "abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",
              16));
      connexionInfo.isCustom = true;

      if (addServer->type() == 0) {
        connexionInfo.port = addServer->port();
#ifndef NOTCPSOCKET
        connexionInfo.host = addServer->server();
#endif
        connexionInfo.ws.clear();
      } else {
        connexionInfo.port = 0;
        connexionInfo.host.clear();
#ifndef NOWEBSOCKET
        connexionInfo.ws = addServer->server();
#endif
      }

      connexionInfo.proxyHost = addServer->proxyServer();
      connexionInfo.proxyPort = addServer->proxyPort();
    }
    index++;
  }
  mergedConnexionInfoList = temp_customConnexionInfoList;
  mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),
                                 temp_xmlConnexionInfoList.begin(),
                                 temp_xmlConnexionInfoList.end());
  std::sort(mergedConnexionInfoList.begin(), mergedConnexionInfoList.end());
  saveConnexionInfoList();
  displayServerList();
}

void Multi::server_remove_clicked() {
  if (selectedServer.unique_code.isEmpty()) return;
  unsigned int index = 0;
  while (index < temp_customConnexionInfoList.size()) {
    ConnectionInfo &connexionInfo = temp_customConnexionInfoList[index];
    if (connexionInfo.isCustom == selectedServer.isCustom &&
        connexionInfo.unique_code == selectedServer.unique_code) {
      if (!connexionInfo.isCustom) return;
      temp_customConnexionInfoList.erase(temp_customConnexionInfoList.begin() +
                                         index);
      selectedServer.unique_code.clear();
      selectedServer.isCustom = false;
      break;
    }
    index++;
  }
  mergedConnexionInfoList = temp_customConnexionInfoList;
  mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),
                                 temp_xmlConnexionInfoList.begin(),
                                 temp_xmlConnexionInfoList.end());
  std::sort(mergedConnexionInfoList.begin(), mergedConnexionInfoList.end());
  saveConnexionInfoList();
  displayServerList();
}

void Multi::httpFinished() {
  warning->SetVisible(false);
  QVariant redirectionTarget =
      reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (reply->error()) {
    warning->SetText(
        tr("Get server list failed: %1").arg(reply->errorString()));
    std::cerr << "Get server list failed: "
              << reply->errorString().toStdString() << std::endl;
    reply->deleteLater();
    reply = NULL;
    return;
  } else if (!redirectionTarget.isNull()) {
    warning->SetText(tr("Get server list redirection denied to: %1")
                         .arg(reply->errorString()));
    std::cerr << "Get server list redirection denied to: "
              << reply->errorString().toStdString() << std::endl;
    reply->deleteLater();
    reply = NULL;
    return;
  }
  std::cout << "Got new server list" << std::endl;

  QByteArray content = reply->readAll();
  QString wPath =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir().mkpath(wPath);
  QFile cache(wPath + QStringLiteral("/server_list.xml"));
  if (cache.open(QIODevice::ReadWrite)) {
    cache.write(content);
    cache.resize(content.size());
    QVariant val = reply->header(QNetworkRequest::LastModifiedHeader);
    if (val.isValid()) {
#ifdef Q_CC_GNU
      // this function avalaible on unix and mingw
      utimbuf butime;
      butime.actime = val.toDateTime().toTime_t();
      butime.modtime = val.toDateTime().toTime_t();
      int returnVal = utime(cache.fileName().toLocal8Bit().data(), &butime);
      if (returnVal == 0)
        return;
      else {
        qDebug() << QStringLiteral("Can't set time: %1").arg(cache.fileName());
        return;
      }
#else
#error "Not supported on this platform"
#endif
    }
    cache.close();
  } else
    std::cerr << "Can't write server list cache" << std::endl;
  temp_xmlConnexionInfoList = loadXmlConnexionInfoList(content);
  mergedConnexionInfoList = temp_customConnexionInfoList;
  mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),
                                 temp_xmlConnexionInfoList.begin(),
                                 temp_xmlConnexionInfoList.end());
  std::cout << "mergedConnexionInfoList.size(): "
            << mergedConnexionInfoList.size() << std::endl;
  std::sort(mergedConnexionInfoList.begin(),
            mergedConnexionInfoList.end());  // qSort(mergedConnexionInfoList);
  displayServerList();
  reply->deleteLater();
  reply = nullptr;
}

void Multi::saveConnexionInfoList() {
  QSet<QString> valid;
  unsigned int index = 0;
  while (index < temp_customConnexionInfoList.size()) {
    const ConnectionInfo &connexionInfo =
        temp_customConnexionInfoList.at(index);
    if (connexionInfo.unique_code.isEmpty()) abort();

    if (connexionInfo.isCustom) {
      valid.insert(QStringLiteral("Custom-%1").arg(connexionInfo.unique_code));
      Settings::settings->beginGroup(
          QStringLiteral("Custom-%1").arg(connexionInfo.unique_code));
      if (!connexionInfo.ws.isEmpty()) {
        Settings::settings->setValue(QStringLiteral("ws"), connexionInfo.ws);
        Settings::settings->remove("host");
        Settings::settings->remove("port");
      } else {
        Settings::settings->setValue(QStringLiteral("host"),
                                     connexionInfo.host);
        Settings::settings->setValue(QStringLiteral("port"),
                                     connexionInfo.port);
        Settings::settings->remove("ws");
      }
      Settings::settings->setValue(QStringLiteral("name"), connexionInfo.name);
    } else
      Settings::settings->beginGroup(
          QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
    if (connexionInfo.connexionCounter > 0)
      Settings::settings->setValue(QStringLiteral("connexionCounter"),
                                   connexionInfo.connexionCounter);
    else
      Settings::settings->remove(QStringLiteral("connexionCounter"));
    if (connexionInfo.lastConnexion > 0 && connexionInfo.connexionCounter > 0)
      Settings::settings->setValue(QStringLiteral("lastConnexion"),
                                   (quint64)connexionInfo.lastConnexion);
    else
      Settings::settings->remove(QStringLiteral("lastConnexion"));
    Settings::settings->setValue(QStringLiteral("proxyHost"),
                                 connexionInfo.proxyHost);
    Settings::settings->setValue(QStringLiteral("proxyPort"),
                                 connexionInfo.proxyPort);
    Settings::settings->endGroup();
    index++;
  }
  QStringList groups = Settings::settings->childGroups();
  index = 0;
  while (index < (unsigned int)groups.size()) {
    const QString &groupName = groups.at(index);
    if (groupName.startsWith("Custom-") && !valid.contains(groupName))
      Settings::settings->remove(groupName);
    index++;
  }
  Settings::settings->sync();
}

std::vector<ConnectionInfo> Multi::loadXmlConnexionInfoList() {
  QString wPath =
      QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  if (QFileInfo(wPath + "/server_list.xml").isFile())
    return loadXmlConnexionInfoList(wPath + "/server_list.xml");
  return loadXmlConnexionInfoList(
      QStringLiteral(":/CC/other/default_server_list.xml"));
}

std::vector<ConnectionInfo> Multi::loadXmlConnexionInfoList(
    const QByteArray &xmlContent) {
  std::vector<ConnectionInfo> returnedVar;
  tinyxml2::XMLDocument domDocument;
  const auto loadOkay = domDocument.Parse(xmlContent.data(), xmlContent.size());
  if (loadOkay != 0) {
    std::cerr << "Multi::loadXmlConnexionInfoList, " << domDocument.ErrorName()
              << std::endl;
    return returnedVar;
  }

  const tinyxml2::XMLElement *root = domDocument.RootElement();
  if (root == NULL) {
    std::cerr << "Unable to open the file: Multi::loadXmlConnexionInfoList, no "
                 "root balise found for the xml file"
              << std::endl;
    return returnedVar;
  }
  if (root->Name() == NULL) {
    qDebug() << QStringLiteral(
                    "Unable to open the file: %1, \"servers\" root balise not "
                    "found 2 for the xml file")
                    .arg("server_list.xml");
    return returnedVar;
  }
  if (strcmp(root->Name(), "servers") != 0) {
    qDebug() << QStringLiteral(
                    "Unable to open the file: %1, \"servers\" root balise not "
                    "found for the xml file")
                    .arg("server_list.xml");
    return returnedVar;
  }

  QRegularExpression regexHost("^[a-zA-Z0-9\\.\\-_]+$");
  bool ok;
  // load the content
  const tinyxml2::XMLElement *server = root->FirstChildElement("server");
#ifndef CATCHCHALLENGER_BOT
  const std::string &language = Language::language.getLanguage().toStdString();
#else
  const std::string language("en");
#endif
  while (server != NULL) {
    if (server->Attribute("unique_code") != NULL) {
      ConnectionInfo connexionInfo;
      connexionInfo.isCustom = false;
      connexionInfo.unique_code = server->Attribute("unique_code");
      connexionInfo.port = 0;
      connexionInfo.connexionCounter = 0;
      connexionInfo.lastConnexion = 0;
      connexionInfo.proxyPort = 0;

      if (server->Attribute("host") != NULL)
        connexionInfo.host = server->Attribute("host");
      if (server->Attribute("port") != NULL) {
        uint32_t temp_port = stringtouint32(server->Attribute("port"), &ok);
        if (!connexionInfo.host.contains(regexHost))
          qDebug() << QStringLiteral(
                          "Unable to open the file: %1, host is wrong: %3 "
                          "child->Name(): %2")
                          .arg("server_list.xml")
                          .arg(server->Name())
                          .arg(connexionInfo.host);
        else if (!ok)
          qDebug() << QStringLiteral(
                          "Unable to open the file: %1, port is not a number: "
                          "%3 child->Name(): %2")
                          .arg("server_list.xml")
                          .arg(server->Name())
                          .arg(server->Attribute("port"));
        else if (temp_port < 1 || temp_port > 65535)
          qDebug() << QStringLiteral(
                          "Unable to open the file: %1, port is not in range: "
                          "%3 child->Name(): %2")
                          .arg("server_list.xml")
                          .arg(server->Name())
                          .arg(server->Attribute("port"));
        else if (connexionInfo.unique_code.isEmpty())
          qDebug() << QStringLiteral(
                          "Unable to open the file: %1, unique_code can't be "
                          "empty: %3 child->Name(): %2")
                          .arg("server_list.xml")
                          .arg(server->Name())
                          .arg(server->Attribute("port"));
        else
          connexionInfo.port = static_cast<uint16_t>(temp_port);
      }
      if (server->Attribute("ws") != NULL)
        connexionInfo.ws = server->Attribute("ws");
      const tinyxml2::XMLElement *lang;
      bool found = false;
      if (!language.empty() && language != "en") {
        lang = server->FirstChildElement("lang");
        while (lang != NULL) {
          if (lang->Attribute("lang") && lang->Attribute("lang") == language) {
            const tinyxml2::XMLElement *name = lang->FirstChildElement("name");
            if (name != NULL && name->GetText() != NULL)
              connexionInfo.name = name->GetText();
            const tinyxml2::XMLElement *register_page =
                lang->FirstChildElement("register_page");
            if (register_page != NULL && register_page->GetText() != NULL)
              connexionInfo.register_page = register_page->GetText();
            const tinyxml2::XMLElement *lost_passwd_page =
                lang->FirstChildElement("lost_passwd_page");
            if (lost_passwd_page != NULL && lost_passwd_page->GetText() != NULL)
              connexionInfo.lost_passwd_page = lost_passwd_page->GetText();
            const tinyxml2::XMLElement *site_page =
                lang->FirstChildElement("site_page");
            if (site_page != NULL && site_page->GetText() != NULL)
              connexionInfo.site_page = site_page->GetText();
            found = true;
            break;
          }
          lang = lang->NextSiblingElement("lang");
        }
      }
      if (!found) {
        lang = server->FirstChildElement("lang");
        while (lang != NULL) {
          if (lang->Attribute("lang") == NULL ||
              strcmp(lang->Attribute("lang"), "en") == 0) {
            const tinyxml2::XMLElement *name = lang->FirstChildElement("name");
            if (name != NULL && name->GetText() != NULL)
              connexionInfo.name = name->GetText();
            const tinyxml2::XMLElement *register_page =
                lang->FirstChildElement("register_page");
            if (register_page != NULL && register_page->GetText() != NULL)
              connexionInfo.register_page = register_page->GetText();
            const tinyxml2::XMLElement *lost_passwd_page =
                lang->FirstChildElement("lost_passwd_page");
            if (lost_passwd_page != NULL && lost_passwd_page->GetText() != NULL)
              connexionInfo.lost_passwd_page = lost_passwd_page->GetText();
            const tinyxml2::XMLElement *site_page =
                lang->FirstChildElement("site_page");
            if (site_page != NULL && site_page->GetText() != NULL)
              connexionInfo.site_page = site_page->GetText();
            break;
          }
          lang = lang->NextSiblingElement("lang");
        }
      }
      Settings::settings->beginGroup(
          QStringLiteral("Xml-%1").arg(server->Attribute("unique_code")));
      if (Settings::settings->contains(QStringLiteral("connexionCounter"))) {
        connexionInfo.connexionCounter =
            Settings::settings->value("connexionCounter").toUInt(&ok);
        if (!ok) connexionInfo.connexionCounter = 0;
      } else
        connexionInfo.connexionCounter = 0;

      // proxy
      if (Settings::settings->contains(QStringLiteral("proxyPort"))) {
        connexionInfo.proxyPort = static_cast<uint16_t>(
            Settings::settings->value("proxyPort").toUInt(&ok));
        if (!ok) connexionInfo.proxyPort = 9050;
      } else
        connexionInfo.proxyPort = 9050;
      if (Settings::settings->contains(QStringLiteral("proxyHost")))
        connexionInfo.proxyHost =
            Settings::settings->value(QStringLiteral("proxyHost")).toString();
      else
        connexionInfo.proxyHost = QString();

      if (Settings::settings->contains(QStringLiteral("lastConnexion"))) {
        connexionInfo.lastConnexion =
            Settings::settings->value(QStringLiteral("lastConnexion"))
                .toUInt(&ok);
        if (!ok)
          connexionInfo.lastConnexion =
              static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);
      } else
        connexionInfo.lastConnexion =
            static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);

      // name
      Settings::settings->endGroup();
      if (connexionInfo.lastConnexion >
          (QDateTime::currentMSecsSinceEpoch() / 1000))
        connexionInfo.lastConnexion =
            static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);
      returnedVar.push_back(connexionInfo);
    } else
      qDebug() << QStringLiteral(
                      "Unable to open the file: %1, missing host or port: "
                      "child->Name(): %2")
                      .arg("server_list.xml")
                      .arg(server->Name());
    server = server->NextSiblingElement("server");
  }
  return returnedVar;
}

std::vector<ConnectionInfo> Multi::loadXmlConnexionInfoList(
    const QString &file) {
  std::vector<ConnectionInfo> returnedVar;
  // open and quick check the file
  QFile itemsFile(file);
  QByteArray xmlContent;
  if (!itemsFile.open(QIODevice::ReadOnly)) {
    qDebug() << QStringLiteral("Unable to open the file: %1, error: %2")
                    .arg(itemsFile.fileName())
                    .arg(itemsFile.errorString());
    return returnedVar;
  }
  xmlContent = itemsFile.readAll();
  itemsFile.close();
  return loadXmlConnexionInfoList(xmlContent);
}

std::vector<ConnectionInfo> Multi::loadConfigConnexionInfoList() {
  std::vector<ConnectionInfo> returnedVar;
  QStringList groups = Settings::settings->childGroups();
  int index = 0;
  while (index < groups.size()) {
    const QString &groupName = groups.at(index);
    Settings::settings->beginGroup(groupName);
    if ((Settings::settings->contains("host") &&
         Settings::settings->contains("port")) ||
        Settings::settings->contains("ws")) {
      QString ws = "";
      if (Settings::settings->contains("ws"))
        ws = Settings::settings->value("ws").toString();
      QString Shost = "";
      if (Settings::settings->contains("host"))
        Shost = Settings::settings->value("host").toString();
      QString port_string = "";
      if (Settings::settings->contains("port"))
        port_string = Settings::settings->value("port").toString();

      QString name = "";
      if (Settings::settings->contains("name"))
        name = Settings::settings->value("name").toString();
      QString connexionCounter = "0";
      if (Settings::settings->contains("connexionCounter"))
        connexionCounter =
            Settings::settings->value("connexionCounter").toString();
      QString lastConnexion = "0";
      if (Settings::settings->contains("lastConnexion"))
        lastConnexion = Settings::settings->value("lastConnexion").toString();

      QString proxyHost = "";
      if (Settings::settings->contains("proxyHost"))
        proxyHost = Settings::settings->value("proxyHost").toString();
      QString proxyPort = "0";
      if (Settings::settings->contains("proxyPort"))
        proxyPort = Settings::settings->value("proxyPort").toString();

      bool ok = true;
      ConnectionInfo connexionInfo;
      connexionInfo.isCustom = false;
      connexionInfo.port = 0;
      connexionInfo.connexionCounter = 0;
      connexionInfo.lastConnexion = 0;
      connexionInfo.proxyPort = 0;

      if (!ws.isEmpty())
        connexionInfo.ws = ws;
      else {
#ifndef NOTCPSOCKET
        uint16_t port = static_cast<uint16_t>(port_string.toInt(&ok));
        if (!ok)
          qDebug() << "dropped connexion, port wrong: " << port_string;
        else {
          connexionInfo.host = Shost;
          connexionInfo.port = port;
        }
#else
        ok = false;
#endif
      }
      if (groupName.startsWith("Custom-"))
        connexionInfo.isCustom = true;
      else if (groupName.startsWith("Xml-"))
        connexionInfo.isCustom = false;
      else
        ok = false;
      if (ok) {
        if (groupName.startsWith("Custom-"))
          connexionInfo.unique_code = groupName.mid(7);
        else  // Xml-
          connexionInfo.unique_code = groupName.mid(7);
        connexionInfo.name = name;
        connexionInfo.connexionCounter = connexionCounter.toUInt(&ok);
        if (!ok) {
          qDebug() << "ignored bug for connexion : connexionCounter";
          connexionInfo.connexionCounter = 0;
        }
        connexionInfo.lastConnexion = lastConnexion.toUInt(&ok);
        if (!ok) {
          qDebug() << "ignored bug for connexion : lastConnexion";
          connexionInfo.lastConnexion =
              static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);
        }
        if (connexionInfo.lastConnexion >
            (QDateTime::currentMSecsSinceEpoch() / 1000)) {
          qDebug() << "ignored bug for connexion : lastConnexion<time()";
          connexionInfo.lastConnexion =
              static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() / 1000);
        }
        if (!proxyHost.isEmpty()) {
          bool ok;
          uint16_t proxy_port = static_cast<uint16_t>(proxyPort.toInt(&ok));
          if (ok) {
            connexionInfo.proxyHost = proxyHost;
            connexionInfo.proxyPort = proxy_port;
          }
        }
        returnedVar.push_back(connexionInfo);
      }
    } else
      qDebug() << "dropped connexion, info seam wrong";
    index++;
    Settings::settings->endGroup();
  }
  return returnedVar;
}

void Multi::downloadFile() {
#ifndef __EMSCRIPTEN__
  QString catchChallengerVersion;
  if (Ultimate::ultimate.isUltimate())
    catchChallengerVersion =
        QStringLiteral("CatchChallenger Ultimate/%1")
            .arg(QString::fromStdString(CatchChallenger::Version::str));
  else
    catchChallengerVersion =
        QStringLiteral("CatchChallenger/%1")
            .arg(QString::fromStdString(CatchChallenger::Version::str));

#if defined(_WIN32) || defined(Q_OS_MAC)
  catchChallengerVersion +=
      QStringLiteral(" (OS: %1)")
          .arg(QString::fromStdString(InternetUpdater::GetOSDisplayString()));
#endif
  catchChallengerVersion += QStringLiteral(" ") + CATCHCHALLENGER_PLATFORM_CODE;
#endif

  QNetworkRequest networkRequest;
  networkRequest.setUrl(QUrl(CATCHCHALLENGER_SERVER_LIST_URL));
#ifndef __EMSCRIPTEN__
  networkRequest.setHeader(QNetworkRequest::UserAgentHeader,
                           catchChallengerVersion);
#endif

  reply = qnam.get(networkRequest);
  QObject::connect(reply, &QNetworkReply::finished, [=]() { httpFinished(); });
  // if(!connect(reply, &QNetworkReply::metaDataChanged, this,
  // &MainWindow::metaDataChanged)) abort(); seam buggy
  warning->SetVisible(true);
  warning->SetText(tr("Loading the server list..."));
  // ui->server_refresh->setEnabled(false);
}

void Multi::on_server_refresh_clicked() {
  if (reply != NULL) {
    reply->abort();
    reply->deleteLater();
    reply = NULL;
  }
  //downloadFile();
}

void Multi::server_select_clicked() {
  if (login == nullptr) {
    login = Login::Create();
    login->SetOnClose(std::bind(&Multi::server_select_finished, this));
  }
  if (selectedServer.isCustom)
    Settings::settings->beginGroup(
        QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
  else
    Settings::settings->beginGroup(
        QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
  if (Settings::settings->contains("last"))
    login->setAuth(Settings::settings->value("last").toStringList());
  int index = 0;
  while ((unsigned int)index < (unsigned int)mergedConnexionInfoList.size()) {
    auto e = mergedConnexionInfoList.at(index);
    if (e.isCustom == selectedServer.isCustom &&
        e.unique_code == selectedServer.unique_code) {
      login->setLinks(e.site_page, e.register_page);
      break;
    }
    index++;
  }
  Settings::settings->endGroup();
  AddChild(login);
}

void Multi::server_select_finished() {
  RemoveChild(login);
  if (login == nullptr) return;
  if (!login->isOk()) return;
  if (selectedServer.isCustom)
    Settings::settings->beginGroup(
        QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
  else
    Settings::settings->beginGroup(
        QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
  QStringList v = login->getAuth();
  if (!v.isEmpty()) {
    Settings::settings->setValue("last", v);
    Settings::settings->sync();
  }
  Settings::settings->endGroup();
#ifdef __EMSCRIPTEN__
  std::cerr << "server_select_finished returned" << std::endl;
#endif

  if (selectedServer.isCustom)
    Settings::settings->beginGroup(
        QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
  else
    Settings::settings->beginGroup(
        QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
  const QString loginString = login->getLogin();
  Settings::settings->endGroup();
  Settings::settings->sync();
  unsigned int index = 0;
  while (index < mergedConnexionInfoList.size()) {
    ConnectionInfo &connexionInfo = mergedConnexionInfoList[index];
    if (connexionInfo.isCustom == selectedServer.isCustom &&
        connexionInfo.unique_code == selectedServer.unique_code) {
      connexionInfo.connexionCounter++;
      saveConnexionInfoList();
      displayServerList();  // need be after connectTheExternalSocket() because
                            // it reset selectedServer
      ConnectToServer(connexionInfo, loginString, login->getPass());
      break;
    }
    index++;
  }
}

void Multi::newLanguage() {
  server_add->SetText(tr("Add"));
  // server_remove->setText(tr("Remove"));
  server_edit->SetText(tr("Edit"));
  /*
  warning->setHtml(
      "<span style=\"background-color: rgb(255, 255, 163);\nborder: 1px solid "
      "rgb(255, 221, 50);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\">" +
      tr("Loading the servers list...") + "</span>");
  serverEmpty->setHtml(
      QStringLiteral(
          "<html><body><p align=\"center\"><span "
          "style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>")
          .arg(tr("Empty")));
  */
}

void Multi::OnScreenSD() {}

void Multi::OnScreenHD() {}

void Multi::OnScreenHDR() {}

void Multi::OnScreenResize() {
  unsigned int space = 10;
  //    unsigned int fontSize=20;
  unsigned int multiItemH = 100;
  if (BoundingRect().width() < 600 || BoundingRect().height() < 600) {
    server_add->SetSize(148, 61);
    server_add->SetPixelSize(23);
    server_remove->SetSize(56, 62);
    server_edit->SetSize(148, 61);
    server_edit->SetPixelSize(23);
    server_refresh->SetSize(56, 62);
    server_select->SetSize(56, 62);
    back->SetSize(56, 62);
    multiItemH = 50;
  } else if (BoundingRect().width() < 900 || BoundingRect().height() < 600) {
    server_add->SetSize(148, 61);
    server_add->SetPixelSize(23);
    server_remove->SetSize(56, 62);
    server_edit->SetSize(148, 61);
    server_edit->SetPixelSize(23);
    server_refresh->SetSize(56, 62);
    server_select->SetSize(84, 93);
    back->SetSize(84, 93);
    multiItemH = 75;
  } else {
    space = 30;
    server_add->SetSize(223, 92);
    server_add->SetPixelSize(35);
    server_remove->SetSize(84, 93);
    server_edit->SetSize(223, 92);
    server_edit->SetPixelSize(35);
    server_refresh->SetSize(84, 93);
    server_select->SetSize(84, 93);
    back->SetSize(84, 93);
  }

  unsigned int tWidthTButton =
      server_add->Width() + space + server_edit->Width() + space +
      server_remove->Width() + space + server_refresh->Width();
  unsigned int tXTButton = BoundingRect().width() / 2 - tWidthTButton / 2;
  unsigned int tWidthTButtonOffset = 0;
  unsigned int y = BoundingRect().height() - space - server_select->Height() -
                   space - server_add->Height();
  server_remove->SetPos(tXTButton + tWidthTButtonOffset, y);
  tWidthTButtonOffset += server_remove->Width() + space;
  server_add->SetPos(tXTButton + tWidthTButtonOffset, y);
  tWidthTButtonOffset += server_add->Width() + space;
  server_edit->SetPos(tXTButton + tWidthTButtonOffset, y);
  tWidthTButtonOffset += server_edit->Width() + space;
  server_refresh->SetPos(tXTButton + tWidthTButtonOffset, y);

  tWidthTButton = server_select->Width() + space + back->Width();
  y = BoundingRect().height() - space - server_select->Height();
  tXTButton = BoundingRect().width() / 2 - tWidthTButton / 2;
  back->SetPos(tXTButton, y);
  server_select->SetPos(tXTButton + back->Width() + space, y);

  y = BoundingRect().height() - space - server_select->Height() - space -
      server_add->Height();

  int border_size = 46;
  int height = BoundingRect().height() - space - server_select->Height() -
               space - server_add->Height() - space - space;
  int width = 0;
  if ((unsigned int)BoundingRect().width() < (800 + space * 2)) {
    width = BoundingRect().width() - space * 2;
    wdialog->SetPos(space, space);
  } else {
    width = 800;
    wdialog->SetPos(BoundingRect().width() / 2 - width / 2, space);
  }
  wdialog->Strech(border_size, width, height);
  warning->SetY(space + wdialog->Height() - border_size - warning->Height());
  warning->SetWidth(bounding_rect_.width());

  scrollZone->SetSize(wdialog->Width() - border_size,
                      wdialog->Height() - border_size);
  scrollZone->SetPos(border_size / 2, border_size / 2);
}

void Multi::GoBack() { static_cast<StackedScene *>(Parent())->PopForeground(); }

void Multi::OnEnter() {
  Scene::OnEnter();
  server_add->RegisterEvents();
  server_remove->RegisterEvents();
  server_edit->RegisterEvents();
  server_select->RegisterEvents();
  back->RegisterEvents();
  server_refresh->RegisterEvents();
  scrollZone->RegisterEvents();
}

void Multi::OnExit() {
  scrollZone->UnRegisterEvents();
  server_add->UnRegisterEvents();
  server_remove->UnRegisterEvents();
  server_edit->UnRegisterEvents();
  server_select->UnRegisterEvents();
  back->UnRegisterEvents();
  server_refresh->UnRegisterEvents();
}

void Multi::OnSelectServer(Node *item) {
  MultiItem *selected = static_cast<MultiItem *>(item);
  const ConnectionInfo &info = selected->Connection();
  selectedServer.unique_code = info.unique_code;
  selectedServer.isCustom = info.isCustom;
  server_edit->SetEnabled(info.isCustom);
  server_remove->SetEnabled(info.isCustom);
  server_select->SetEnabled(!selectedServer.unique_code.isEmpty());
  auto items = scrollZone->Items();
  for (auto it = begin(items); it != end(items); it++) {
    if ((*it) == item) continue;
    static_cast<MultiItem *>((*it))->SetSelected(false);
  }
}

void Multi::ConnectToServer(ConnectionInfo connexionInfo, QString login,
                            QString pass) {
  if (!subserver_) subserver_ = SubServer::Create();
  static_cast<StackedScene *>(Parent())->PushForeground(subserver_);

  // work around for the resetAll() of protocol
  const std::string mainDatapackCode =
      CommonSettingsServer::commonSettingsServer.mainDatapackCode;
  const std::string subDatapackCode =
      CommonSettingsServer::commonSettingsServer.subDatapackCode;
  ConnectionManager::GetInstance()->connectToServer(connexionInfo, login, pass);
  CommonSettingsServer::commonSettingsServer.mainDatapackCode =
      mainDatapackCode;
  CommonSettingsServer::commonSettingsServer.subDatapackCode = subDatapackCode;
}
