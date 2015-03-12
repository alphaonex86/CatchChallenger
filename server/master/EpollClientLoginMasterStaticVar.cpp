#include "EpollClientLoginMaster.h"

#include <iostream>

using namespace CatchChallenger;

const unsigned char EpollClientLoginMaster::protocolHeaderToMatch[]={0x88,0x62,0xBC,0xBB,0x67,0x9E,0x3D,0xE7};
char EpollClientLoginMaster::private_token[TOKEN_SIZE];

unsigned char EpollClientLoginMaster::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x02/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyWrongAuth[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x07/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x04/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x05/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x06/*return code*/};
