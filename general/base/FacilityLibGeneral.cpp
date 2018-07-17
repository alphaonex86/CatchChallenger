#include "FacilityLibGeneral.h"
#include "PortableEndian.h"
#include "GeneralVariable.h"

#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unistd.h>

using namespace CatchChallenger;

std::string FacilityLibGeneral::text_slash="/";
std::string FacilityLibGeneral::text_male="male";
std::string FacilityLibGeneral::text_female="female";
std::string FacilityLibGeneral::text_unknown="unknown";
std::string FacilityLibGeneral::text_clan="clan";
std::string FacilityLibGeneral::text_dotcomma=";";

std::string FacilityLibGeneral::applicationDirPath;

unsigned int FacilityLibGeneral::toUTF8WithHeader(const std::string &text,char * const data)
{
    if(text.empty() || text.size()>255)
        return 0;
    data[0]=static_cast<uint8_t>(text.size());
    memcpy(data+1,text.data(),static_cast<size_t>(text.size()));
    return 1+static_cast<uint8_t>(text.size());
}

unsigned int FacilityLibGeneral::toUTF8With16BitsHeader(const std::string &text,char * const data)
{
    if(text.empty() || text.size()>65535)
        return 0;
    *reinterpret_cast<uint16_t *>(data+0)=(uint16_t)htole16((uint16_t)text.size());
    memcpy(data+2,text.data(),static_cast<size_t>(text.size()));
    return 2+static_cast<uint8_t>(text.size());
}

std::vector<FacilityLibGeneral::InodeDescriptor> FacilityLibGeneral::listFolderNotRecursive(const std::string& folder,const ListFolder &type)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(folder.empty())
    {
        std::cerr << "can't FacilityLibGeneral::listFolderNotRecursive(\"\")" << std::endl;
    }
    {
        const char lastChar=folder.at(folder.size()-1);
        if(lastChar!='/' && lastChar!='\\')
        {
            std::cerr << "can't FacilityLibGeneral::listFolderNotRecursive(\"XXXXX\"), XXXX don't end with /" << std::endl;
        }
    }
    #endif
    std::vector<InodeDescriptor> output;
    DIR *dir;
    struct dirent *ent;
    if((dir=opendir(folder.c_str()))!=NULL)
    {
        /* print all the files and directories within directory */
        while((ent=readdir(dir))!=NULL)
        {
            InodeDescriptor inode;
            inode.name=ent->d_name;
            inode.absoluteFilePath=folder+'/'+ent->d_name;
            struct stat myStat;
            if(strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0)
            {
                if(stat((folder+ent->d_name).c_str(),&myStat)==0)
                {
                    if((myStat.st_mode&S_IFMT)==S_IFDIR && type&FacilityLibGeneral::ListFolder::Dirs)
                    {
                        inode.type=FacilityLibGeneral::InodeDescriptor::Type::Dir;
                        output.push_back(inode);
                    }
                    if((myStat.st_mode&S_IFMT)==S_IFREG && type&FacilityLibGeneral::ListFolder::Files)
                    {
                        inode.type=FacilityLibGeneral::InodeDescriptor::Type::File;
                        output.push_back(inode);
                    }
                }
            }
        }
        closedir(dir);
        std::sort(output.begin(),output.end());
        return output;
    }
    else
    {
        /* could not open directory */
        std::cerr << "Unable to open: " << folder << ", errno: " << errno << std::endl;
        return std::vector<InodeDescriptor>();
    }
}

std::vector<std::string> FacilityLibGeneral::listFolder(const std::string& folder,const std::string& suffix)
{
    std::vector<std::string> returnList;
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=listFolderNotRecursive(folder+suffix);//possible wait time here
    for (unsigned int index=0;index<entryList.size();++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::Dir)
        {
            const std::vector<std::string> &newList=listFolder(folder,suffix+fileInfo.name+text_slash);//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::File)
            returnList.push_back(suffix+fileInfo.name);
    }
    return returnList;
}

