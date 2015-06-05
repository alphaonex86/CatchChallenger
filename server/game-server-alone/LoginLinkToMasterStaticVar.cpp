#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

char LoginLinkToMaster::protocolReplyNoMoreToken[]={0x41/*reply client to server*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
char LoginLinkToMaster::protocolReplyGetToken[3+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER]={0x41/*reply client to server*/,0x00/*the init reply query number*/,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER/*reply size*/};
char LoginLinkToMaster::sendDisconnectedPlayer[2+4]={0x45/*reply client to server*/,0x01/*the init reply query number*/};
unsigned char LoginLinkToMaster::header_magic_number_and_private_token[9+TOKEN_SIZE_FOR_MASTERAUTH]=PROTOCOL_HEADER_MASTERSERVER;
