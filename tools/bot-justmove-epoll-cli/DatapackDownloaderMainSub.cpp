#include "DatapackDownloaderMainSub.hpp"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <regex>
#include <string.h>

#include "../../general/base/GeneralVariable.hpp"

//need host + port here to have datapack base

std::regex DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> DatapackDownloaderMainSub::extensionAllowed;
std::regex DatapackDownloaderMainSub::excludePathMain("^sub[/\\\\]");
std::vector<std::string> DatapackDownloaderMainSub::httpDatapackMirrorServerList;

/*
do datapack checksum via sendDatapackContentX() //then will call datapackChecksumDoneX()
if(CommonSettingsCommon::commonSettingsCommon.datapackHashX server checksum!=client checksum)//sha224
{
    if(server http mirror.empty())
        sendFilelistByCCProtocol() with XXH32()/datapack_file_list()
    else
    {
        getHTTPmirrorfilelist()
        do partial datapack checksum with XXH32()/datapack_file_list()
        get the missing changed file()
    }
}
*/

DatapackDownloaderMainSub::DatapackDownloaderMainSub()
{
    httpError=false;
    httpModeMain=false;
    httpModeSub=false;
    datapackStatus=DatapackStatus::Main;
    datapackTarMain=false;
    datapackTarSub=false;
    index_mirror_main=0;
    index_mirror_sub=0;
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
}

DatapackDownloaderMainSub::~DatapackDownloaderMainSub()
{
}

void DatapackDownloaderMainSub::datapackDownloadError()
{
    newError("Download error","DatapackDownloaderMainSub::datapackDownloadError()");
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
}

void DatapackDownloaderMainSub::datapackFileList(const char * const data,const unsigned int &size)
{
    switch(datapackStatus)
    {
        case DatapackStatus::Main:
        {
            if(datapackFilesListMain.empty() && size==1)
            {
                if(!httpModeMain)
                    checkIfContinueOrFinished();
                return;
            }
            unsigned int pos=0;
            std::vector<bool> boolList;
            boolList.reserve(size*8);
            while((size-pos)>0)
            {
                const uint8_t &returnCode=data[pos];
                boolList.push_back(returnCode&0x01);
                boolList.push_back(returnCode&0x02);
                boolList.push_back(returnCode&0x04);
                boolList.push_back(returnCode&0x08);
                boolList.push_back(returnCode&0x10);
                boolList.push_back(returnCode&0x20);
                boolList.push_back(returnCode&0x40);
                boolList.push_back(returnCode&0x80);
                pos++;
            }
            if(boolList.size()<datapackFilesListMain.size())
            {
                std::cerr << "bool list too small with DatapackDownloaderMainSub::datapackFileList" << std::endl;
                return;
            }
            unsigned int index=0;
            while(index<datapackFilesListMain.size())
            {
                if(boolList.at(index))
                {
                    std::cerr << "remove the file: " << datapackPathMain() << '/' << datapackFilesListMain.at(index) << std::endl;
                    if(remove((datapackPathMain()+'/'+datapackFilesListMain.at(index)).c_str())!=0)
                        std::cerr << "unable to remove the file: " << datapackFilesListMain.at(index) << ": " << errno << std::endl;
                    //removeFile(datapackFilesListMain.at(index));
                    std::cout << "remove: " << datapackPathMain()+'/'+datapackFilesListMain.at(index) << std::endl;
                }
                index++;
            }
            boolList.clear();
            datapackFilesListMain.clear();
            cleanDatapackMain(std::string());
            if(boolList.size()>=8)
            {
                std::cerr << "bool list too big with DatapackDownloaderMain::datapackFileList" << std::endl;
                return;
            }
            if(!httpModeMain)
                checkIfContinueOrFinished();
            datapackStatus=DatapackStatus::Sub;
        }
        break;
        case DatapackStatus::Sub:
        {
            if(datapackFilesListSub.empty() && size==1)
            {
                if(!httpModeSub)
                    checkIfContinueOrFinished();
                return;
            }
            unsigned int pos=0;
            std::vector<bool> boolList;
            boolList.reserve(size*8);
            while((size-pos)>0)
            {
                const uint8_t &returnCode=data[pos];
                boolList.push_back(returnCode&0x01);
                boolList.push_back(returnCode&0x02);
                boolList.push_back(returnCode&0x04);
                boolList.push_back(returnCode&0x08);
                boolList.push_back(returnCode&0x10);
                boolList.push_back(returnCode&0x20);
                boolList.push_back(returnCode&0x40);
                boolList.push_back(returnCode&0x80);
                pos++;
            }
            if(boolList.size()<datapackFilesListSub.size())
            {
                std::cerr << "bool list too small with DatapackDownloaderSub::datapackFileList" << std::endl;
                return;
            }
            unsigned int index=0;
            while(index<datapackFilesListSub.size())
            {
                if(boolList.at(index))
                {
                    std::cerr << "remove the file: " << datapackPathSub() << '/' << datapackFilesListSub.at(index) << std::endl;
                    if(remove((datapackPathSub()+'/'+datapackFilesListSub.at(index)).c_str())!=0)
                        std::cerr << "unable to remove the file: " << datapackFilesListSub.at(index) << ": " << errno << std::endl;
                    //removeFile(datapackFilesListSub.at(index));
                    std::cout << "remove: " << datapackPathSub()+'/'+datapackFilesListSub.at(index) << std::endl;
                }
                index++;
            }
            boolList.clear();
            datapackFilesListSub.clear();
            cleanDatapackSub(std::string());
            if(boolList.size()>=8)
            {
                std::cerr << "bool list too big with DatapackDownloaderSub::datapackFileList" << std::endl;
                return;
            }
            if(!httpModeSub)
                checkIfContinueOrFinished();
        }
        break;
        default:
        return;
    }
}

void DatapackDownloaderMainSub::resetAll()
{
    httpModeMain=false;
    httpModeSub=false;
    httpError=false;
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
}

void DatapackDownloaderMainSub::sendDatapackContentMainSub()
{
    sendDatapackContentMain();
}

void DatapackDownloaderMainSub::haveTheDatapackMainSub()
{
    if(DatapackDownloaderMainSub::httpDatapackMirrorServerList.empty())
    {
        std::cout << "Have the datapack main sub" << std::endl;
    }

    resetAll();
}

void DatapackDownloaderMainSub::checkIfContinueOrFinished()
{
    wait_datapack_content_main=false;
    if(subDatapackCode().empty())
        haveTheDatapackMainSub();
    else
    {
        datapackStatus=DatapackStatus::Sub;
        sendDatapackContentSub();
    }
}

size_t DatapackDownloaderMainSub::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = static_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
