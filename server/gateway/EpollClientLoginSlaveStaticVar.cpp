#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

std::vector<EpollClientLoginSlave *> EpollClientLoginSlave::client_list;

const unsigned char EpollClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

uint8_t EpollClientLoginSlave::tempDatapackListReplySize=0;
QByteArray EpollClientLoginSlave::tempDatapackListReplyArray;
uint8_t EpollClientLoginSlave::tempDatapackListReply=0;
int EpollClientLoginSlave::tempDatapackListReplyTestCount=0;
QByteArray EpollClientLoginSlave::rawFilesBuffer;
QByteArray EpollClientLoginSlave::compressedFilesBuffer;
int EpollClientLoginSlave::rawFilesBufferCount=0;
int EpollClientLoginSlave::compressedFilesBufferCount=0;

std::regex EpollClientLoginSlave::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/");
std::regex EpollClientLoginSlave::datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> EpollClientLoginSlave::compressedExtension;

EpollClientLoginSlave::DatapackData EpollClientLoginSlave::datapack_file_base;
std::unordered_map<std::string,EpollClientLoginSlave::DatapackData> EpollClientLoginSlave::datapack_file_main;
std::unordered_map<std::string,std::unordered_map<std::string,EpollClientLoginSlave::DatapackData> > EpollClientLoginSlave::datapack_file_sub;