std::vector<std::string> FacilityLibGeneral::listFolderWithExclude(const std::string& folder,const std::string &exclude,const std::string& suffix)
{
    std::vector<std::string> returnList;
    if(suffix==exclude)
        return returnList;
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=listFolderNotRecursive(folder+suffix);//possible wait time here
    for (unsigned int index=0;index<entryList.size();++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::Dir)
        {
            const std::vector<std::string> &newList=listFolderWithExclude(folder,exclude,suffix+fileInfo.name+text_slash);//put unix separator because it's transformed into that's under windows too
            returnList.insert(returnList.end(),newList.begin(),newList.end());
        }
        else if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::File)
            returnList.push_back(suffix+fileInfo.name);
    }
    return returnList;
}

std::string FacilityLibGeneral::randomPassword(const std::string& string,const uint8_t& length)
{
    if(string.size()<2)
        return std::string();
    std::string randomPassword;
    int index=0;
    while(index<length)
    {
        randomPassword+=string.at(rand()%string.size());
        index++;
    }
    return randomPassword;
}

std::vector<std::string> FacilityLibGeneral::skinIdList(const std::string& skinPath)
{
    std::vector<std::string> skinFolderList;
    std::vector<FacilityLibGeneral::InodeDescriptor> entryList=listFolderNotRecursive(skinPath,FacilityLibGeneral::ListFolder::Dirs);//possible wait time here
    size_t entryMax = entryList.size();
    for(unsigned int index=0; index < entryMax; ++index)
    {
        const FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==FacilityLibGeneral::InodeDescriptor::Type::Dir)
            if(FacilityLibGeneral::isFile(fileInfo.absoluteFilePath+"/back.png") &&
                    FacilityLibGeneral::isFile(fileInfo.absoluteFilePath+"/front.png") &&
                    FacilityLibGeneral::isFile(fileInfo.absoluteFilePath+"/trainer.png"))
                skinFolderList.push_back(fileInfo.name);
    }
    std::sort(skinFolderList.begin(), skinFolderList.end());
    while(skinFolderList.size()>255)
        skinFolderList.pop_back();
    return skinFolderList;
}

std::string FacilityLibGeneral::dropPrefixAndSuffixLowerThen33(const std::string &str)
{
    unsigned int start=0;
    while(start<str.size())
    {
        if(str.at(start)>33)
            break;
        start++;
    }
    if(start>=str.size())
        return std::string();
    unsigned int stop=static_cast<unsigned int>(str.size())-1;
    while(str.at(stop)<33)
        stop--;
    return str.substr(start,stop-start+1);
}

bool FacilityLibGeneral::isFile(const std::string& file)
{
    #ifdef Q_OS_WIN32
    std::string fullPathFileToOpen=file;
    stringreplaceAll(fullPathFileToOpen,"/","\\");
    FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
    #else
    FILE *filedesc = fopen(file.c_str(), "rb");
    #endif
    if(filedesc!=NULL)
    {
        fclose(filedesc);
        return true;
    } else {
        return false;
    }
}

bool FacilityLibGeneral::isDir(const std::string& folder)
{
    DIR *dir=opendir(folder.c_str());
    if(dir!=NULL)
    {
        closedir(dir);
        return true;
    }
    else
        return false;
}

std::vector<char> FacilityLibGeneral::readAllFileAndClose(FILE * file)
{
    std::vector<char> data;
    fseek(file,0,SEEK_END);
    long fsize=ftell(file);
    fseek(file,0,SEEK_SET);

    data.resize(fsize);
    int64_t size=fread(data.data(),1,fsize,file);
    if(size!=fsize)
    {
        if(ferror(file))
        {
            int ferrorCode=ferror(file);
            int errnoCode=errno;
            std::cerr << "I/O error when reading, ferrorCode: " << ferrorCode << ", errnoCode: " << errnoCode << std::endl;
            abort();
        }
        if(feof(file))
        {
            std::cerr << "Error: At the end of file" << std::endl;
            abort();
        }
        std::cerr << "Read file size not same" << std::endl;
        abort();
    }
    fclose(file);

    return data;
}

uint32_t FacilityLibGeneral::fileSize(FILE * file)
{
    fseek(file,0,SEEK_END);
    const size_t fsize=ftell(file);
    if(fsize>0x7FFFFFFF)
    {
        std::cerr << "fsize>0x7FFFFFFF into FacilityLibGeneral::fileSize()" << std::endl;
        abort();
    }
    fseek(file,0,SEEK_SET);
    return static_cast<uint32_t>(fsize);
}

