#include "Api_client_real.h"

#include <iostream>

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include <cmath>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QDataStream>
#include <QDebug>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../tarcompressed/TarDecode.h"
#include "../../general/base/GeneralVariable.h"

//need host + port here to have datapack base

QString Api_client_real::text_slash=QStringLiteral("/");
QString Api_client_real::text_dotcoma=QStringLiteral(";");
QRegularExpression Api_client_real::regex_DATAPACK_FILE_REGEX=QRegularExpression(DATAPACK_FILE_REGEX);
std::regex Api_client_real::excludePathBase("^map[/\\\\]main[/\\\\]");
std::regex Api_client_real::excludePathMain("^sub[/\\\\]");

Api_client_real::Api_client_real(ConnectedSocket *socket) :
    Api_protocol_Qt(socket),
    qnamQueueCount(0),
    qnamQueueCount2(0),
    qnamQueueCount3(0),
    qnamQueueCount4(0)
{
    datapackTarBase=false;
    datapackTarMain=false;
    datapackTarSub=false;
    index_mirror_base=0;
    index_mirror_main=0;
    index_mirror_sub=0;
    host="localhost";
    port=42489;
    if(!connect(socket, &ConnectedSocket::disconnected,	this,&Api_client_real::disconnect))
        abort();
    if(!connect(this,   &Api_client_real::QtnewFileBase,      this,&Api_client_real::writeNewFileBase))
        abort();
    if(!connect(this,   &Api_client_real::QtnewFileMain,      this,&Api_client_real::writeNewFileMain))
        abort();
    if(!connect(this,   &Api_client_real::QtnewFileSub,      this,&Api_client_real::writeNewFileSub))
        abort();
    if(!connect(this,   &Api_client_real::QtnewHttpFileBase,  this,&Api_client_real::getHttpFileBase))
        abort();
    if(!connect(this,   &Api_client_real::QtnewHttpFileMain,  this,&Api_client_real::getHttpFileMain))
        abort();
    if(!connect(this,   &Api_client_real::QtnewHttpFileSub,  this,&Api_client_real::getHttpFileSub))
        abort();
    if(!connect(this,   &Api_client_real::doDifferedChecksumBase,&datapackChecksum,&CatchChallenger::QtDatapackChecksum::doDifferedChecksumBase))
        abort();
    if(!connect(this,   &Api_client_real::doDifferedChecksumMain,&datapackChecksum,&CatchChallenger::QtDatapackChecksum::doDifferedChecksumMain))
        abort();
    if(!connect(this,   &Api_client_real::doDifferedChecksumSub,&datapackChecksum,&CatchChallenger::QtDatapackChecksum::doDifferedChecksumSub))
        abort();
    if(!connect(&datapackChecksum,&CatchChallenger::QtDatapackChecksum::datapackChecksumDoneBase,this,&Api_client_real::datapackChecksumDoneBase))
        abort();
    if(!connect(&datapackChecksum,&CatchChallenger::QtDatapackChecksum::datapackChecksumDoneMain,this,&Api_client_real::datapackChecksumDoneMain))
        abort();
    if(!connect(&datapackChecksum,&CatchChallenger::QtDatapackChecksum::datapackChecksumDoneSub,this,&Api_client_real::datapackChecksumDoneSub))
        abort();
    if(!connect(&zstdDecodeThreadBase,&QZstdDecodeThread::decodedIsFinish,this,&Api_client_real::decodedIsFinishBase))
        abort();
    if(!connect(&zstdDecodeThreadMain,&QZstdDecodeThread::decodedIsFinish,this,&Api_client_real::decodedIsFinishMain))
        abort();
    if(!connect(&zstdDecodeThreadSub,&QZstdDecodeThread::decodedIsFinish,this,&Api_client_real::decodedIsFinishSub))
        abort();
    resetAll();
    //dataClear();do into disconnected()
}

Api_client_real::~Api_client_real()
{
    if(socket!=NULL)
    {
        socket->abort();
        socket->disconnectFromHost();
        if(socket->state()!=QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected();
    }
    #ifndef NOTHREADS
    zstdDecodeThreadBase.exit();
    zstdDecodeThreadBase.quit();
    zstdDecodeThreadMain.exit();
    zstdDecodeThreadMain.quit();
    zstdDecodeThreadSub.exit();
    zstdDecodeThreadSub.quit();

    zstdDecodeThreadBase.wait();
    zstdDecodeThreadMain.wait();
    zstdDecodeThreadSub.wait();
    #endif
}

bool Api_client_real::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    return Api_client_real::parseReplyData(mainCodeType,queryNumber,std::string(data,size));
}

