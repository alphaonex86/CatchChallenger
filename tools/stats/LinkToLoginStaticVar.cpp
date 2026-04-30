#include "LinkToLogin.h"
#include "../../general/base/ProtocolVersion.hpp"

using namespace CatchChallenger;

unsigned char LinkToLogin::header_magic_number[]=PROTOCOL_HEADER_LOGIN;
unsigned char LinkToLogin::private_token_statclient[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
#ifndef STATSODROIDSHOW2
FILE * LinkToLogin::pFile=NULL;
#else
int LinkToLogin::usbdev=0;
#endif
std::string LinkToLogin::pFilePath;
bool LinkToLogin::withIndentation=false;
