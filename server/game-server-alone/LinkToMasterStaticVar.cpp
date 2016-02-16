#include "LinkToMaster.h"

using namespace CatchChallenger;

char LinkToMaster::protocolReplyNoMoreToken[]={CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER/*reply client to server*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
char LinkToMaster::protocolReplyAlreadyConnectedToken[]={CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER/*reply client to server*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x03/*return code*/};
char LinkToMaster::protocolReplyGetToken[]={CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER/*reply client to server*/,0x00/*the init reply query number*/,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/};
char LinkToMaster::protocolReplyPing[]={CATCHCHALLENGER_PROTOCOL_REPLY_CLIENT_TO_SERVER/*reply client to server*/,0x00/*the init reply query number*/};
char LinkToMaster::sendDisconnectedPlayer[]={0x3E/*reply client to server*/};
char LinkToMaster::sendCurrentPlayer[]={0x3F/*reply client to server*/};
unsigned char LinkToMaster::header_magic_number[9]=PROTOCOL_HEADER_MASTERSERVER;
unsigned char LinkToMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];
