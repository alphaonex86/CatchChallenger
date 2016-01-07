#include <iostream>
#include <vector>
#include <unordered_set>
#include <openssl/sha.h>
#include <time.h>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/cpp11addition.h"

using namespace CatchChallenger;

std::vector<std::string> listFolder(const std::string& folder,const std::string& suffix)
{
    std::vector<std::string> returnList;
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=FacilityLibGeneral::listFolderNotRecursive(folder+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::Dir)
        {
            const std::vector<std::string> &newList=FacilityLibGeneral::listFolder(folder,suffix+fileInfo.name+"/");//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::File)
            returnList.push_back(suffix+fileInfo.name);
    }
    return returnList;
}

std::vector<std::string> listFolderWithExclude(const std::string& folder,const std::string &exclude,const std::string& suffix)
{
    std::vector<std::string> returnList;
    if(suffix==exclude)
        return returnList;
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=FacilityLibGeneral::listFolderNotRecursive(folder+suffix);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::Dir)
        {
            const std::vector<std::string> &newList=FacilityLibGeneral::listFolderWithExclude(folder,exclude,suffix+fileInfo.name+"/");//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::File)
            returnList.push_back(suffix+fileInfo.name);
    }
    return returnList;
}

std::unordered_map<std::string,int> datapack_file_list(const std::string &path, const std::string &exclude, const bool withHash)
{
    std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
    std::unordered_map<std::string,int> filesList;

    std::vector<std::string> returnList;
    if(exclude.empty())
        returnList=FacilityLibGeneral::listFolder(path);
    else
        returnList=FacilityLibGeneral::listFolderWithExclude(path,exclude);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef _WIN32
        std::string fileName=returnList.at(index);
        #else
        const std::string &fileName=returnList.at(index);
        #endif
        if(regex_search(fileName,datapack_rightFileName))
        {
            {
                if(withHash)
                {
                    struct stat buf;
                    if(stat((path+returnList.at(index)).c_str(),&buf)==0)
                    {
                        if(buf.st_size<=CATCHCHALLENGER_MAX_FILE_SIZE)
                        {
                        }
                        else
                            std::cerr << "datapack_file_list file too big failed on " +path+returnList.at(index)+ ":"+std::to_string(buf.st_size) << std::endl;
                    }
                    else
                        std::cerr << "datapack_file_list stat failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                }
                filesList[fileName]=0;
            }
        }
        index++;
    }
    return filesList;
}

int main()
{
    std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
    //do the base
    {
        SHA256_CTX hashBase;
        if(SHA224_Init(&hashBase)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,int> &pair=datapack_file_list("./","map/main/",false);
        if(pair.size()==0)
        {
            std::cout << "datapack_file_list(\"./\",\"map/main/\",false) empty (abort)" << std::endl;
            abort();
        }
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        const std::regex mainDatapackBaseFilter("^map[/\\\\]main[/\\\\]");
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            if(regex_search(datapack_file_temp.at(index),datapack_rightFileName))
            {
                FILE *filedesc = fopen((datapack_file_temp.at(index)).c_str(), "rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                     //switch the data to correct hash or drop it
                    if(regex_search(datapack_file_temp.at(index),mainDatapackBaseFilter))
                    {}
                    else
                    {
                        SHA224_Update(&hashBase,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            else
                std::cerr << "File excluded because don't match the regex (1): " << datapack_file_temp.at(index) << std::endl;
            index++;
        }
        std::vector<char> datapackHashBase;
        datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(datapackHashBase.data()),&hashBase);

        std::cout << binarytoHexa(datapackHashBase) << " hash for datapack loaded base" << std::endl;
    }
    /// \todo check if big file is compressible under 1MB

    //do the main
    std::vector<FacilityLibGeneral::InodeDescriptor> entryListMain=FacilityLibGeneral::listFolderNotRecursive("map/main/",FacilityLibGeneral::ListFolder::Dirs);
    unsigned int indexMain=0;
    while(indexMain<entryListMain.size())
    {
        const std::string &mainDatapackFolder="map/main/"+entryListMain.at(indexMain).name+"/";
        SHA256_CTX hashMain;
        if(SHA224_Init(&hashMain)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,int> &pair=datapack_file_list(mainDatapackFolder,"sub/",false);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            auto i=pair.begin();
            while(i!=pair.cend())
            {
                if(i->first.find("sub/")!=std::string::npos)
                {
                    std::cerr << "sub/ found into: " << i->first << " (abort)" << std::endl;
                    abort();
                }
                ++i;
            }
        }
        #endif
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        const std::regex mainDatapackFolderFilter("^sub[/\\\\]");
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            if(regex_search(datapack_file_temp.at(index),datapack_rightFileName))
            {
                FILE *filedesc = fopen((mainDatapackFolder+datapack_file_temp.at(index)).c_str(), "rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    //switch the data to correct hash or drop it
                    if(regex_search(datapack_file_temp.at(index),mainDatapackFolderFilter))
                    {
                    }
                    else
                    {
                        SHA224_Update(&hashMain,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            else
                std::cerr << "File excluded because don't match the regex (2): " << datapack_file_temp.at(index) << std::endl;
            index++;
        }
        std::vector<char> datapackHashMain;
        datapackHashMain.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(datapackHashMain.data()),&hashMain);

        std::cout << binarytoHexa(datapackHashMain) << " hash for datapack loaded main: " << mainDatapackFolder << std::endl;

        //do the sub
        std::vector<FacilityLibGeneral::InodeDescriptor> entryListSub=FacilityLibGeneral::listFolderNotRecursive(mainDatapackFolder+"sub/",FacilityLibGeneral::ListFolder::Dirs);
        unsigned int indexSub=0;
        while(indexSub<entryListSub.size())
        {
            const std::string &subDatapackFolder=mainDatapackFolder+"sub/"+entryListSub.at(indexSub).name+"/";
            SHA256_CTX hashSub;
            if(SHA224_Init(&hashSub)!=1)
            {
                std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                abort();
            }
            const std::unordered_map<std::string,int> &pair=datapack_file_list(subDatapackFolder,"",false);
            std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
            std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
            unsigned int index=0;
            while(index<datapack_file_temp.size()) {
                if(regex_search(datapack_file_temp.at(index),datapack_rightFileName))
                {
                    FILE *filedesc = fopen((subDatapackFolder+datapack_file_temp.at(index)).c_str(), "rb");
                    if(filedesc!=NULL)
                    {
                        //read and load the file
                        const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                        SHA224_Update(&hashSub,data.data(),data.size());
                    }
                    else
                    {
                        std::cerr << "Stop now! Unable to open the file " << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                        abort();
                    }
                }
                else
                    std::cerr << "File excluded because don't match the regex (3): " << datapack_file_temp.at(index) << std::endl;
                index++;
            }
            std::vector<char> datapackHashSub;
            datapackHashSub.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
            SHA224_Final(reinterpret_cast<unsigned char *>(datapackHashSub.data()),&hashSub);

            std::cout << binarytoHexa(datapackHashSub) << " hash for datapack loaded sub: " << subDatapackFolder << std::endl;

            indexSub++;
        }

        indexMain++;
    }

    return 0;
}
