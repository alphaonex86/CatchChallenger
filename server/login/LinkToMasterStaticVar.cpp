#include "LinkToMaster.h"

using namespace CatchChallenger;

unsigned char LinkToMaster::header_magic_number_and_private_token[9+TOKEN_SIZE_FOR_MASTERAUTH]=PROTOCOL_HEADER_MASTERSERVER;
