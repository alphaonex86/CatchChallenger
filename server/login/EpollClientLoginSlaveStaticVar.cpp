#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

char EpollClientLoginSlave::private_token[TOKEN_SIZE];
QList<unsigned int> EpollClientLoginSlave::maxAccountIdList;
QList<unsigned int> EpollClientLoginSlave::maxCharacterIdList;
QList<unsigned int> EpollClientLoginSlave::maxClanIdList;
bool EpollClientLoginSlave::maxAccountIdRequested=false;
bool EpollClientLoginSlave::maxCharacterIdRequested=false;
bool EpollClientLoginSlave::maxClanIdRequested=false;
char EpollClientLoginSlave::maxAccountIdRequest[]={0x11/*reply server to client*/,0x00,0x01/*query id*/,0x00/*the init reply query number*/};
char EpollClientLoginSlave::maxCharacterIdRequest[]={0x11/*reply server to client*/,0x00,0x02/*query id*/,0x00/*the init reply query number*/};
char EpollClientLoginSlave::maxClanIdRequest[]={0x11/*reply server to client*/,0x00,0x03/*query id*/,0x00/*the init reply query number*/};

unsigned char EpollClientLoginSlave::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyServerFull[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x05/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x06/*return code*/};

unsigned char EpollClientLoginSlave::loginInProgressBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x06/*return code*/};
unsigned char EpollClientLoginSlave::loginIsWrongBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER;

bool EpollClientLoginSlave::automatic_account_creation;
unsigned int EpollClientLoginSlave::character_delete_time;
QString EpollClientLoginSlave::httpDatapackMirror;
unsigned int EpollClientLoginSlave::min_character;
unsigned int EpollClientLoginSlave::max_character;
unsigned int EpollClientLoginSlave::max_pseudo_size;
EpollPostgresql EpollClientLoginSlave::databaseBaseLogin;
EpollPostgresql EpollClientLoginSlave::databaseBaseCommon;
