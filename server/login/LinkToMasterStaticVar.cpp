#include "LinkToMaster.h"

using namespace CatchChallenger;

unsigned char LinkToMaster::header_magic_number[9]=PROTOCOL_HEADER_MASTERSERVER;
unsigned char LinkToMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];
