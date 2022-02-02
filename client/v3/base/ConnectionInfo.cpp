// Copyright 2021 CatchChallenger
#include "ConnectionInfo.hpp"

bool ConnectionInfo::operator<(const ConnectionInfo &connexionInfo) const {
  if (connexionCounter < connexionInfo.connexionCounter) return false;
  if (connexionCounter > connexionInfo.connexionCounter) return true;
  if (lastConnexion < connexionInfo.lastConnexion) return false;
  if (lastConnexion > connexionInfo.lastConnexion) return true;
  return true;
}
