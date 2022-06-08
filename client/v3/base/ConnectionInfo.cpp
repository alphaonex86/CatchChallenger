// Copyright 2021 CatchChallenger
#include "ConnectionInfo.hpp"

bool ConnectionInfo::operator<(const ConnectionInfo &connexionInfo) const {
  if (connexionCounter < connexionInfo.connexionCounter) return false;
  if (connexionCounter > connexionInfo.connexionCounter) return true;
  if (lastConnexion < connexionInfo.lastConnexion) return false;
  if (lastConnexion > connexionInfo.lastConnexion) return true;
  return true;
}

ConnectionInfo::Type ConnectionInfo::GetType() const {
  if (!host.isEmpty()){
    return ConnectionInfo::Type_TCP;
  } else if (!ws.isEmpty()) {
    return ConnectionInfo::Type_WebSocket;
  }
  return ConnectionInfo::Type_Solo;
}

bool ConnectionInfo::HasProxy() const {
  return !proxyHost.isEmpty();
}
