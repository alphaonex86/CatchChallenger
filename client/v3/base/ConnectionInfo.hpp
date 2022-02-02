// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_BASE_CONNECTIONINFO_HPP_
#define CLIENT_QTOPENGL_BASE_CONNECTIONINFO_HPP_

#include <QString>

class ConnectionInfo {
 public:
  QString unique_code;
  QString name;
  bool isCustom;

  // hightest priority
  QString host;
  uint16_t port;
  // lower priority
  QString ws;

  uint32_t connexionCounter;
  uint64_t lastConnexion;  // presume timestamps, then uint64_t

  QString register_page;
  QString lost_passwd_page;
  QString site_page;

  QString proxyHost;
  uint16_t proxyPort;

  bool operator<(const ConnectionInfo &connexionInfo) const;
};

#endif  // CLIENT_QTOPENGL_BASE_CONNECTIONINFO_HPP_
