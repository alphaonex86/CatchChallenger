#include "LinkToMaster.h"

using namespace CatchChallenger;

char LinkToMaster::protocolReplyNoMoreToken[]={0x41/*reply client to server*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x04/*return code*/};
char LinkToMaster::protocolReplyAlreadyConnectedToken[]={0x41/*reply client to server*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
char LinkToMaster::protocolReplyGetToken[3+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER]={0x41/*reply client to server*/,0x00/*the init reply query number*/,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER/*reply size*/};
char LinkToMaster::sendDisconnectedPlayer[2+4]={0x45/*reply client to server*/,0x01/*the init reply query number*/};
char LinkToMaster::sendCurrentPlayer[2+2]={0x45/*reply client to server*/,0x02/*the init reply query number*/};
unsigned char LinkToMaster::header_magic_number[9]=PROTOCOL_HEADER_MASTERSERVER;
unsigned char LinkToMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];
