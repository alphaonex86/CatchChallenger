#ifndef CATCHCHALLENGER_FACILITYLIB_GENERAL_H
#define CATCHCHALLENGER_FACILITYLIB_GENERAL_H

#include <string>
#include <vector>

namespace CatchChallenger {
class FacilityLibGeneral
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
    static uint32_t fileSize(FILE * file);
    static std::string getSuffix(const std::string& fileName);
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
