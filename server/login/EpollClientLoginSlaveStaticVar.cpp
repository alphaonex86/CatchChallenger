#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

std::vector<EpollClientLoginSlave *> EpollClientLoginSlave::client_list;
std::vector<unsigned int> EpollClientLoginSlave::maxAccountIdList;
std::vector<unsigned int> EpollClientLoginSlave::maxCharacterIdList;
std::vector<unsigned int> EpollClientLoginSlave::maxClanIdList;
bool EpollClientLoginSlave::maxAccountIdRequested=false;
bool EpollClientLoginSlave::maxCharacterIdRequested=false;
bool EpollClientLoginSlave::maxMonsterIdRequested=false;

unsigned char EpollClientLoginSlave::protocolReplyProtocolNotSupported[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyServerFull[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionNone[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompresssionZlib[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x05/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionXz[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/,0x06/*return code*/};

unsigned char EpollClientLoginSlave::loginIsWrongBufferReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};

unsigned char EpollClientLoginSlave::loginInProgressBuffer[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x06/*return code*/};
unsigned char EpollClientLoginSlave::addCharacterIsWrongBuffer[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/,0x00,0x00,0x00,0x00};
unsigned char EpollClientLoginSlave::addCharacterReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x05/*reply size*/,0x00/*temp return code*/};
unsigned char EpollClientLoginSlave::removeCharacterReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};
char EpollClientLoginSlave::baseDatapackSum[];
char EpollClientLoginSlave::loginGood[];
unsigned int EpollClientLoginSlave::loginGoodSize=0;

char EpollClientLoginSlave::serverServerList[];
unsigned int EpollClientLoginSlave::serverServerListSize=0;
char EpollClientLoginSlave::serverServerListComputedMessage[];
unsigned int EpollClientLoginSlave::serverServerListComputedMessageSize=0;
unsigned int EpollClientLoginSlave::serverServerListCurrentPlayerSize=0;
char EpollClientLoginSlave::serverLogicalGroupList[];
unsigned int EpollClientLoginSlave::serverLogicalGroupListSize=0;
char EpollClientLoginSlave::serverLogicalGroupAndServerList[];
unsigned int EpollClientLoginSlave::serverLogicalGroupAndServerListSize=0;
EpollClientLoginSlave::ProxyMode EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Reconnect;

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

EpollPostgresql EpollClientLoginSlave::databaseBaseLogin;
EpollPostgresql EpollClientLoginSlave::databaseBaseCommon;

char EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[1024];
unsigned int EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize=0;
