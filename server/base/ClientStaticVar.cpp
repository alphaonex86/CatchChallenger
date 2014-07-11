#include "Client.h"

using namespace CatchChallenger;

QHash<QString,Client *> Client::playerByPseudo;
QList<Client *> Client::clientBroadCastList;

QString Client::text_chat=QLatin1Literal("chat");
QString Client::text_space=QLatin1Literal(" ");
QString Client::text_system=QLatin1Literal("system");
QString Client::text_system_important=QLatin1Literal("system_important");
QString Client::text_setrights=QLatin1Literal("setrights");
QString Client::text_normal=QLatin1Literal("normal");
QString Client::text_premium=QLatin1Literal("premium");
QString Client::text_gm=QLatin1Literal("gm");
QString Client::text_dev=QLatin1Literal("dev");
QString Client::text_playerlist=QLatin1Literal("playerlist");
QString Client::text_startbold=QLatin1Literal("<b>");
QString Client::text_stopbold=QLatin1Literal("</b>");
QString Client::text_playernumber=QLatin1Literal("playernumber");
QString Client::text_kick=QLatin1Literal("kick");
QString Client::text_Youarealoneontheserver=QLatin1Literal("You are alone on the server!");
QString Client::text_playersconnected=QLatin1Literal(" players connected");
QString Client::text_playersconnectedspace=QLatin1Literal("players connected ");
QString Client::text_havebeenkickedby=QLatin1Literal(" have been kicked by ");
QString Client::text_unknowcommand=QLatin1Literal("unknow command: %1, text: %2");
QString Client::text_commandnotunderstand=QLatin1Literal("command not understand");
QString Client::text_command=QLatin1Literal("command: ");
QString Client::text_commaspace=QLatin1Literal(", ");
QString Client::text_unabletofoundtheconnectedplayertokick=QLatin1Literal("unable to found the connected player to kick");
QString Client::text_unabletofoundthisrightslevel=QLatin1Literal("unable to found this rights level: ");

unsigned char Client::protocolReplyProtocolNotSupported[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x02/*return code*/};
unsigned char Client::protocolReplyServerFull[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x03/*return code*/};
unsigned char Client::protocolReplyCompressionNone[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x04/*return code*/};
unsigned char Client::protocolReplyCompresssionZlib[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x05/*return code*/};
unsigned char Client::protocolReplyCompressionXz[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01+CATCHCHALLENGER_TOKENSIZE/*reply size*/,0x06/*return code*/};

unsigned char Client::loginInProgressBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x06/*return code*/};
unsigned char Client::loginIsWrongBuffer[]={0xC1/*reply server to client*/,0x00/*the init reply query number*/,0x01/*reply size*/,0x00/*temp return code*/};

const unsigned char Client::protocolHeaderToMatch[] = PROTOCOL_HEADER;

QList<int> Client::generalChatDrop;
int Client::generalChatDropTotalCache=0;
int Client::generalChatDropNewValue=0;
QList<int> Client::clanChatDrop;
int Client::clanChatDropTotalCache=0;
int Client::clanChatDropNewValue=0;
QList<int> Client::privateChatDrop;
int Client::privateChatDropTotalCache=0;
int Client::privateChatDropNewValue=0;
QList<quint16> Client::marketObjectIdList;

QList<void *> Client::paramToPassToCallBack;
QList<quint8> Client::selectCharacterQueryId;

QList<quint16> Client::simplifiedIdList;
quint8 Client::tempDatapackListReplySize=0;
quint8 Client::tempDatapackListReply=0;
QByteArray Client::tempDatapackListReplyArray;
QByteArray Client::rawFiles;
QByteArray Client::compressedFiles;
int Client::tempDatapackListReplyTestCount;
int Client::rawFilesCount;
int Client::compressedFilesCount;
QSet<QString> Client::compressedExtension;
QSet<QString> Client::extensionAllowed;
quint64 Client::datapack_list_cache_timestamp;
QHash<QString,quint32> Client::datapack_file_list_cache;
QRegularExpression Client::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));
QString Client::single_quote=QLatin1Literal("'");
QString Client::antislash_single_quote=QLatin1Literal("\\'");

QString Client::text_dotslash=QLatin1Literal("./");
QString Client::text_slash=QLatin1Literal("/");
QString Client::text_double_slash=QLatin1Literal("//");
QString Client::text_antislash=QLatin1Literal("\\");
QString Client::text_dotcomma=QLatin1Literal(";");
QString Client::text_top=QLatin1Literal("top");
QString Client::text_bottom=QLatin1Literal("bottom");
QString Client::text_left=QLatin1Literal("left");
QString Client::text_right=QLatin1Literal("right");
QString Client::text_dottmx=QLatin1Literal(".tmx");
QString Client::text_unknown=QLatin1Literal("unknown");
QString Client::text_female=QLatin1Literal("female");
QString Client::text_male=QLatin1Literal("male");
QString Client::text_warehouse=QLatin1Literal("warehouse");
QString Client::text_wear=QLatin1Literal("wear");
QString Client::text_market=QLatin1Literal("market");

QRegularExpression Client::commandRegExp=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)?$"));
QRegularExpression Client::commandRegExpWithArgs=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)$"));
QRegularExpression Client::isolateTheMainCommand=QRegularExpression(QLatin1String("^ (.*)$"));

QString Client::text_server_full=QLatin1String("Server full");
QString Client::text_slashpmspace=QLatin1String("/pm ");
QString Client::text_regexresult1=QLatin1String("\\1");
QString Client::text_regexresult2=QLatin1String("\\2");
QString Client::text_send_command_slash=QLatin1String("send command: /");
QString Client::text_trade=QLatin1String("trade");
QString Client::text_battle=QLatin1String("battle");
QString Client::text_give=QLatin1String("give");
QString Client::text_setevent=QLatin1String("setevent");
QString Client::text_take=QLatin1String("take");
QString Client::text_tp=QLatin1String("tp");
QString Client::text_stop=QLatin1String("stop");
QString Client::text_restart=QLatin1String("restart");
QString Client::text_unknown_send_command_slash=QLatin1String("unknown send command: /");
QString Client::text_commands_seem_not_right=QLatin1String("commands seem not right: /");

Direction Client::temp_direction;
QHash<quint32,Client *> Client::playerById;
QHash<QString,QList<Client *> > Client::captureCity;
QHash<QString,CaptureCityValidated> Client::captureCityValidatedList;
QHash<quint32,Clan *> Client::clanList;

QString Client::text_0=QLatin1Literal("0");
QString Client::text_1=QLatin1Literal("1");
QString Client::text_false=QLatin1Literal("false");
QString Client::text_true=QLatin1Literal("true");
QString Client::text_to=QLatin1Literal("to");
