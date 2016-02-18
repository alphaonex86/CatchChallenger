#include "LinkToLogin.h"

using namespace CatchChallenger;

unsigned char LinkToLogin::header_magic_number[]=PROTOCOL_HEADER_LOGIN;
unsigned char LinkToLogin::private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
FILE * LinkToLogin::pFile=NULL;
std::string LinkToLogin::pFilePath;
