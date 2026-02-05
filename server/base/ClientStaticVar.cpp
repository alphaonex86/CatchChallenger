#include "Client.hpp"
#include "../../general/base/ProtocolVersion.hpp"

using namespace CatchChallenger;

//std::unordered_map<std::string,SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> Client::playerByPseudo;

unsigned char Client::protocolReplyProtocolNotSupported[]={0x7F/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size, little endian*/,0x02/*return code*/};
unsigned char Client::protocolReplyServerFull[]={0x7F/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size, little endian*/,0x03/*return code*/};
unsigned char Client::protocolReplyCompressionNone[]={0x7F/*reply server to client*/,0x00/*the init reply query number*/,0x01
                                                      #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                                      +TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/
                                                      #endif
                                                      ,0x00,0x00,0x00/*little endian*/
                                                      ,0x04/*compression: none*/};
#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
unsigned char Client::protocolReplyCompresssionZstandard[]={0x7F/*reply server to client*/,0x00/*the init reply query number*/,0x01
                                                       #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                                       +TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/
                                                       #endif
                                                       ,0x00,0x00,0x00/*little endian*/
                                                       ,0x08/*compression: zstd*/};
#endif

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
unsigned char Client::loginIsWrongBuffer[]={0x7F/*reply server to client*/,0x00/*the init reply query number*/,0x01,0x00,0x00,0x00/*reply size, little endian*/,0x00/*temp return code*/};
//std::vector<Client *> Client::stat_client_list;
unsigned char Client::private_token_statclient[TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
#endif

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
const unsigned char Client::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

unsigned char *Client::protocolReplyCharacterList=NULL;
uint16_t Client::protocolReplyCharacterListSize=0;
unsigned char * Client::protocolMessageLogicalGroupAndServerList=NULL;
uint16_t Client::protocolMessageLogicalGroupAndServerListSize=0;
uint16_t Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber=0;
#else
const unsigned char Client::protocolHeaderToMatch[] = PROTOCOL_HEADER_GAMESERVER;
#endif

DdosBuffer<uint16_t,3> Client::generalChatDrop;
DdosBuffer<uint16_t,3> Client::clanChatDrop;
DdosBuffer<uint16_t,3> Client::privateChatDrop;

std::vector<uint8_t> Client::selectCharacterQueryId;
//std::vector<uint16_t> Client::simplifiedIdList;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
uint8_t Client::tempDatapackListReplySize=0;
uint8_t Client::tempDatapackListReply=0;
std::vector<char> Client::tempDatapackListReplyArray;
unsigned int Client::tempDatapackListReplyTestCount;
uint64_t Client::datapack_list_cache_timestamp_base;
uint64_t Client::datapack_list_cache_timestamp_main;
uint64_t Client::datapack_list_cache_timestamp_sub;
//do into BaseServerMasterSendDatapack::datapack_file_hash_cache_base std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_hash_cache_base;
std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_hash_cache_main;
std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_hash_cache_sub;
#endif

std::regex Client::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/");

std::unordered_map<ZONE_TYPE,std::vector<PLAYER_INDEX_FOR_CONNECTED> > Client::captureCity;
std::unordered_map<ZONE_TYPE,CaptureCityValidated> Client::captureCityValidatedList;
std::unordered_map<uint32_t,uint64_t> Client::characterCreationDateList;

unsigned char *Client::characterIsRightFinalStepHeader=NULL;
uint32_t Client::characterIsRightFinalStepHeaderSize=0;
bool Client::timeRangeEventNew=false;
uint64_t Client::timeRangeEventTimestamps=0;

#ifdef CATCHCHALLENGER_EXTRA_CHECK
Player_private_and_public_informations *ClientBase::public_and_private_informations_solo=NULL;
#endif

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
std::vector<Client::TokenAuth> Client::tokenAuthList;
#endif

