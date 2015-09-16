#include "Client.h"

using namespace CatchChallenger;

std::unordered_map<std::string,Client *> Client::playerByPseudo;
std::vector<Client *> Client::clientBroadCastList;

const std::string Client::text_chat="chat";
const std::string Client::text_space=" ";
const std::string Client::text_system="system";
const std::string Client::text_system_important="system_important";
const std::string Client::text_setrights="setrights";
const std::string Client::text_normal="normal";
const std::string Client::text_premium="premium";
const std::string Client::text_gm="gm";
const std::string Client::text_dev="dev";
const std::string Client::text_playerlist="playerlist";
const std::string Client::text_startbold="<b>";
const std::string Client::text_stopbold="</b>";
const std::string Client::text_playernumber="playernumber";
const std::string Client::text_kick="kick";
const std::string Client::text_Youarealoneontheserver="You are alone on the server!";
const std::string Client::text_playersconnected=" players connected";
const std::string Client::text_playersconnectedspace="players connected ";
const std::string Client::text_havebeenkickedby=" have been kicked by ";
const std::string Client::text_unknowcommand="unknow command: %1, text: %2";
const std::string Client::text_commandnotunderstand="command not understand";
const std::string Client::text_command="command: ";
const std::string Client::text_commaspace=", ";
const std::string Client::text_unabletofoundtheconnectedplayertokick="unable to found the connected player to kick";
const std::string Client::text_unabletofoundthisrightslevel="unable to found this rights level: ";

unsigned char Client::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char Client::protocolReplyServerFull[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
unsigned char Client::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01
                                                      #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                                      +TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/
                                                      #endif
                                                      ,0x04/*return code*/};
unsigned char Client::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01
                                                       #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                                       +TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/
                                                       #endif
                                                       ,0x05/*return code*/};
unsigned char Client::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01
                                                    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                                    +TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT/*reply size*/
                                                    #endif
                                                    ,0x06/*return code*/};

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
unsigned char Client::loginIsWrongBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};
#endif

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
const unsigned char Client::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

unsigned char * Client::protocolMessageLogicalGroupAndServerList=NULL;
uint16_t Client::protocolMessageLogicalGroupAndServerListSize=0;
uint16_t Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber=0;
#else
const unsigned char Client::protocolHeaderToMatch[] = PROTOCOL_HEADER_GAMESERVER;
#endif

std::vector<int> Client::generalChatDrop;
int Client::generalChatDropTotalCache=0;
int Client::generalChatDropNewValue=0;
std::vector<int> Client::clanChatDrop;
int Client::clanChatDropTotalCache=0;
int Client::clanChatDropNewValue=0;
std::vector<int> Client::privateChatDrop;
int Client::privateChatDropTotalCache=0;
int Client::privateChatDropNewValue=0;
std::vector<uint16_t> Client::marketObjectIdList;
#ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
std::queue<Client::PlantInWaiting> Client::plant_list_in_waiting;
#endif
uint8_t Client::indexOfItemOnMap;
#ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
uint8_t Client::indexOfDirtOnMap;//index of plant on map, ordened by map and x,y ordened into the xml file, less bandwith than send map,x,y
#endif

std::vector<uint8_t> Client::selectCharacterQueryId;

std::vector<uint16_t> Client::simplifiedIdList;
uint8_t Client::tempDatapackListReplySize=0;
uint8_t Client::tempDatapackListReply=0;
QByteArray Client::tempDatapackListReplyArray;
unsigned int Client::tempDatapackListReplyTestCount;
quint64 Client::datapack_list_cache_timestamp_base;
quint64 Client::datapack_list_cache_timestamp_main;
quint64 Client::datapack_list_cache_timestamp_sub;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_base;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_main;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_sub;
std::regex Client::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/");
std::string Client::single_quote="'";
std::string Client::antislash_single_quote="\\'";

const std::string Client::text_dotslash="./";
const std::string Client::text_slash="/";
const std::string Client::text_double_slash="//";
const std::string Client::text_antislash="\\";
const std::string Client::text_dotcomma=";";
const std::string Client::text_top="top";
const std::string Client::text_bottom="bottom";
const std::string Client::text_left="left";
const std::string Client::text_right="right";
const std::string Client::text_dottmx=".tmx";
const std::string Client::text_unknown="unknown";
const std::string Client::text_female="female";
const std::string Client::text_male="male";
const std::string Client::text_warehouse="warehouse";
const std::string Client::text_wear="wear";
const std::string Client::text_market="market";

const std::regex Client::commandRegExp=std::regex("^/([a-z]+)( [^ ].*)?$");
const std::regex Client::commandRegExpWithArgs=std::regex("^/([a-z]+)( [^ ].*)$");
const std::regex Client::isolateTheMainCommand=std::regex("^ (.*)$");

const std::string Client::text_server_full="Server full";
const std::string Client::text_slashpmspace="/pm ";
const std::string Client::text_regexresult1="\\1";
const std::string Client::text_regexresult2="\\2";
const std::string Client::text_send_command_slash="send command: /";
const std::string Client::text_trade="trade";
const std::string Client::text_battle="battle";
const std::string Client::text_give="give";
const std::string Client::text_setevent="setevent";
const std::string Client::text_take="take";
const std::string Client::text_tp="tp";
const std::string Client::text_stop="stop";
const std::string Client::text_restart="restart";
const std::string Client::text_unknown_send_command_slash="unknown send command: /";
const std::string Client::text_commands_seem_not_right="commands seem not right: /";

Direction Client::temp_direction;
std::unordered_map<uint32_t,Client *> Client::playerById;
std::unordered_map<std::string,std::vector<Client *> > Client::captureCity;
std::unordered_map<std::string,CaptureCityValidated> Client::captureCityValidatedList;
std::unordered_map<uint32_t,Clan *> Client::clanList;

const std::string Client::text_0="0";
const std::string Client::text_1="1";
const std::string Client::text_false="false";
const std::string Client::text_true="true";
const std::string Client::text_to="to";

unsigned char *Client::characterIsRightFinalStepHeader=NULL;
uint32_t Client::characterIsRightFinalStepHeaderSize=0;

#ifdef CATCHCHALLENGER_EXTRA_CHECK
Player_private_and_public_informations *ClientBase::public_and_private_informations_solo=NULL;
#endif

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
std::vector<Client::TokenAuth> Client::tokenAuthList;
#endif

