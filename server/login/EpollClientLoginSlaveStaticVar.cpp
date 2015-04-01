#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

char EpollClientLoginSlave::private_token[TOKEN_SIZE];
LoginLinkToMaster *EpollClientLoginSlave::linkToMaster=NULL;
QList<unsigned int> EpollClientLoginSlave::maxAccountIdList;
QList<unsigned int> EpollClientLoginSlave::maxCharacterIdList;
QList<unsigned int> EpollClientLoginSlave::maxClanIdList;
bool EpollClientLoginSlave::maxAccountIdRequested=false;
bool EpollClientLoginSlave::maxCharacterIdRequested=false;
bool EpollClientLoginSlave::maxMonsterIdRequested=false;
char EpollClientLoginSlave::maxAccountIdRequest[]={0x11/*reply server to client*/,0x01/*query id*/,0x00/*the init reply query number*/};
char EpollClientLoginSlave::maxCharacterIdRequest[]={0x11/*reply server to client*/,0x02/*query id*/,0x00/*the init reply query number*/};
char EpollClientLoginSlave::maxMonsterIdRequest[]={0x11/*reply server to client*/,0x03/*query id*/,0x00/*the init reply query number*/};
char EpollClientLoginSlave::selectCharaterRequest[]={0x02/*reply server to client*/,0x05/*query id*/,0x00/*the init reply query number*/};

unsigned char EpollClientLoginSlave::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyServerFull[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x04/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x05/*return code*/};
unsigned char EpollClientLoginSlave::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x06/*return code*/};

unsigned char EpollClientLoginSlave::loginInProgressBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x06/*return code*/};
unsigned char EpollClientLoginSlave::loginIsWrongBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};
char EpollClientLoginSlave::baseDatapackSum[];
char EpollClientLoginSlave::loginGood[];
char EpollClientLoginSlave::loginGoodSize=0;

char EpollClientLoginSlave::serverPartialServerList[];
int EpollClientLoginSlave::serverPartialServerListSize=0;
char EpollClientLoginSlave::serverServerList[];
int EpollClientLoginSlave::serverServerListSize=0;
char EpollClientLoginSlave::serverLogicalGroupList[];
int EpollClientLoginSlave::serverLogicalGroupListSize=0;
char EpollClientLoginSlave::serverLogicalGroupAndServerList[];
int EpollClientLoginSlave::serverLogicalGroupAndServerListSize=0;
EpollClientLoginSlave::ProxyMode EpollClientLoginSlave::proxyMode=EpollClientLoginSlave::ProxyMode::Direct;

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER;

bool EpollClientLoginSlave::automatic_account_creation=false;
unsigned int EpollClientLoginSlave::character_delete_time=3600*24;
QString EpollClientLoginSlave::httpDatapackMirror;
unsigned int EpollClientLoginSlave::min_character=0;
unsigned int EpollClientLoginSlave::max_character=3;
unsigned int EpollClientLoginSlave::max_pseudo_size=20;
quint8 EpollClientLoginSlave::max_player_monsters=5;// \warning Never put number greater than 10 here
quint16 EpollClientLoginSlave::max_warehouse_player_monsters=30;
quint8 EpollClientLoginSlave::max_player_items=30;
quint16 EpollClientLoginSlave::max_warehouse_player_items=100;
EpollPostgresql EpollClientLoginSlave::databaseBaseLogin;
EpollPostgresql EpollClientLoginSlave::databaseBaseCommon;

char EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup[1024];
int EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize=0;
