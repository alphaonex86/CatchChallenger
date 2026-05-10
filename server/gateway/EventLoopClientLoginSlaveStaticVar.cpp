#include "EventLoopClientLoginSlave.hpp"
#include "../../general/base/ProtocolVersion.hpp"

#include <iostream>

using namespace CatchChallenger;

std::vector<EventLoopClientLoginSlave *> EventLoopClientLoginSlave::client_list;

const unsigned char EventLoopClientLoginSlave::protocolHeaderToMatch[] = PROTOCOL_HEADER_LOGIN;

uint8_t EventLoopClientLoginSlave::tempDatapackListReplySize=0;
std::vector<char> EventLoopClientLoginSlave::tempDatapackListReplyArray;
uint8_t EventLoopClientLoginSlave::tempDatapackListReply=0;
unsigned int EventLoopClientLoginSlave::tempDatapackListReplyTestCount=0;
std::vector<char> EventLoopClientLoginSlave::rawFilesBuffer;
std::vector<char> EventLoopClientLoginSlave::compressedFilesBuffer;
unsigned int EventLoopClientLoginSlave::rawFilesBufferCount=0;
unsigned int EventLoopClientLoginSlave::compressedFilesBufferCount=0;

std::regex EventLoopClientLoginSlave::fileNameStartStringRegex=std::regex("^[a-zA-Z]:/",std::regex_constants::optimize);
std::regex EventLoopClientLoginSlave::datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX,std::regex_constants::optimize);
std::unordered_set<std::string> EventLoopClientLoginSlave::compressedExtension;

EventLoopClientLoginSlave::DatapackData EventLoopClientLoginSlave::datapack_file_base;
std::unordered_map<std::string,EventLoopClientLoginSlave::DatapackData> EventLoopClientLoginSlave::datapack_file_main;
std::unordered_map<std::string,std::unordered_map<std::string,EventLoopClientLoginSlave::DatapackData> > EventLoopClientLoginSlave::datapack_file_sub;
