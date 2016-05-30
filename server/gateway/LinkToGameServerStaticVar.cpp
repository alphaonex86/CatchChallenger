#include "LinkToGameServer.h"

using namespace CatchChallenger;

std::vector<char> LinkToGameServer::httpDatapackMirrorRewriteBase;
std::vector<char> LinkToGameServer::httpDatapackMirrorRewriteMainAndSub;
bool LinkToGameServer::compressionSet=false;
std::string LinkToGameServer::mDatapackBase;
std::string LinkToGameServer::lastconnectErrorMessage;
