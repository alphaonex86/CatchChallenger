#include "Client.h"

using namespace CatchChallenger;

QHash<QString,Client *> Client::playerByPseudo;
QList<Client *> Client::clientBroadCastList;

const QString Client::text_chat=QLatin1Literal("chat");
const QString Client::text_space=QLatin1Literal(" ");
const QString Client::text_system=QLatin1Literal("system");
const QString Client::text_system_important=QLatin1Literal("system_important");
const QString Client::text_setrights=QLatin1Literal("setrights");
const QString Client::text_normal=QLatin1Literal("normal");
const QString Client::text_premium=QLatin1Literal("premium");
const QString Client::text_gm=QLatin1Literal("gm");
const QString Client::text_dev=QLatin1Literal("dev");
const QString Client::text_playerlist=QLatin1Literal("playerlist");
const QString Client::text_startbold=QLatin1Literal("<b>");
const QString Client::text_stopbold=QLatin1Literal("</b>");
const QString Client::text_playernumber=QLatin1Literal("playernumber");
const QString Client::text_kick=QLatin1Literal("kick");
const QString Client::text_Youarealoneontheserver=QLatin1Literal("You are alone on the server!");
const QString Client::text_playersconnected=QLatin1Literal(" players connected");
const QString Client::text_playersconnectedspace=QLatin1Literal("players connected ");
const QString Client::text_havebeenkickedby=QLatin1Literal(" have been kicked by ");
const QString Client::text_unknowcommand=QLatin1Literal("unknow command: %1, text: %2");
const QString Client::text_commandnotunderstand=QLatin1Literal("command not understand");
const QString Client::text_command=QLatin1Literal("command: ");
const QString Client::text_commaspace=QLatin1Literal(", ");
const QString Client::text_unabletofoundtheconnectedplayertokick=QLatin1Literal("unable to found the connected player to kick");
const QString Client::text_unabletofoundthisrightslevel=QLatin1Literal("unable to found this rights level: ");

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

const QString Client::text_dotslash=QLatin1Literal("./");
const QString Client::text_slash=QLatin1Literal("/");
const QString Client::text_double_slash=QLatin1Literal("//");
const QString Client::text_antislash=QLatin1Literal("\\");
const QString Client::text_dotcomma=QLatin1Literal(";");
const QString Client::text_top=QLatin1Literal("top");
const QString Client::text_bottom=QLatin1Literal("bottom");
const QString Client::text_left=QLatin1Literal("left");
const QString Client::text_right=QLatin1Literal("right");
const QString Client::text_dottmx=QLatin1Literal(".tmx");
const QString Client::text_unknown=QLatin1Literal("unknown");
const QString Client::text_female=QLatin1Literal("female");
const QString Client::text_male=QLatin1Literal("male");
const QString Client::text_warehouse=QLatin1Literal("warehouse");
const QString Client::text_wear=QLatin1Literal("wear");
const QString Client::text_market=QLatin1Literal("market");

const QRegularExpression Client::commandRegExp=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)?$"));
const QRegularExpression Client::commandRegExpWithArgs=QRegularExpression(QLatin1String("^/([a-z]+)( [^ ].*)$"));
const QRegularExpression Client::isolateTheMainCommand=QRegularExpression(QLatin1String("^ (.*)$"));

const QString Client::text_server_full=QLatin1String("Server full");
const QString Client::text_slashpmspace=QLatin1String("/pm ");
const QString Client::text_regexresult1=QLatin1String("\\1");
const QString Client::text_regexresult2=QLatin1String("\\2");
const QString Client::text_send_command_slash=QLatin1String("send command: /");
const QString Client::text_trade=QLatin1String("trade");
const QString Client::text_battle=QLatin1String("battle");
const QString Client::text_give=QLatin1String("give");
const QString Client::text_setevent=QLatin1String("setevent");
const QString Client::text_take=QLatin1String("take");
const QString Client::text_tp=QLatin1String("tp");
const QString Client::text_stop=QLatin1String("stop");
const QString Client::text_restart=QLatin1String("restart");
const QString Client::text_unknown_send_command_slash=QLatin1String("unknown send command: /");
const QString Client::text_commands_seem_not_right=QLatin1String("commands seem not right: /");

Direction Client::temp_direction;
QHash<quint32,Client *> Client::playerById;
QHash<QString,QList<Client *> > Client::captureCity;
QHash<QString,CaptureCityValidated> Client::captureCityValidatedList;
QHash<quint32,Clan *> Client::clanList;

const QString Client::text_0=QLatin1Literal("0");
const QString Client::text_1=QLatin1Literal("1");
const QString Client::text_false=QLatin1Literal("false");
const QString Client::text_true=QLatin1Literal("true");
const QString Client::text_to=QLatin1Literal("to");