bool Api_client_real::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const std::string &data)
{
    if(querySendTime.find(queryNumber)!=querySendTime.cend())
    {
        std::time_t result = std::time(nullptr);
        if((uint64_t)result>=(uint64_t)querySendTime.at(queryNumber))
            lastReplyTime(result-querySendTime.at(queryNumber));
        querySendTime.erase(queryNumber);
    }
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0xA1:
        {
            std::cout << "Select character: " << datapackStatus << std::endl;
            switch(datapackStatus)
            {
                case DatapackStatus::Base:
                {
                    if(datapackFilesListBase.empty() && data.size()==1)
                    {
                        datapackStatus=DatapackStatus::Main;
                        if(!httpModeBase)
                            haveTheDatapack();
                        return true;
                    }
                    QList<bool> boolList;
                    while((in.device()->size()-in.device()->pos())>0)
                    {
                        uint8_t returnCode;
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
                    if((uint32_t)boolList.size()<datapackFilesListBase.size())
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too small with main ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(mainCodeType).arg(queryNumber).toStdString());
                        return false;
                    }
                    unsigned int index=0;
                    while(index<datapackFilesListBase.size())
                    {
                        if(boolList.first())
                        {
                            qDebug() << (QStringLiteral("remove the file: %1")
                                         .arg(QString::fromStdString(mDatapackBase)+text_slash+
                                              QString::fromStdString(datapackFilesListBase.at(index))));
                            QFile file(QString::fromStdString(mDatapackBase)+text_slash+QString::fromStdString(datapackFilesListBase.at(index)));
                            if(!file.remove())
                                qDebug() << (QStringLiteral("unable to remove the file: %1: %2").arg(QString::fromStdString(datapackFilesListBase.at(index))).arg(file.errorString()));
                            //removeFile(datapackFilesListBase.at(index));
                        }
                        boolList.removeFirst();
                        index++;
                    }
                    datapackFilesListBase.clear();
                    cleanDatapackBase();
                    if(boolList.size()>=8)
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too big with main ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(queryNumber).toStdString());
                        return false;
                    }
                    if(!httpModeBase)
                        haveTheDatapack();
                    datapackStatus=DatapackStatus::Main;
                }
                break;
                case DatapackStatus::Main:
                {
                    if(datapackFilesListMain.empty() && data.size()==1)
                    {
                        datapackStatus=DatapackStatus::Sub;
                        if(!httpModeMain)
                            checkIfContinueOrFinished();
                        return true;
                    }
                    QList<bool> boolList;
                    while((in.device()->size()-in.device()->pos())>0)
                    {
                        uint8_t returnCode;
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
                    if((uint32_t)boolList.size()<datapackFilesListMain.size())
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too small with main ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(mainCodeType).arg(queryNumber).toStdString());
                        return false;
                    }
                    unsigned int index=0;
                    while(index<datapackFilesListMain.size())
                    {
                        if(boolList.first())
                        {
                            qDebug() << (QStringLiteral("remove the file: %1")
                                         .arg(QString::fromStdString(mDatapackMain)+text_slash+QString::fromStdString(datapackFilesListMain.at(index))));
                            QFile file(QString::fromStdString(mDatapackMain)+text_slash+QString::fromStdString(datapackFilesListMain.at(index)));
                            if(!file.remove())
                                qDebug() << (QStringLiteral("unable to remove the file: %1: %2").arg(QString::fromStdString(datapackFilesListMain.at(index))).arg(file.errorString()));
                            //removeFile(datapackFilesListMain.at(index));
                        }
                        boolList.removeFirst();
                        index++;
                    }
                    datapackFilesListMain.clear();
                    cleanDatapackMain();
                    if(boolList.size()>=8)
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too big with main ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(mainCodeType).arg(queryNumber).toStdString());
                        return false;
                    }
                    if(!httpModeMain)
                        checkIfContinueOrFinished();
                    datapackStatus=DatapackStatus::Sub;
                }
                break;
                case DatapackStatus::Sub:
                {
                    if(datapackFilesListSub.empty() && data.size()==1)
                    {
                        datapackStatus=DatapackStatus::Finished;
                        if(!httpModeSub)
                            datapackDownloadFinishedSub();
                        return true;
                    }
                    QList<bool> boolList;
                    while((in.device()->size()-in.device()->pos())>0)
                    {
                        uint8_t returnCode;
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
                    if((uint32_t)boolList.size()<datapackFilesListSub.size())
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too small with sub ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(mainCodeType).arg(queryNumber).toStdString());
                        return false;
                    }
                    unsigned int index=0;
                    while(index<datapackFilesListSub.size())
                    {
                        if(boolList.first())
                        {
                            qDebug() << (QStringLiteral("remove the file: %1")
                                         .arg(QString::fromStdString(mDatapackSub)+text_slash+QString::fromStdString(datapackFilesListSub.at(index))));
                            QFile file(QString::fromStdString(mDatapackSub)+text_slash+QString::fromStdString(datapackFilesListSub.at(index)));
                            if(!file.remove())
                                qDebug() << (QStringLiteral("unable to remove the file: %1: %2").arg(QString::fromStdString(datapackFilesListSub.at(index))).arg(file.errorString()));
                            //removeFile(datapackFilesListSub.at(index));
                        }
                        boolList.removeFirst();
                        index++;
                    }
                    datapackFilesListSub.clear();
                    cleanDatapackSub();
                    if(boolList.size()>=8)
                    {
                        newError(tr("Procotol wrong or corrupted").toStdString(),
                                 QStringLiteral("bool list too big with sub ident: %1, and queryNumber: %2, type: query_type_protocol")
                                 .arg(mainCodeType).arg(queryNumber).toStdString());
                        return false;
                    }
                    if(!httpModeSub)
                        datapackDownloadFinishedSub();
                    datapackStatus=DatapackStatus::Finished;
                }
                break;
                default:
                return false;
            }
        }
        return true;
        default:
            return Api_protocol::parseReplyData(mainCodeType,queryNumber,data.data(),data.size());
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray data_remaining=QByteArray(data.data(),data.size()).right(data.size()-in.device()->pos());
        parseError(tr("Procotol wrong or corrupted").toStdString(),
                   QStringLiteral("error: remaining data: Api_client_real::parseReplyData(%1,%2,%3): %4")
                   .arg(mainCodeType).arg(queryNumber).arg(QString(data_remaining.toHex()))
                   .toStdString());
        return false;
    }
    return true;
}

