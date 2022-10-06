// Copyright 2021 CatchChallenger
#include "Solo.hpp"

#include <QObject>
#include <QStandardPaths>
#include <string>
#include <iostream>

#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/base/CompressionProtocol.hpp"
#include "../../../../general/base/FacilityLibGeneral.hpp"
#include "../../../libqtcatchchallenger/Settings.hpp"
#include "../../Globals.hpp"
#include "../../base/ConnectionInfo.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/StackedScene.hpp"
#include "../../entities/Utils.hpp"
#include "Loading.hpp"
#ifdef __ANDROID_API__
#include "../../entities/JniMessenger.hpp"
#endif

using Scenes::Solo;
using std::placeholders::_1;

Solo::Solo() : Scene(nullptr) {
  subserver_ = nullptr;
  AddChild(Loading::GetInstance());
  Initialize();
}

Solo::~Solo() {
  if (subserver_) {
    delete subserver_;
    subserver_ = nullptr;
  }
}

Solo *Solo::Create() { return new (std::nothrow) Solo(); }

void Solo::Initialize() {
  CommonSettingsServer::commonSettingsServer.mainDatapackCode = "[main]";
  CommonSettingsServer::commonSettingsServer.subDatapackCode = "[sub]";
  QStringList l =
      QStandardPaths::standardLocations(QStandardPaths::DataLocation);
  if (l.empty()) {
    PrintError(tr("No writable path").toStdString());
    return;
  }

  QString savegamesPath = l.first() + "/solo/";
  // locate the new folder and create it
  if (!QDir().mkpath(savegamesPath)) {
    PrintError(tr("Unable to write savegame into: %1")
                   .arg(savegamesPath)
                   .toStdString());
    return;
  }

  // initialize the db
  if (!QFile(savegamesPath + "catchchallenger.db.sqlite").exists()) {
    QByteArray dbData;
    {
      QFile dbSource(QStringLiteral(":/catchchallenger.db.sqlite"));
      if (!dbSource.open(QIODevice::ReadOnly)) {
        PrintError(QStringLiteral("Unable to open the db model: %1")
                       .arg(savegamesPath)
                       .toStdString());
        CatchChallenger::FacilityLibGeneral::rmpath(
            savegamesPath.toStdString());
        return;
      }
      dbData = dbSource.readAll();
      if (dbData.isEmpty()) {
        PrintError(QStringLiteral("Unable to read the db model: %1")
                       .arg(savegamesPath)
                       .toStdString());
        CatchChallenger::FacilityLibGeneral::rmpath(
            savegamesPath.toStdString());
        return;
      }
      dbSource.close();
    }
    {
      QFile dbDestination(savegamesPath + "catchchallenger.db.sqlite");
      if (!dbDestination.open(QIODevice::WriteOnly)) {
        PrintError(QStringLiteral("Unable to write savegame into: %1")
                       .arg(savegamesPath)
                       .toStdString());
        CatchChallenger::FacilityLibGeneral::rmpath(
            savegamesPath.toStdString());
        return;
      }
      if (dbDestination.write(dbData) < 0) {
        dbDestination.close();
        PrintError(QStringLiteral("Unable to write savegame into: %1")
                       .arg(savegamesPath)
                       .toStdString());
        CatchChallenger::FacilityLibGeneral::rmpath(
            savegamesPath.toStdString());
        return;
      }
      dbDestination.close();
    }
  }

  if (!Settings::settings->contains("pass")) {
    // initialise the pass
    QString pass = QString::fromStdString(
        CatchChallenger::FacilityLibGeneral::randomPassword(
            "abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",
            32));

    // initialise the meta data
    Settings::settings->setValue("pass", pass);
    Settings::settings->sync();
    if (Settings::settings->status() != QSettings::NoError) {
      PrintError(QStringLiteral("Unable to write savegame pass into: %1")
                     .arg(savegamesPath)
                     .toStdString());
      CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
      return;
    }
  }

  if (Globals::InternalServer != nullptr) {
    if (!Globals::InternalServer->isStopped()) {
      Globals::InternalServer->stop();
    }
    Globals::InternalServer->deleteLater();
    Globals::InternalServer = nullptr;
  }
  if (Globals::InternalServer == nullptr) {
    Globals::InternalServer = new CatchChallenger::InternalServer();
#ifndef NOTHREADS
    Globals::ThreadSolo->start();
// internalServer->moveToThread(threadSolo);//-> Timers cannot be stopped from
// another thread, fixed by using QThread InternalServer::run()
#endif
  }
  QObject::connect(Globals::InternalServer,
                   &CatchChallenger::InternalServer::is_started,
                   std::bind(&Solo::IsStarted, this, _1));
  Loading::GetInstance()->SetText("Open the local game");

  {
    // std::string datapackPathBase=client->datapackPathBase();
    std::string datapackPathBase =
        QCoreApplication::applicationDirPath().toStdString() + "/datapack/";
#ifdef __ANDROID_API__
    auto datapackFolder = JniMessenger::DecompressDatapack();
    datapackPathBase = datapackFolder.toStdString();
#endif
    CatchChallenger::GameServerSettings formatedServerSettings =
        Globals::InternalServer->getSettings();

    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick = 0;
    CommonSettingsCommon::commonSettingsCommon.max_character = 1;
    CommonSettingsCommon::commonSettingsCommon.min_character = 1;

    formatedServerSettings.automatic_account_creation = true;
    formatedServerSettings.max_players = 1;
    formatedServerSettings.sendPlayerNumber = false;
    formatedServerSettings.compressionType = CompressionProtocol::None;
    formatedServerSettings.everyBodyIsRoot = true;
    formatedServerSettings.teleportIfMapNotFoundOrOutOfMap = true;

    formatedServerSettings.database_login.tryOpenType =
        CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_login.file =
        (savegamesPath + QStringLiteral("catchchallenger.db.sqlite"))
            .toStdString();
    formatedServerSettings.database_base.tryOpenType =
        CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_base.file =
        (savegamesPath + QStringLiteral("catchchallenger.db.sqlite"))
            .toStdString();
    formatedServerSettings.database_common.tryOpenType =
        CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_common.file =
        (savegamesPath + QStringLiteral("catchchallenger.db.sqlite"))
            .toStdString();
    formatedServerSettings.database_server.tryOpenType =
        CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_server.file =
        (savegamesPath + QStringLiteral("catchchallenger.db.sqlite"))
            .toStdString();
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm =
        CatchChallenger::MapVisibilityAlgorithmSelection_None;
    formatedServerSettings.datapack_basePath = datapackPathBase;

    {
      CatchChallenger::GameServerSettings::ProgrammedEvent &event =
          formatedServerSettings.programmedEventList["day"]["day"];
      event.cycle = 60;
      event.offset = 0;
      event.value = "day";
    }
    {
      CatchChallenger::GameServerSettings::ProgrammedEvent &event =
          formatedServerSettings.programmedEventList["day"]["night"];
      event.cycle = 60;
      event.offset = 30;
      event.value = "night";
    }
    {
      const QFileInfoList &list =
          QDir(QString::fromStdString(datapackPathBase) + "/map/main/")
              .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
      if (list.isEmpty()) {
        PrintError((tr("No main code detected into the current datapack, "
                       "base, check: ") +
                    QString::fromStdString(datapackPathBase) + "/map/main/")
                       .toStdString());
        return;
      }
      CommonSettingsServer::commonSettingsServer.mainDatapackCode =
          list.at(0).fileName().toStdString();
    }
    QDir mainDir(
        QString::fromStdString(datapackPathBase) + "map/main/" +
        QString::fromStdString(
            CommonSettingsServer::commonSettingsServer.mainDatapackCode) +
        "/");
    if (!mainDir.exists()) {
      const QFileInfoList &list =
          QDir(QString::fromStdString(datapackPathBase) + "/map/main/")
              .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
      if (list.isEmpty()) {
        PrintError((tr("No main code detected into the current datapack, "
                       "empty main, check: ") +
                    QString::fromStdString(datapackPathBase) + "/map/main/")
                       .toStdString());
        return;
      }
      CommonSettingsServer::commonSettingsServer.mainDatapackCode =
          list.at(0).fileName().toStdString();
    }

    {
      const QFileInfoList &list =
          QDir(
              QString::fromStdString(datapackPathBase) + "/map/main/" +
              QString::fromStdString(
                  CommonSettingsServer::commonSettingsServer.mainDatapackCode) +
              "/sub/")
              .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
      if (!list.isEmpty())
        CommonSettingsServer::commonSettingsServer.subDatapackCode =
            list.at(0).fileName().toStdString();
      else
        CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
    }
    if (!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty()) {
      QDir subDir(
          QString::fromStdString(datapackPathBase) + "/map/main/" +
          QString::fromStdString(
              CommonSettingsServer::commonSettingsServer.mainDatapackCode) +
          "/sub/" +
          QString::fromStdString(
              CommonSettingsServer::commonSettingsServer.subDatapackCode) +
          "/");
      if (!subDir.exists()) {
        const QFileInfoList &list =
            QDir(QString::fromStdString(datapackPathBase) + "/map/main/" +
                 QString::fromStdString(
                     CommonSettingsServer::commonSettingsServer
                         .mainDatapackCode) +
                 "/sub/")
                .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        if (!list.isEmpty())
          CommonSettingsServer::commonSettingsServer.subDatapackCode =
              list.at(0).fileName().toStdString();
        else
          CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
      }
    }

    Globals::InternalServer->setSettings(formatedServerSettings);
  }

  Globals::InternalServer->start();
  // do after server is init connectToServer(connexionInfo,QString(),QString());
}

