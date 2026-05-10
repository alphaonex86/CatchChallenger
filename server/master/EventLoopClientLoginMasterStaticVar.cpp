#include "EventLoopClientLoginMaster.hpp"
#include "../../general/base/ProtocolVersion.hpp"

#include <iostream>

using namespace CatchChallenger;

const unsigned char EventLoopClientLoginMaster::protocolHeaderToMatch[]=PROTOCOL_HEADER_MASTERSERVER;
char EventLoopClientLoginMaster::private_token[TOKEN_SIZE_FOR_MASTERAUTH];

unsigned char EventLoopClientLoginMaster::protocolReplyProtocolNotSupported[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x02/*return code*/};
unsigned char EventLoopClientLoginMaster::protocolReplyWrongAuth[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
unsigned char EventLoopClientLoginMaster::protocolReplyCompressionNone[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
unsigned char EventLoopClientLoginMaster::protocolReplyCompresssionZlib[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x05/*return code*/};
unsigned char EventLoopClientLoginMaster::protocolReplyCompressionXz[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x06/*return code*/};
unsigned char EventLoopClientLoginMaster::protocolReplyCompressionLz4[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x07/*return code*/};
#endif
unsigned char EventLoopClientLoginMaster::selectCharaterRequestOnGameServer[]={0x93,0x00/*the init reply query number*/};
unsigned char EventLoopClientLoginMaster::duplicateConnexionDetected[]={0x4D};
unsigned char EventLoopClientLoginMaster::getTokenForCharacterSelect[]={0xF8,0x00/*query id*/};
char EventLoopClientLoginMaster::loginSettingsAndCharactersGroup[];
unsigned int EventLoopClientLoginMaster::loginSettingsAndCharactersGroupSize=0;
char EventLoopClientLoginMaster::serverServerList[];
unsigned int EventLoopClientLoginMaster::serverServerListSize=0;
char EventLoopClientLoginMaster::serverPartialServerList[];
unsigned int EventLoopClientLoginMaster::serverPartialServerListSize=0;
char EventLoopClientLoginMaster::serverLogicalGroupList[];
unsigned int EventLoopClientLoginMaster::serverLogicalGroupListSize=0;
char EventLoopClientLoginMaster::loginPreviousToReplyCache[];
unsigned int EventLoopClientLoginMaster::loginPreviousToReplyCacheSize=0;
std::unordered_map<std::string,int> EventLoopClientLoginMaster::logicalGroupHash;
EventLoopClientLoginMaster::DataForUpdatedServers EventLoopClientLoginMaster::dataForUpdatedServers;

bool EventLoopClientLoginMaster::currentPlayerForGameServerToUpdate=false;
uint32_t EventLoopClientLoginMaster::lastSizeDisplayLoginServers=0;
uint32_t EventLoopClientLoginMaster::lastSizeDisplayGameServers=0;

//start to 0 due to pre incrementation before use
uint32_t EventLoopClientLoginMaster::maxAccountId=0;

std::vector<EventLoopClientLoginMaster *> EventLoopClientLoginMaster::gameServers;
std::vector<EventLoopClientLoginMaster *> EventLoopClientLoginMaster::loginServers;
FILE *EventLoopClientLoginMaster::fpRandomFile=NULL;
bool EventLoopClientLoginMaster::havePlayerCountChange=false;

