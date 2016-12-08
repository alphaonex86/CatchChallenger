#include "EpollClientLoginMaster.h"
#include "../../general/base/ProtocolVersion.h"

#include <iostream>

using namespace CatchChallenger;

const unsigned char EpollClientLoginMaster::protocolHeaderToMatch[]=PROTOCOL_HEADER_MASTERSERVER;
char EpollClientLoginMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];

unsigned char EpollClientLoginMaster::protocolReplyProtocolNotSupported[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x02/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyWrongAuth[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionNone[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
unsigned char EpollClientLoginMaster::protocolReplyCompresssionZlib[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,0x00,0x00,0x00/*reply size*/,0x05/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionXz[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,0x00,0x00,0x00/*reply size*/,0x06/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionLz4[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,0x00,0x00,0x00/*reply size*/,0x07/*return code*/};
#endif
unsigned char EpollClientLoginMaster::selectCharaterRequestOnGameServer[]={0x93,0x00/*the init reply query number*/};
unsigned char EpollClientLoginMaster::duplicateConnexionDetected[]={0x4D};
unsigned char EpollClientLoginMaster::getTokenForCharacterSelect[]={0xF8,0x00/*query id*/};
char EpollClientLoginMaster::loginSettingsAndCharactersGroup[];
unsigned int EpollClientLoginMaster::loginSettingsAndCharactersGroupSize=0;
char EpollClientLoginMaster::serverServerList[];
unsigned int EpollClientLoginMaster::serverServerListSize=0;
char EpollClientLoginMaster::serverPartialServerList[];
unsigned int EpollClientLoginMaster::serverPartialServerListSize=0;
char EpollClientLoginMaster::serverLogicalGroupList[];
unsigned int EpollClientLoginMaster::serverLogicalGroupListSize=0;
char EpollClientLoginMaster::loginPreviousToReplyCache[];
unsigned int EpollClientLoginMaster::loginPreviousToReplyCacheSize=0;
std::unordered_map<std::string,int> EpollClientLoginMaster::logicalGroupHash;
EpollClientLoginMaster::DataForUpdatedServers EpollClientLoginMaster::dataForUpdatedServers;

bool EpollClientLoginMaster::currentPlayerForGameServerToUpdate=false;
uint32_t EpollClientLoginMaster::lastSizeDisplayLoginServers=0;
uint32_t EpollClientLoginMaster::lastSizeDisplayGameServers=0;

//start to 0 due to pre incrementation before use
uint32_t EpollClientLoginMaster::maxAccountId=0;

std::vector<EpollClientLoginMaster *> EpollClientLoginMaster::gameServers;
std::vector<EpollClientLoginMaster *> EpollClientLoginMaster::loginServers;
FILE *EpollClientLoginMaster::fpRandomFile=NULL;

