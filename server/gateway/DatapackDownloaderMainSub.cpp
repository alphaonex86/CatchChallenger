#include "DatapackDownloaderMainSub.h"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <regex>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"

//need host + port here to have datapack base

std::regex DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX(DATAPACK_FILE_REGEX);
std::unordered_set<std::string> DatapackDownloaderMainSub::extensionAllowed;
std::regex DatapackDownloaderMainSub::excludePathMain("^sub[/\\\\]");
std::string DatapackDownloaderMainSub::commandUpdateDatapackMain;
std::string DatapackDownloaderMainSub::commandUpdateDatapackSub;
std::vector<std::string> DatapackDownloaderMainSub::httpDatapackMirrorServerList;

std::unordered_map<std::string,std::unordered_map<std::string,DatapackDownloaderMainSub *> > DatapackDownloaderMainSub::datapackDownloaderMainSub;

DatapackDownloaderMainSub::DatapackDownloaderMainSub(const std::string &mDatapackBase, const std::string &mainDatapackCode, const std::string &subDatapackCode) :
    mDatapackBase(mDatapackBase),
    mDatapackMain(mDatapackBase+"map/main/"+mainDatapackCode+"/"),
    mainDatapackCode(mainDatapackCode),
    subDatapackCode(subDatapackCode)
{
    datapackStatus=DatapackStatus::Main;
    datapackTarXzMain=false;
    datapackTarXzSub=false;
    index_mirror_main=0;
    index_mirror_sub=0;
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
    if(!subDatapackCode.empty())
        mDatapackSub=mDatapackBase+"map/main/"+mainDatapackCode+"/sub/"+subDatapackCode+"/";
}

DatapackDownloaderMainSub::~DatapackDownloaderMainSub()
{
}

void DatapackDownloaderMainSub::datapackDownloadError()
{
    unsigned int index=0;
    while(index<clientInSuspend.size())
    {
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer * const>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->disconnectClient();
        index++;
    }
    clientInSuspend.clear();
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
}

void DatapackDownloaderMainSub::writeNewFileToRoute(const std::string &fileName, const std::vector<char> &data)
{
    switch(datapackStatus)
    {
        case DatapackStatus::Main:
            writeNewFileMain(fileName,data);
        break;
        case DatapackStatus::Sub:
            writeNewFileSub(fileName,data);
        break;
        default:
        return;
    }
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
                    std::cerr << "remove the file: " << mDatapackMain << '/' << datapackFilesListMain.at(index) << std::endl;
                    if(remove((mDatapackMain+'/'+datapackFilesListMain.at(index)).c_str())!=0)
                        std::cerr << "unable to remove the file: " << datapackFilesListMain.at(index) << ": " << errno << std::endl;
                    //removeFile(datapackFilesListMain.at(index));
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
                    std::cerr << "remove the file: " << mDatapackSub << '/' << datapackFilesListSub.at(index) << std::endl;
                    if(remove((mDatapackSub+'/'+datapackFilesListSub.at(index)).c_str())!=0)
                        std::cerr << "unable to remove the file: " << datapackFilesListSub.at(index) << ": " << errno << std::endl;
                    //removeFile(datapackFilesListSub.at(index));
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

    unsigned int index=0;
    while(index<clientInSuspend.size())
    {
        LinkToGameServer * const clientLink=static_cast<LinkToGameServer * const>(clientInSuspend.at(index));
        if(clientLink!=NULL)
            clientLink->sendDiffered0205Reply();
        index++;
    }
    clientInSuspend.clear();

    //regen the datapack cache
    if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()<=1)
    {
        EpollClientLoginSlave::datapack_file_main[mainDatapackCode].datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackMain);
        EpollClientLoginSlave::datapack_file_sub[mainDatapackCode][subDatapackCode].datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackSub);
    }

    resetAll();

    /*if(!DatapackDownloaderMainSub::commandUpdateDatapackMain.empty())
    {
        if(QProcess::execute(QString::fromStdString(DatapackDownloaderMainSub::commandUpdateDatapackMain),QStringList() << QString::fromStdString(mDatapackMain))<0)
            std::cerr << "Unable to execute " << DatapackDownloaderMainSub::commandUpdateDatapackMain << " " << mDatapackMain << std::endl;
    }
    if(!DatapackDownloaderMainSub::commandUpdateDatapackSub.empty() && !mDatapackSub.empty())
    {
        if(QProcess::execute(QString::fromStdString(DatapackDownloaderMainSub::commandUpdateDatapackSub),QStringList() << QString::fromStdString(mDatapackSub))<0)
            std::cerr << "Unable to execute " << DatapackDownloaderMainSub::commandUpdateDatapackSub << " " << mDatapackSub << std::endl;
    }*/
}

void DatapackDownloaderMainSub::checkIfContinueOrFinished()
{
    wait_datapack_content_main=false;
    if(subDatapackCode.empty())
        haveTheDatapackMainSub();
    else
    {
        datapackStatus=DatapackStatus::Sub;
        sendDatapackContentSub();
    }
}

void DatapackDownloaderMainSub::sendDatapackProgressionMainSub(void * client)
{
    if(client==NULL)
        return;
    EpollClientLoginSlave * const client_real=static_cast<EpollClientLoginSlave *>(client);
    uint8_t progression=0;//do the adaptative from curl progression
    if(clientInSuspend.empty())
        progression=0;//not specific to base client, put 0 as generic value
    client_real->sendDatapackProgression(progression);
}
