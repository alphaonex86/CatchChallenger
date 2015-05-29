#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

unsigned char LoginLinkToMaster::protocolReplyNoMoreToken[]={0x41/*reply client to server*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char LoginLinkToMaster::protocolReplyGetToken[3+32]={0x41/*reply client to server*/,0x00/*the init reply query number*/,32/*reply size*/};
unsigned char LoginLinkToMaster::sendDisconnectedPlayer[2+4]={0x45/*reply client to server*/,0x01/*the init reply query number*/};
