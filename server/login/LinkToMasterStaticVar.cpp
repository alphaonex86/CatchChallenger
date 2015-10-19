#include "LinkToMaster.h"

using namespace CatchChallenger;

unsigned char LinkToMaster::header_magic_number[]={0xB8,0x00};
unsigned char LinkToMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];
