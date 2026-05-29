#ifndef CATCHCHALLENGER_FACILITYLIB_GENERAL_H
#define CATCHCHALLENGER_FACILITYLIB_GENERAL_H

#include <string>
#include <vector>
#include <cstdint>
#include "lib.h"

namespace CatchChallenger {
class DLL_PUBLIC FacilityLibGeneral
{
public:
    static unsigned int toUTF8WithHeader(const std::string &text,char * const data);
    static unsigned int toUTF8With16BitsHeader(const std::string &text,char * const data);
    static std::vector<std::string> listFolder(const std::string& folder,const std::string& suffix=std::string());
    static std::vector<std::string> listFolderWithExclude(const std::string& folder,const std::string &exclude,const std::string& suffix=std::string());
    static std::string randomPassword(const std::string& string,const uint8_t& length);
    static std::vector<std::string> skinIdList(const std::string& skinPath);
    static std::string dropPrefixAndSuffixLowerThen33(const std::string &str);
    //static std::string secondsToString(const uint64_t &seconds);
    static bool rmpath(const std::string &dirPath);
    //static std::string timeToString(const uint32_t &time);
    enum ListFolder
    {
        Dirs=1,
        Files=2,
        FilesAndDirs=3
    };
    struct InodeDescriptor
    {
        enum Type
        {
            Dir,
            File
        };
        Type type;
        std::string name;
        std::string absoluteFilePath;

        bool operator < (const InodeDescriptor &b) const
        {
            return absoluteFilePath<b.absoluteFilePath;
        }
    };

    static std::vector<InodeDescriptor> listFolderNotRecursive(const std::string& folder,const ListFolder &type=ListFolder::FilesAndDirs);
    static bool isFile(const std::string& file);
    static bool isDir(const std::string& folder);
    static std::vector<char> readAllFileAndClose(FILE * file);
    // Read an entire file into memory. Default is blocking fopen/fread; if a
    // backend has installed wholeFileRingReader (the io_uring server does at
    // startup) the read is routed through its ring instead, falling back to
    // the blocking path whenever the ring reader declines or fails. ok is set
    // false only when the file cannot be opened at all.
    static std::vector<char> readWholeFile(const std::string &path,bool &ok);
    // Optional injected reader: returns true when it has fully read path into
    // out (readWholeFile then returns out), false to request the blocking
    // fallback. Lets server/cli route file reads through io_uring without
    // general/base depending on liburing. Null on client/select/epoll/poll.
    typedef bool (*WholeFileRingReader)(const std::string &path,std::vector<char> &out);
    static WholeFileRingReader wholeFileRingReader;
    // Write size bytes of data to path, replacing it whole (the FILE_DB save
    // pattern: compose-in-memory then truncate-rewrite). Default is blocking
    // fopen("wb")+fwrite; if wholeFileRingWriter is installed (io_uring server)
    // the write is routed through its ring, falling back to blocking whenever
    // the ring writer declines or fails. Returns true on success.
    static bool writeWholeFile(const std::string &path,const char *data,size_t size);
    // Optional injected writer: returns true when it has fully written the
    // file, false to request the blocking fallback. Same injection model as
    // wholeFileRingReader. Null on client/select/epoll/poll.
    typedef bool (*WholeFileRingWriter)(const std::string &path,const char *data,size_t size);
    static WholeFileRingWriter wholeFileRingWriter;
    static uint32_t fileSize(FILE * file);
    static std::string getSuffix(const std::string& fileName);
    static std::string getSuffixAndValidatePathFromFS(const std::string& fileName);
    static std::string getFolderFromFile(const std::string& fileName);

    static std::string applicationDirPath;
private:
    static std::string text_slash;
    static std::string text_male;
    static std::string text_female;
    static std::string text_unknown;
    static std::string text_clan;
    static std::string text_dotcomma;
};
}

#endif
