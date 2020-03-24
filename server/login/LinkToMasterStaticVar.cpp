#include "LinkToMaster.hpp"

using namespace CatchChallenger;

unsigned char LinkToMaster::header_magic_number[]={0xB8,0x00};
unsigned char LinkToMaster::private_token_master[];
unsigned char LinkToMaster::private_token_statclient[];
unsigned char LinkToMaster::queryNumberToCharacterGroup[CATCHCHALLENGER_MAXPROTOCOLQUERY];