void Solo::IsStarted(bool started) {
  QObject::disconnect(Globals::InternalServer,
                      &CatchChallenger::InternalServer::is_started, nullptr,
                      nullptr);
  SceneManager::GetInstance()->RegisterInternalServerEvents();
  if (started) {
    ConnectionInfo connexionInfo;
    connexionInfo.connexionCounter = 0;
    connexionInfo.isCustom = false;
    connexionInfo.port = 0;
    connexionInfo.lastConnexion = 0;
    connexionInfo.proxyPort = 0;
    ConnectToServer(connexionInfo, "Admin",
                    Settings::settings->value("pass").toString());
    return;
  } else {
    if (Globals::InternalServer != nullptr) {
      delete Globals::InternalServer;
      Globals::InternalServer = nullptr;
    }
    QCoreApplication::quit();
  }
}

void Solo::ConnectToServer(ConnectionInfo connexionInfo, QString login,
                           QString pass) {
  Globals::SetSolo(true);

  ConnectionManager::ClearInstance();
  if (!subserver_) subserver_ = SubServer::Create();
  static_cast<StackedScene *>(Parent())->PushForeground(subserver_);

  // work around for the resetAll() of protocol
  const std::string mainDatapackCode =
      CommonSettingsServer::commonSettingsServer.mainDatapackCode;
  const std::string subDatapackCode =
      CommonSettingsServer::commonSettingsServer.subDatapackCode;
  auto connexionManager = ConnectionManager::GetInstance();
  connexionManager->connectToServer(connexionInfo, login, pass);
  CommonSettingsServer::commonSettingsServer.mainDatapackCode =
      mainDatapackCode;
  CommonSettingsServer::commonSettingsServer.subDatapackCode = subDatapackCode;
}

void Solo::OnScreenResize() {}
