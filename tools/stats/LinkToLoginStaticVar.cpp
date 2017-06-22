#include "LinkToLogin.h"
#include "../../general/base/ProtocolVersion.h"

using namespace CatchChallenger;

unsigned char LinkToLogin::header_magic_number[]=PROTOCOL_HEADER_LOGIN;
unsigned char LinkToLogin::private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
#ifndef STATSODROIDSHOW2
FILE * LinkToLogin::pFile=NULL;
#else
int LinkToLogin::usbdev=0;
#endif
std::string LinkToLogin::pFilePath;
