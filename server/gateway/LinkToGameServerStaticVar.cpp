#include "LinkToGameServer.hpp"

using namespace CatchChallenger;

std::vector<char> LinkToGameServer::httpDatapackMirrorRewriteBase;
std::vector<char> LinkToGameServer::httpDatapackMirrorRewriteMainAndSub;
bool LinkToGameServer::compressionSet=false;
std::string LinkToGameServer::mDatapackBase;
std::string LinkToGameServer::lastconnectErrorMessage;
const std::regex LinkToGameServer::mainDatapackCodeFilter=std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE,std::regex::optimize);
const std::regex LinkToGameServer::subDatapackCodeFilter=std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE,std::regex::optimize);
const std::string LinkToGameServer::protocolString("://");
const std::string LinkToGameServer::protocolHttp("http");
const std::string LinkToGameServer::protocolHttps("https");