std::string FacilityLibGeneral::getSuffix(const std::string& fileName)
{
    const auto &pos=fileName.find_last_of('.');
    if(pos==std::string::npos)
        return std::string();
    else
        return fileName.substr(pos+1);
}

std::string FacilityLibGeneral::getSuffixAndValidatePathFromFS(const std::string& fileName)
{
    const size_t &size=fileName.size();
    if(size<5)
        return std::string();
    const char * const data=fileName.data();
    size_t index=size;
    while(1)//the last 4 char
    {
        index--;
        const char &currentChar=data[index];
        if(currentChar=='.')
            break;
        else if(currentChar<97 || currentChar>122)
            return std::string();
        if(index<=(size-5))
            return std::string();
        if(index<=1)
            return std::string();
    }
    size_t index2=0;
    //not needed: the FS will never return that's: if end with / false
    //not needed: the FS will never return that's: if contains // false
    while(index2<index)
    {
        const char &currentChar=data[index2];
        if(currentChar>=97 && currentChar<=122)//a-z
        {}
        else if(currentChar>=45 && currentChar<=57)//0-9 + / + . + -
        {}
        else if(currentChar==95)
        {}
        else
            return std::string();
        index2++;
    }
    //not needed: if start with / false
    return fileName.substr(index+1);
}

std::string FacilityLibGeneral::getFolderFromFile(const std::string& fileName)
{
    const auto &pos=fileName.find_last_of('/');
    if(pos==std::string::npos)
        return std::string();
    else
        return fileName.substr(0,pos);
}

/*std::string FacilityLibGeneral::secondsToString(const uint64_t &seconds)
{
    if(seconds<60)
        return QObject::tr("%n second(s)","",seconds);
    else if(seconds<60*60)
        return QObject::tr("%n minute(s)","",seconds/60);
    else if(seconds<60*60*24)
        return QObject::tr("%n hour(s)","",seconds/(60*60));
    else
        return QObject::tr("%n day(s)","",seconds/(60*60*24));
}*/

bool FacilityLibGeneral::rmpath(const std::string &dirPath)
{
    DIR *dir;
    struct dirent *ent;
    if((dir=opendir(dirPath.c_str()))!=NULL)
    {
        bool allHaveWork=true;
        /* print all the files and directories within directory */
        while((ent=readdir(dir))!=NULL)
        {
            InodeDescriptor inode;
            inode.name=ent->d_name;
            inode.absoluteFilePath=dirPath+'/'+ent->d_name;
            struct stat myStat;
            if(strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0)
            {
                if(stat(inode.absoluteFilePath.c_str(),&myStat)==0)
                {
                    if((myStat.st_mode&S_IFMT)==S_IFDIR)
                    {
                        if(!rmpath(inode.absoluteFilePath))
                            allHaveWork=false;
                    }
                    else
                    {
                        if(remove(inode.absoluteFilePath.c_str())!=0)
                        {
                            std::cerr << "Unable to remove file: " << inode.absoluteFilePath << ", errno: " << errno << std::endl;
                            allHaveWork=false;
                        }
                    }
                }
            }
        }
        closedir(dir);

        if(!allHaveWork)
            return false;
        allHaveWork=rmdir(dirPath.c_str())==0;
        if(!allHaveWork)
        {
            std::cerr << "Unable to remove folder: " << dirPath << ", errno: " << errno << std::endl;
            return false;
        }
        return allHaveWork;
    }
    else
    {
        if(errno!=ENOENT)
        {
            std::cerr << "Unable to remove folder: " << dirPath << ", errno: " << errno << std::endl;
            return false;
        }
        else
            return true;
    }
}

/*std::string FacilityLibGeneral::timeToString(const uint32_t &time)
{
    if(time>=3600*24*10)
        return QObject::tr("%n day(s)","",time/(3600*24));
    else if(time>=3600*24)
        return QObject::tr("%n day(s) and %1","",time/(3600*24)).arg(QObject::tr("%n hour(s)","",(time%(3600*24))/3600));
    else if(time>=3600)
        return QObject::tr("%n hour(s) and %1","",time/3600).arg(QObject::tr("%n minute(s)","",(time%3600)/60));
    else
        return QObject::tr("%n minute(s) and %1","",time/60).arg(QObject::tr("%n second(s)","",time%60));
}*/
