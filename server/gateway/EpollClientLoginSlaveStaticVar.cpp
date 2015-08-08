#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

std::vector<EpollClientLoginSlave *> EpollClientLoginSlave::client_list;

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

quint8 EpollClientLoginSlave::tempDatapackListReplySize=0;
QByteArray EpollClientLoginSlave::tempDatapackListReplyArray;
quint8 EpollClientLoginSlave::tempDatapackListReply=0;
int EpollClientLoginSlave::tempDatapackListReplyTestCount=0;
QByteArray EpollClientLoginSlave::rawFilesBuffer;
QByteArray EpollClientLoginSlave::compressedFilesBuffer;
int EpollClientLoginSlave::rawFilesBufferCount=0;
int EpollClientLoginSlave::compressedFilesBufferCount=0;

QString EpollClientLoginSlave::text_dotslash=QStringLiteral(";");
QString EpollClientLoginSlave::text_antislash=QStringLiteral("\\");
QString EpollClientLoginSlave::text_double_slash=QStringLiteral("//");
QString EpollClientLoginSlave::text_slash=QStringLiteral("/");

QRegularExpression EpollClientLoginSlave::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));
QRegularExpression EpollClientLoginSlave::datapack_rightFileName = QRegularExpression(DATAPACK_FILE_REGEX);
