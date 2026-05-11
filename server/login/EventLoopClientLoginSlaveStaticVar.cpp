#include "EventLoopClientLoginSlave.hpp"
#include "../../general/base/ProtocolVersion.hpp"
// Full DB-backend type to instantiate the static EventLoopDb members.
#if defined(CATCHCHALLENGER_DB_POSTGRESQL)
#include "../cli/db/EventLoopPostgresql.hpp"
#elif defined(CATCHCHALLENGER_DB_MYSQL)
#include "../cli/db/EventLoopMySQL.hpp"
#endif

#include <iostream>

using namespace CatchChallenger;

std::vector<EventLoopClientLoginSlave *> EventLoopClientLoginSlave::client_list;
std::vector<EventLoopClientLoginSlave *> EventLoopClientLoginSlave::stat_client_list;
std::vector<unsigned int> EventLoopClientLoginSlave::maxAccountIdList;
bool EventLoopClientLoginSlave::maxAccountIdRequested=false;

unsigned char EventLoopClientLoginSlave::protocolReplyProtocolNotSupported[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x02/*return code*/};
unsigned char EventLoopClientLoginSlave::protocolReplyServerFull[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x03/*return code*/};
unsigned char EventLoopClientLoginSlave::protocolReplyCompressionNone[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x04/*return code*/};
#ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
unsigned char EventLoopClientLoginSlave::protocolReplyCompresssionZstandard[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,0x00,0x00,0x00/*reply size*/,0x08/*return code*/};
#endif

unsigned char EventLoopClientLoginSlave::loginIsWrongBufferReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x00/*temp return code*/};

unsigned char EventLoopClientLoginSlave::loginInProgressBuffer[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x06/*return code*/};
unsigned char EventLoopClientLoginSlave::addCharacterIsWrongBuffer[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x00/*temp return code*/,0x00,0x00,0x00,0x00/*Fixed size to drop dynamic size overhead*/};
unsigned char EventLoopClientLoginSlave::addCharacterReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x00/*temp return code*/,0x00,0x00,0x00,0x00};
unsigned char EventLoopClientLoginSlave::removeCharacterReply[]={CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size*/,0x00/*temp return code*/};
char EventLoopClientLoginSlave::baseDatapackSum[];
char EventLoopClientLoginSlave::loginGood[];
unsigned int EventLoopClientLoginSlave::loginGoodSize=0;

char EventLoopClientLoginSlave::serverServerList[];
unsigned int EventLoopClientLoginSlave::serverServerListSize=0;
char EventLoopClientLoginSlave::serverServerListComputedMessage[];
unsigned int EventLoopClientLoginSlave::serverServerListComputedMessageSize=0;
unsigned int EventLoopClientLoginSlave::serverServerListCurrentPlayerSize=0;
char EventLoopClientLoginSlave::serverLogicalGroupList[];
unsigned int EventLoopClientLoginSlave::serverLogicalGroupListSize=0;
char EventLoopClientLoginSlave::serverLogicalGroupAndServerList[];
unsigned int EventLoopClientLoginSlave::serverLogicalGroupAndServerListSize=0;
EventLoopClientLoginSlave::ProxyMode EventLoopClientLoginSlave::proxyMode=EventLoopClientLoginSlave::ProxyMode::Reconnect;

const unsigned char EventLoopClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

EventLoopDb EventLoopClientLoginSlave::databaseBaseLogin;
EventLoopDb EventLoopClientLoginSlave::databaseBaseCommon;

char EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroup[1024];
unsigned int EventLoopClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize=0;
