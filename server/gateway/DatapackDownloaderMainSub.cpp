#include "DatapackDownloaderMainSub.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include <cmath>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QProcess>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"

//need host + port here to have datapack base

QString DatapackDownloaderMainSub::text_slash=QLatin1Literal("/");
QString DatapackDownloaderMainSub::text_dotcoma=QLatin1Literal(";");
QRegularExpression DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX=QRegularExpression(DATAPACK_FILE_REGEX);
QSet<QString> DatapackDownloaderMainSub::extensionAllowed;
QRegularExpression DatapackDownloaderMainSub::excludePathMain("^sub[/\\\\]");
QString DatapackDownloaderMainSub::commandUpdateDatapackMain;
QString DatapackDownloaderMainSub::commandUpdateDatapackSub;

QHash<QString,QHash<QString,DatapackDownloaderMainSub *> > DatapackDownloaderMainSub::datapackDownloaderMainSub;

DatapackDownloaderMainSub::DatapackDownloaderMainSub(const QString &mDatapackBase, const QString &mainDatapackCode, const QString &subDatapackCode) :
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
    if(!subDatapackCode.isEmpty())
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
}

void DatapackDownloaderMainSub::writeNewFileToRoute(const QString &fileName, const QByteArray &data)
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
            if(datapackFilesListMain.isEmpty() && size==1)
            {
                if(!httpModeMain)
                    checkIfContinueOrFinished();
                return;
            }
            unsigned int pos=0;
            QList<bool> boolList;
            while((size-pos)>0)
            {
                const quint8 &returnCode=data[pos];
                boolList.append(returnCode&0x01);
                boolList.append(returnCode&0x02);
                boolList.append(returnCode&0x04);
                boolList.append(returnCode&0x08);
                boolList.append(returnCode&0x10);
                boolList.append(returnCode&0x20);
                boolList.append(returnCode&0x40);
                boolList.append(returnCode&0x80);
                pos++;
            }
            if(boolList.size()<datapackFilesListMain.size())
            {
                qDebug() << (QStringLiteral("bool list too small with DatapackDownloaderMainSub::datapackFileList"));
                return;
            }
            int index=0;
            while(index<datapackFilesListMain.size())
            {
                if(boolList.first())
                {
                    DebugClass::debugConsole(QStringLiteral("remove the file: %1").arg(mDatapackMain+text_slash+datapackFilesListMain.at(index)));
                    QFile file(mDatapackMain+text_slash+datapackFilesListMain.at(index));
                    if(!file.remove())
                        DebugClass::debugConsole(QStringLiteral("unable to remove the file: %1: %2").arg(datapackFilesListMain.at(index)).arg(file.errorString()));
                    //removeFile(datapackFilesListMain.at(index));
                }
                boolList.removeFirst();
                index++;
            }
            datapackFilesListMain.clear();
            cleanDatapackMain(QString());
            if(boolList.size()>=8)
            {
                qDebug() << (QStringLiteral("bool list too big with DatapackDownloaderMainSub::datapackFileList"));
                return;
            }
            if(!httpModeMain)
                checkIfContinueOrFinished();
            datapackStatus=DatapackStatus::Sub;
        }
        break;
        case DatapackStatus::Sub:
        {
            if(datapackFilesListSub.isEmpty() && size==1)
            {
                if(!httpModeSub)
                    datapackDownloadFinishedSub();
                return;
            }
            unsigned int pos=0;
            QList<bool> boolList;
            while((size-pos)>0)
            {
                const quint8 &returnCode=data[pos];
                boolList.append(returnCode&0x01);
                boolList.append(returnCode&0x02);
                boolList.append(returnCode&0x04);
                boolList.append(returnCode&0x08);
                boolList.append(returnCode&0x10);
                boolList.append(returnCode&0x20);
                boolList.append(returnCode&0x40);
                boolList.append(returnCode&0x80);
                pos++;
            }
            if(boolList.size()<datapackFilesListSub.size())
            {
                qDebug() << (QStringLiteral("bool list too small with DatapackDownloaderMainSub::datapackFileList"));
                return;
            }
            int index=0;
            while(index<datapackFilesListSub.size())
            {
                if(boolList.first())
                {
                    DebugClass::debugConsole(QStringLiteral("remove the file: %1").arg(mDatapackSub+text_slash+datapackFilesListSub.at(index)));
                    QFile file(mDatapackSub+text_slash+datapackFilesListSub.at(index));
                    if(!file.remove())
                        DebugClass::debugConsole(QStringLiteral("unable to remove the file: %1: %2").arg(datapackFilesListSub.at(index)).arg(file.errorString()));
                    //removeFile(datapackFilesListSub.at(index));
                }
                boolList.removeFirst();
                index++;
            }
            datapackFilesListSub.clear();
            cleanDatapackSub(QString());
            if(boolList.size()>=8)
            {
                qDebug() << (QStringLiteral("bool list too big with DatapackDownloaderMainSub::datapackFileList"));
                return;
            }
            if(!httpModeSub)
                datapackDownloadFinishedSub();
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

    if(!DatapackDownloaderMainSub::commandUpdateDatapackMain.isEmpty())
    {
        if(QProcess::execute(DatapackDownloaderMainSub::commandUpdateDatapackMain,QStringList() << mDatapackMain)<0)
            qDebug() << "Unable to execute " << DatapackDownloaderMainSub::commandUpdateDatapackMain << " " << mDatapackMain;
    }
    if(!DatapackDownloaderMainSub::commandUpdateDatapackSub.isEmpty() && !mDatapackSub.isEmpty())
    {
        if(QProcess::execute(DatapackDownloaderMainSub::commandUpdateDatapackSub,QStringList() << mDatapackSub)<0)
            qDebug() << "Unable to execute " << DatapackDownloaderMainSub::commandUpdateDatapackSub << " " << mDatapackSub;
    }
}

