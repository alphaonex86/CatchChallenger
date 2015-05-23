#include "Api_client_real.h"

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
#include "qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"

//need host + port here to have datapack base

QString Api_client_real::text_slash=QLatin1Literal("/");
QString Api_client_real::text_dotcoma=QLatin1Literal(";");
QRegularExpression Api_client_real::regex_DATAPACK_FILE_REGEX=QRegularExpression(DATAPACK_FILE_REGEX);
QRegularExpression Api_client_real::excludePathBase("^map[/\\\\]main[/\\\\]");
QRegularExpression Api_client_real::excludePathMain("^sub[/\\\\]");

Api_client_real::Api_client_real(ConnectedSocket *socket,bool tolerantMode) :
    Api_protocol(socket,tolerantMode),
    qnamQueueCount(0),
    qnamQueueCount2(0),
    qnamQueueCount3(0),
    qnamQueueCount4(0)
{
    datapackTarXz=false;
    index_mirror=0;
    index_mirror_main=0;
    index_mirror_sub=0;
    host=QLatin1Literal("localhost");
    port=42489;
    connect(socket, &ConnectedSocket::disconnected,	this,&Api_client_real::disconnected);
    connect(this,   &Api_client_real::newFile,      this,&Api_client_real::writeNewFile);
    connect(this,   &Api_client_real::newHttpFile,  this,&Api_client_real::getHttpFile);
    connect(this,   &Api_client_real::doDifferedChecksumBase,&datapackChecksum,&CatchChallenger::DatapackChecksum::doDifferedChecksumBase);
    connect(this,   &Api_client_real::doDifferedChecksumMain,&datapackChecksum,&CatchChallenger::DatapackChecksum::doDifferedChecksumMain);
    connect(this,   &Api_client_real::doDifferedChecksumSub,&datapackChecksum,&CatchChallenger::DatapackChecksum::doDifferedChecksumSub);
    connect(&datapackChecksum,&CatchChallenger::DatapackChecksum::datapackChecksumDoneBase,this,&Api_client_real::datapackChecksumDoneBase);
    connect(&datapackChecksum,&CatchChallenger::DatapackChecksum::datapackChecksumDoneMain,this,&Api_client_real::datapackChecksumDoneMain);
    connect(&datapackChecksum,&CatchChallenger::DatapackChecksum::datapackChecksumDoneSub,this,&Api_client_real::datapackChecksumDoneSub);
    connect(&xzDecodeThread,&QXzDecodeThread::decodedIsFinish,this,&Api_client_real::decodedIsFinish);
    disconnected();
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
}

void Api_client_real::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    return parseFullReplyData(mainCodeType,subCodeType,queryNumber,QByteArray(data,size));
}

void Api_client_real::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0x02:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Send datapack file list
                case 0x000C:
                    {
                        if(datapackFilesListBase.isEmpty() && data.size()==1)
                        {
                            if(!httpMode)
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
                        if(!httpMode)
                            haveTheDatapack();
                    }
                    return;
                break;
                default:
                    Api_protocol::parseFullReplyData(mainCodeType,subCodeType,queryNumber,data);
                    return;
                break;
            }
        }
        break;
        default:
            Api_protocol::parseFullReplyData(mainCodeType,subCodeType,queryNumber,data);
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray data_remaining=data.right(data.size()-in.device()->pos());
        parseError(tr("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: Api_client_real::parseReplyData(%1,%2,%3): %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data_remaining.toHex())));
        return;
    }
}

//general data
void Api_client_real::defineMaxPlayers(const quint16 &maxPlayers)
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

    Api_protocol::resetAll();
}

void Api_client_real::tryConnect(QString host,quint16 port)
{
    if(socket==NULL)
        return;
    DebugClass::debugConsole(QStringLiteral("Try connect on: %1:%2").arg(host).arg(port));
    this->host=host;
    this->port=port;
    socket->connectToHost(host,port);
}

void Api_client_real::disconnected()
{
    wait_datapack_content=false;
    resetAll();
}

void Api_client_real::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

QString Api_client_real::getHost()
{
    return host;
}

quint16 Api_client_real::getPort()
{
    return port;
}

void Api_client_real::sendDatapackContentMainSub()
{
    if(wait_datapack_content)
    {
        DebugClass::debugConsole(QStringLiteral("already in wait of datapack content"));
        return;
    }

    //compute the mirror
    {
        QStringList values=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
        {
            QString slash(QStringLiteral("/"));
            int index=0;
            while(index<values.size())
            {
                if(!values.at(index).endsWith(slash))
                    values[index]+=slash;
                index++;
            }
        }
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=values.join(Api_client_real::text_dotcoma);
    }

    datapackTarXzMain=false;
    datapackTarXzSub=false;
    wait_datapack_content_main=true;
    wait_datapack_content_sub=true;
    datapackFilesListMain=listDatapackBase(QString());
    datapackFilesListMain.sort();
    datapackFilesListSub=listDatapackBase(QString());
    datapackFilesListSub.sort();
    emit doDifferedChecksumMain(mDatapackMain);
    emit doDifferedChecksumSub(mDatapackSub);
}

void Api_client_real::downloadProgressDatapackMainSub(qint64 bytesReceived, qint64 bytesTotal)
{
    if(!datapackTarXzMain && !datapackTarXzSub)
    {
        if(bytesReceived>0)
            datapackSize(1,bytesTotal);
    }
    emit progressingDatapackFile(bytesReceived);
}

void Api_client_real::httpErrorEvent()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply for http is NULL"));
        socket->disconnectFromHost();
        return;
    }
    qDebug() << reply->url().toString() << reply->errorString();
    mirrorTryNext();
    return;
}

void Api_client_real::setProxy(const QNetworkProxy &proxy)
{
    this->proxy=proxy;
}
