// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_BASE_CONNECTIONMANAGER_HPP_
#define CLIENT_QTOPENGL_BASE_CONNECTIONMANAGER_HPP_

#include <QObject>
#include <functional>
#include <string>

#include "../../../general/base/GeneralStructures.hpp"
#include "../../libqtcatchchallenger/ConnectedSocket.hpp"
#include "../../libqtcatchchallenger/Api_client_real.hpp"
#include "../../libqtcatchchallenger/Api_protocol_Qt.hpp"
#include "../core/ProgressNotifier.hpp"
#include "ConnectionInfo.hpp"
#ifdef CATCHCHALLENGER_SOLO
  #include "../../../server/qt/QFakeSocket.hpp"
#endif

class ConnectionManager : public ProgressNotifier, public QObject {
 public:
  static ConnectionManager *GetInstance();
  static void ClearInstance();
  void connectToServer(ConnectionInfo connexionInfo, QString login,
                       QString pass);
  void selectCharacter(const uint32_t indexSubServer,
                       const uint32_t indexCharacter);
  CatchChallenger::Api_protocol_Qt *client;

  void errorString(std::string error);
  void disconnectedFromServer();

  void SetOnLogged(
      std::function<
          void(std::vector<std::vector<CatchChallenger::CharacterEntry>>)>
          callback);
  void SetOnGoToMap(std::function<void()> callback);
  void SetOnError(std::function<void(std::string)> callback);
  void SetOnDisconnect(std::function<void()> callback);

  void disconnected(std::string reason);
#ifndef __EMSCRIPTEN__
  void sslErrors(const QList<QSslError> &errors);
#endif
  void connectTheExternalSocket(ConnectionInfo connexionInfo,
                                CatchChallenger::Api_client_real *client);
  QString serverToDatapachPath(ConnectionInfo connexionInfo) const;
  void stateChanged(QAbstractSocket::SocketState socketState);
  QAbstractSocket::SocketState state();
  void error(QAbstractSocket::SocketError socketError);
  void newError(const std::string &error, const std::string &detailedError);
  void message(const std::string &message);

  void protocol_is_good();
  void connectedOnLoginServer();
  void connectingOnGameServer();
  void connectedOnGameServer();
  void haveDatapackMainSubCode();
  void gatewayCacheUpdate(const uint8_t gateway, const uint8_t progression);

 private:
  static ConnectionManager *instance_;
  CatchChallenger::ConnectedSocket *socket;
#ifndef NOTCPSOCKET
  QSslSocket *realSslSocket;
#endif
#ifndef NOWEBSOCKET
  QWebSocket *realWebSocket;
#endif
#ifdef CATCHCHALLENGER_SOLO
  QFakeSocket *fakeSocket;
#endif
  QString lastServer;
  uint32_t datapckFileSize;

  bool haveDatapack;
  bool haveDatapackMainSub;
  bool datapackIsParsed;
  std::vector<std::vector<CatchChallenger::CharacterEntry>> characterEntryList;

  std::function<void(
      (std::vector<std::vector<CatchChallenger::CharacterEntry>>))>
      on_logged_;
  std::function<void()> on_go_to_map_;
  std::function<void()> on_disconnect_;
  std::function<void(std::string)> on_error_;

  ConnectionManager();
  void QtdatapackSizeBase(const uint32_t &datapckFileNumber,
                          const uint32_t &datapckFileSize);
  void QtdatapackSizeMain(const uint32_t &datapckFileNumber,
                          const uint32_t &datapckFileSize);
  void QtdatapackSizeSub(const uint32_t &datapckFileNumber,
                         const uint32_t &datapckFileSize);
  void progressingDatapackFileBase(const uint32_t &size);
  void progressingDatapackFileMain(const uint32_t &size);
  void progressingDatapackFileSub(const uint32_t &size);

  void sendDatapackContentMainSub();
  void haveTheDatapack();
  void haveTheDatapackMainSub();
  void datapackParsed();
  void datapackParsedMainSub();
  void datapackChecksumError();
  void Qtlogged(const std::vector<std::vector<CatchChallenger::CharacterEntry>>
                    &characterEntryList);
  void QthaveCharacter();
};

#endif  // CLIENT_QTOPENGL_BASE_CONNECTIONMANAGER_HPP_