//general data
void Api_client_real::defineMaxPlayers(const uint16_t &maxPlayers)
{
    ProtocolParsing::setMaxPlayers(maxPlayers);
}

void Api_client_real::resetAll()
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

    closeDownload();
    Api_protocol::resetAll();
}

void Api_client_real::closeDownload()
{
    for( auto& n : urlInWaitingListBase )
        n.first->abort();
    for( auto& n : urlInWaitingListMain )
        n.first->abort();
    for( auto& n : urlInWaitingListSub )
        n.first->abort();
}

void Api_client_real::tryConnect(std::string host,uint16_t port)
{
    if(socket==NULL)
        return;
    qDebug() << (QStringLiteral("Try connect on: %1:%2").arg(QString::fromStdString(host)).arg(port));
    this->host=host;
    this->port=port;
    socket->connectToHost(QHostAddress(QString::fromStdString(host)),port);
}

/*void Api_client_real::disconnect()
{
    if(stage()==StageConnexion::Stage1 || stage()==StageConnexion::Stage4)
    {
        wait_datapack_content_base=false;
        wait_datapack_content_main=false;
        wait_datapack_content_sub=false;
        resetAll();
    }
}*/

void Api_client_real::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

std::string Api_client_real::getHost()
{
    return host;
}

uint16_t Api_client_real::getPort()
{
    return port;
}

void Api_client_real::sendDatapackContentMainSub(const std::string &hashMain, const std::string &hashSub)
{
    bool mainNeedUpdate=true;
    if(!hashMain.empty())
        if((unsigned int)hashMain.size()==(unsigned int)CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size() &&
                memcmp(hashMain.data(),CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),hashMain.size())==0)
        {
            mainNeedUpdate=false;
        }
    bool subNeedUpdate=true;
    if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.empty() && hashSub.empty())
        subNeedUpdate=false;
    else if(!hashSub.empty())
        if((unsigned int)hashSub.size()==(unsigned int)CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size() &&
                memcmp(hashSub.data(),CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),hashSub.size())==0)
        {
            subNeedUpdate=false;
        }
    if(!mainNeedUpdate && !subNeedUpdate)
    {
        datapackStatus=DatapackStatus::Finished;
        haveTheDatapackMainSub();
        return;
    }

    sendDatapackContentMain();
}

void Api_client_real::setProxy(const QNetworkProxy &proxy)
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
