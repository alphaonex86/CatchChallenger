#include "EpollClientLoginMaster.h"

#include <iostream>

using namespace CatchChallenger;

const unsigned char EpollClientLoginMaster::protocolHeaderToMatch[]=PROTOCOL_HEADER_MASTERSERVER;
char EpollClientLoginMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];

unsigned char EpollClientLoginMaster::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyWrongAuth[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x05/*return code*/};
unsigned char EpollClientLoginMaster::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x06/*return code*/};
char EpollClientLoginMaster::selectCharaterRequestOnGameServer[]={0x02/*reply server to client*/,0x06/*query id*/,0x00/*the init reply query number*/};
unsigned char EpollClientLoginMaster::duplicateConnexionDetected[]={0xE1/*reply server to client*/,0x02/*query id*/};
unsigned char EpollClientLoginMaster::getTokenForCharacterSelect[]={0xF8/*reply server to client*/,0x00/*query id*/};
unsigned char EpollClientLoginMaster::replyToRegisterLoginServer[];
unsigned char EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset=0;
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
unsigned char EpollClientLoginMaster::replyToIdListBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/};
std::unordered_map<std::string,int> EpollClientLoginMaster::logicalGroupHash;

bool EpollClientLoginMaster::currentPlayerForGameServerToUpdate=false;

//start to 0 due to pre incrementation before use
uint32_t EpollClientLoginMaster::maxAccountId=0;

std::vector<EpollClientLoginMaster *> EpollClientLoginMaster::gameServers;
std::vector<EpollClientLoginMaster *> EpollClientLoginMaster::loginServers;
FILE *EpollClientLoginMaster::fpRandomFile=NULL;

