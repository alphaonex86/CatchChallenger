#include "Client.h"

using namespace CatchChallenger;

std::unordered_map<std::string,Client *> Client::playerByPseudo;
std::vector<Client *> Client::clientBroadCastList;

const std::string Client::text_chat=QLatin1Literal("chat");
const std::string Client::text_space=QLatin1Literal(" ");
const std::string Client::text_system=QLatin1Literal("system");
const std::string Client::text_system_important=QLatin1Literal("system_important");
const std::string Client::text_setrights=QLatin1Literal("setrights");
const std::string Client::text_normal=QLatin1Literal("normal");
const std::string Client::text_premium=QLatin1Literal("premium");
const std::string Client::text_gm=QLatin1Literal("gm");
const std::string Client::text_dev=QLatin1Literal("dev");
const std::string Client::text_playerlist=QLatin1Literal("playerlist");
const std::string Client::text_startbold=QLatin1Literal("<b>");
const std::string Client::text_stopbold=QLatin1Literal("</b>");
const std::string Client::text_playernumber=QLatin1Literal("playernumber");
const std::string Client::text_kick=QLatin1Literal("kick");
const std::string Client::text_Youarealoneontheserver=QLatin1Literal("You are alone on the server!");
const std::string Client::text_playersconnected=QLatin1Literal(" players connected");
const std::string Client::text_playersconnectedspace=QLatin1Literal("players connected ");
const std::string Client::text_havebeenkickedby=QLatin1Literal(" have been kicked by ");
const std::string Client::text_unknowcommand=QLatin1Literal("unknow command: %1, text: %2");
const std::string Client::text_commandnotunderstand=QLatin1Literal("command not understand");
const std::string Client::text_command=QLatin1Literal("command: ");
const std::string Client::text_commaspace=QLatin1Literal(", ");
const std::string Client::text_unabletofoundtheconnectedplayertokick=QLatin1Literal("unable to found the connected player to kick");
const std::string Client::text_unabletofoundthisrightslevel=QLatin1Literal("unable to found this rights level: ");

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
std::vector<Client::PlantInWaiting> Client::plant_list_in_waiting;
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
int Client::tempDatapackListReplyTestCount;
quint64 Client::datapack_list_cache_timestamp_base;
quint64 Client::datapack_list_cache_timestamp_main;
quint64 Client::datapack_list_cache_timestamp_sub;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_base;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_main;
std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_hash_cache_sub;
QRegularExpression Client::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));
std::string Client::single_quote=QLatin1Literal("'");
std::string Client::antislash_single_quote=QLatin1Literal("\\'");

const std::string Client::text_dotslash=QLatin1Literal("./");
const std::string Client::text_slash=QLatin1Literal("/");
const std::string Client::text_double_slash=QLatin1Literal("//");
const std::string Client::text_antislash=QLatin1Literal("\\");
const std::string Client::text_dotcomma=QLatin1Literal(";");
const std::string Client::text_top=QLatin1Literal("top");
const std::string Client::text_bottom=QLatin1Literal("bottom");
const std::string Client::text_left=QLatin1Literal("left");
const std::string Client::text_right=QLatin1Literal("right");
const std::string Client::text_dottmx=QLatin1Literal(".tmx");
const std::string Client::text_unknown=QLatin1Literal("unknown");
const std::string Client::text_female=QLatin1Literal("female");
const std::string Client::text_male=QLatin1Literal("male");
const std::string Client::text_warehouse=QLatin1Literal("warehouse");
const std::string Client::text_wear=QLatin1Literal("wear");
const std::string Client::text_market=QLatin1Literal("market");

const QRegularExpression Client::commandRegExp=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)?$"));
const QRegularExpression Client::commandRegExpWithArgs=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)$"));
const QRegularExpression Client::isolateTheMainCommand=QRegularExpression(QLatin1String("^ (.*)$"));

const std::string Client::text_server_full=QLatin1String("Server full");
const std::string Client::text_slashpmspace=QLatin1String("/pm ");
const std::string Client::text_regexresult1=QLatin1String("\\1");
const std::string Client::text_regexresult2=QLatin1String("\\2");
const std::string Client::text_send_command_slash=QLatin1String("send command: /");
const std::string Client::text_trade=QLatin1String("trade");
const std::string Client::text_battle=QLatin1String("battle");
const std::string Client::text_give=QLatin1String("give");
const std::string Client::text_setevent=QLatin1String("setevent");
const std::string Client::text_take=QLatin1String("take");
const std::string Client::text_tp=QLatin1String("tp");
const std::string Client::text_stop=QLatin1String("stop");
const std::string Client::text_restart=QLatin1String("restart");
const std::string Client::text_unknown_send_command_slash=QLatin1String("unknown send command: /");
const std::string Client::text_commands_seem_not_right=QLatin1String("commands seem not right: /");

Direction Client::temp_direction;
std::unordered_map<uint32_t,Client *> Client::playerById;
std::unordered_map<std::string,std::vector<Client *> > Client::captureCity;
std::unordered_map<std::string,CaptureCityValidated> Client::captureCityValidatedList;
std::unordered_map<uint32_t,Clan *> Client::clanList;

const std::string Client::text_0=QLatin1Literal("0");
const std::string Client::text_1=QLatin1Literal("1");
const std::string Client::text_false=QLatin1Literal("false");
const std::string Client::text_true=QLatin1Literal("true");
const std::string Client::text_to=QLatin1Literal("to");

#ifdef CATCHCHALLENGER_EXTRA_CHECK
Player_private_and_public_informations *ClientBase::public_and_private_informations_solo=NULL;
#endif

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
std::vector<Client::TokenAuth> Client::tokenAuthList;
#endif

