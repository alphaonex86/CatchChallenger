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

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"

//need host + port here to have datapack base

QString DatapackDownloaderMainSub::text_slash=QLatin1Literal("/");
QString DatapackDownloaderMainSub::text_dotcoma=QLatin1Literal(";");
QRegularExpression DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX=QRegularExpression(DATAPACK_FILE_REGEX);
QRegularExpression DatapackDownloaderMainSub::excludePathBase("^map[/\\\\]main[/\\\\]");
QRegularExpression DatapackDownloaderMainSub::excludePathMain("^sub[/\\\\]");

DatapackDownloaderMainSub * DatapackDownloaderMainSub::datapackDownloaderMainSub=NULL;

DatapackDownloaderMainSub::DatapackDownloaderMainSub() :
    qnamQueueCount(0),
    qnamQueueCount2(0),
    qnamQueueCount3(0),
    qnamQueueCount4(0)
{
    datapackStatus=DatapackStatus::Base;
    datapackTarXzBase=false;
    datapackTarXzMain=false;
    datapackTarXzSub=false;
    index_mirror_base=0;
    index_mirror_main=0;
    index_mirror_sub=0;
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;
}

DatapackDownloaderMainSub::~DatapackDownloaderMainSub()
{
}

void DatapackDownloaderMainSub::datapackFileList(const char * const data,const unsigned int &size)
{
    switch(datapackStatus)
    {
        case DatapackStatus::Base:
        {
            if(datapackFilesListBase.isEmpty() && data.size()==1)
            {
                if(!httpModeBase)
                    haveTheDatapack();
                return;
            }
            QList<bool> boolList;
            while((in.device()->size()-in.device()->pos())>0)
            {
                quint8 returnCode;
                in >> returnCode;
                boolList.append(returnCode&0x01);
                boolList.append(returnCode&0x02);
                boolList.append(returnCode&0x04);
                boolList.append(returnCode&0x08);
                boolList.append(returnCode&0x10);
                boolList.append(returnCode&0x20);
                boolList.append(returnCode&0x40);
                boolList.append(returnCode&0x80);
            }
            if(boolList.size()<datapackFilesListBase.size())
            {
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too small with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                return;
            }
            int index=0;
            while(index<datapackFilesListBase.size())
            {
                if(boolList.first())
                {
                    DebugClass::debugConsole(QStringLiteral("remove the file: %1").arg(mDatapackBase+text_slash+datapackFilesListBase.at(index)));
                    QFile file(mDatapackBase+text_slash+datapackFilesListBase.at(index));
                    if(!file.remove())
                        DebugClass::debugConsole(QStringLiteral("unable to remove the file: %1: %2").arg(datapackFilesListBase.at(index)).arg(file.errorString()));
                    //removeFile(datapackFilesListBase.at(index));
                }
                boolList.removeFirst();
                index++;
            }
            datapackFilesListBase.clear();
            cleanDatapackBase(QString());
            if(boolList.size()>=8)
            {
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too big with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                return;
            }
            if(!httpModeBase)
                haveTheDatapack();
            datapackStatus=DatapackStatus::Main;
        }
        break;
        case DatapackStatus::Main:
        {
            if(datapackFilesListMain.isEmpty() && data.size()==1)
            {
                if(!httpModeMain)
                    checkIfContinueOrFinished();
                return;
            }
            QList<bool> boolList;
            while((in.device()->size()-in.device()->pos())>0)
            {
                quint8 returnCode;
                in >> returnCode;
                boolList.append(returnCode&0x01);
                boolList.append(returnCode&0x02);
                boolList.append(returnCode&0x04);
                boolList.append(returnCode&0x08);
                boolList.append(returnCode&0x10);
                boolList.append(returnCode&0x20);
                boolList.append(returnCode&0x40);
                boolList.append(returnCode&0x80);
            }
            if(boolList.size()<datapackFilesListMain.size())
            {
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too small with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too big with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                return;
            }
            if(!httpModeMain)
                checkIfContinueOrFinished();
            datapackStatus=DatapackStatus::Sub;
        }
        break;
        case DatapackStatus::Sub:
        {
            if(datapackFilesListSub.isEmpty() && data.size()==1)
            {
                if(!httpModeSub)
                    datapackDownloadFinishedSub();
                return;
            }
            QList<bool> boolList;
            while((in.device()->size()-in.device()->pos())>0)
            {
                quint8 returnCode;
                in >> returnCode;
                boolList.append(returnCode&0x01);
                boolList.append(returnCode&0x02);
                boolList.append(returnCode&0x04);
                boolList.append(returnCode&0x08);
                boolList.append(returnCode&0x10);
                boolList.append(returnCode&0x20);
                boolList.append(returnCode&0x40);
                boolList.append(returnCode&0x80);
            }
            if(boolList.size()<datapackFilesListSub.size())
            {
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too small with sub ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
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
                newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too big with sub ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                return;
            }
            if(!httpModeSub)
                datapackDownloadFinishedSub();
            datapackStatus=DatapackStatus::Finished;
        }
        break;
        default:
        return;
    }
}

//general data
void DatapackDownloaderMainSub::defineMaxPlayers(const quint16 &maxPlayers)
{
    ProtocolParsing::setMaxPlayers(maxPlayers);
}

void DatapackDownloaderMainSub::resetAll()
{
    httpModeBase=false;
    httpModeMain=false;
    httpModeSub=false;
    httpError=false;
    RXSize=0;
    TXSize=0;
    query_files_list_base.clear();
    query_files_list_main.clear();
    query_files_list_sub.clear();
    urlInWaitingListBase.clear();
    urlInWaitingListMain.clear();
    urlInWaitingListSub.clear();
    wait_datapack_content_base=false;
    wait_datapack_content_main=false;
    wait_datapack_content_sub=false;

    Api_protocol::resetAll();
}

void DatapackDownloaderMainSub::tryConnect(QString host,quint16 port)
{
    if(socket==NULL)
        return;
    DebugClass::debugConsole(QStringLiteral("Try connect on: %1:%2").arg(host).arg(port));
    this->host=host;
    this->port=port;
    socket->connectToHost(host,port);
}

void DatapackDownloaderMainSub::disconnected()
{
    if(stage()==StageConnexion::Stage1 || stage()==StageConnexion::Stage3)
    {
        wait_datapack_content_base=false;
        wait_datapack_content_main=false;
        wait_datapack_content_sub=false;
        resetAll();
    }
}

void DatapackDownloaderMainSub::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

QString DatapackDownloaderMainSub::getHost()
{
    return host;
}

quint16 DatapackDownloaderMainSub::getPort()
{
    return port;
}

void DatapackDownloaderMainSub::sendDatapackContentMainSub()
{
    sendDatapackContentMain();
}

void DatapackDownloaderMainSub::setProxy(const QNetworkProxy &proxy)
{
    this->proxy=proxy;
    if(proxy.type()==QNetworkProxy::Socks5Proxy)
    {
        qnam.setProxy(proxy);
        qnam2.setProxy(proxy);
        qnam3.setProxy(proxy);
        qnam4.setProxy(proxy);
    }
    else
    {
        qnam.setProxy(QNetworkProxy::applicationProxy());
        qnam2.setProxy(QNetworkProxy::applicationProxy());
        qnam3.setProxy(QNetworkProxy::applicationProxy());
        qnam4.setProxy(QNetworkProxy::applicationProxy());
    }
}
