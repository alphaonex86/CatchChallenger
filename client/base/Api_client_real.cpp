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

#include "../../general/base/CommonSettings.h"
#include "qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"

//need host + port here to have datapack base

QString Api_client_real::text_slash=QLatin1Literal("/");
QString Api_client_real::text_dotcoma=QLatin1Literal(";");
QRegularExpression Api_client_real::regex_DATAPACK_FILE_REGEX=QRegularExpression(DATAPACK_FILE_REGEX);

Api_client_real::Api_client_real(ConnectedSocket *socket,bool tolerantMode) :
    Api_protocol(socket,tolerantMode),
    qnamQueueCount(0),
    qnamQueueCount2(0),
    qnamQueueCount3(0),
    qnamQueueCount4(0)
{
    datapackTarXz=false;
    index_mirror=0;
    test_with_proxy=true;
    host=QLatin1Literal("localhost");
    port=42489;
    connect(socket, &ConnectedSocket::disconnected,	this,&Api_client_real::disconnected);
    connect(this,   &Api_client_real::newFile,      this,&Api_client_real::writeNewFile);
    connect(this,   &Api_client_real::newHttpFile,  this,&Api_client_real::getHttpFile);
    connect(this,   &Api_client_real::doDifferedChecksum,&datapackChecksum,&CatchChallenger::DatapackChecksum::doDifferedChecksum);
    connect(&datapackChecksum,&CatchChallenger::DatapackChecksum::datapackChecksumDone,this,&Api_client_real::datapackChecksumDone);
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

void Api_client_real::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
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
                        if(datapackFilesList.isEmpty() && data.size()==1)
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
                        if(boolList.size()<datapackFilesList.size())
                        {
                            newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too small with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        int index=0;
                        while(index<datapackFilesList.size())
                        {
                            if(boolList.first())
                            {
                                DebugClass::debugConsole(QStringLiteral("remove the file: %1").arg(mDatapack+text_slash+datapackFilesList.at(index)));
                                QFile file(mDatapack+text_slash+datapackFilesList.at(index));
                                if(!file.remove())
                                    DebugClass::debugConsole(QStringLiteral("unable to remove the file: %1: %2").arg(datapackFilesList.at(index)).arg(file.errorString()));
                                //removeFile(datapackFilesList.at(index));
                            }
                            boolList.removeFirst();
                            index++;
                        }
                        datapackFilesList.clear();
                        cleanDatapack(QString());
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
    httpMode=false;
    httpError=false;
    RXSize=0;
    TXSize=0;
    query_files_list.clear();
    urlInWaitingList.clear();
    wait_datapack_content=false;

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

void Api_client_real::writeNewFile(const QString &fileName,const QByteArray &data)
{
    const QString &fullPath=mDatapack+text_slash+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

        if(file.exists())
            if(!file.remove())
            {
                DebugClass::debugConsole(QStringLiteral("Can't remove: %1: %2").arg(fileName).arg(file.errorString()));
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            DebugClass::debugConsole(QStringLiteral("Can't open: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        if(file.write(data)!=data.size())
        {
            file.close();
            DebugClass::debugConsole(QStringLiteral("Can't write: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        file.flush();
        file.close();
    }

    //send size
    {
        if(httpMode)
            newDatapackFile(ceil((float)data.size()/1000)*1000);
        else
            newDatapackFile(data.size());
    }
}

void Api_client_real::getHttpFile(const QString &url, const QString &fileName)
{
    if(httpError)
        return;
    if(!httpMode)
        httpMode=true;
    QNetworkRequest networkRequest(url);
    QNetworkReply *reply;
    //choice the right queue
    {
        if(qnamQueueCount4 < qnamQueueCount3 && qnamQueueCount4 < qnamQueueCount2 && qnamQueueCount4 < qnamQueueCount)
            reply = qnam4.get(networkRequest);
        else if(qnamQueueCount3 < qnamQueueCount2 && qnamQueueCount3 < qnamQueueCount)
            reply = qnam3.get(networkRequest);
        else if(qnamQueueCount2 < qnamQueueCount)
            reply = qnam2.get(networkRequest);
        else
            reply = qnam.get(networkRequest);
    }
    //add to queue count
    {
        QNetworkAccessManager * manager=reply->manager();
        if(manager==&qnam)
            qnamQueueCount++;
        else if(manager==&qnam2)
            qnamQueueCount2++;
        else if(manager==&qnam3)
            qnamQueueCount3++;
        else if(manager==&qnam4)
            qnamQueueCount4++;
        else
            qDebug() << "queue detection bug to add";
    }
    UrlInWaiting urlInWaiting;
    urlInWaiting.fileName=fileName;
    urlInWaitingList[reply]=urlInWaiting;
    connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinished);
}

void Api_client_real::httpFinished()
{
    if(httpError)
        return;
    if(urlInWaitingList.isEmpty())
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("no more reply in waiting"));
        socket->disconnectFromHost();
        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply for http is NULL"));
        socket->disconnectFromHost();
        return;
    }
    //remove to queue count
    {
        QNetworkAccessManager * manager=reply->manager();
        if(manager==&qnam)
            qnamQueueCount--;
        else if(manager==&qnam2)
            qnamQueueCount2--;
        else if(manager==&qnam3)
            qnamQueueCount3--;
        else if(manager==&qnam4)
            qnamQueueCount4--;
        else
            qDebug() << "queue detection bug to remove";
    }
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished())
    {
        httpError=true;
        newError(tr("Unable to download the datapack"),QStringLiteral("get the new update failed: not finished"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    else if(reply->error())
    {
        httpError=true;
        newError(tr("Unable to download the datapack"),QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    } else if(!redirectionTarget.isNull()) {
        httpError=true;
        newError(tr("Unable to download the datapack"),QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    if(!urlInWaitingList.contains(reply))
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply of unknown query"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingList.value(reply);
    writeNewFile(urlInWaiting.fileName,reply->readAll());

    if(urlInWaitingList.remove(reply)!=1)
        DebugClass::debugConsole(QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingList.size()));
    reply->deleteLater();
    if(urlInWaitingList.isEmpty())
        haveTheDatapack();
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

void Api_client_real::sendDatapackContent()
{
    if(wait_datapack_content)
    {
        DebugClass::debugConsole(QStringLiteral("already in wait of datapack content"));
        return;
    }

    //compute the mirror
    {
        QStringList values=CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
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
        CommonSettings::commonSettings.httpDatapackMirror=values.join(Api_client_real::text_dotcoma);
    }

    datapackTarXz=false;
    wait_datapack_content=true;
    datapackFilesList=listDatapack(QString());
    datapackFilesList.sort();
    emit doDifferedChecksum(mDatapack);
}

void Api_client_real::datapackChecksumDone(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList)
{
    if(datapackFilesList.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesList.size()!=partialHash.size():" << datapackFilesList.size() << "!=" << partialHashList.size();
        qDebug() << "this->datapackFilesList:" << this->datapackFilesList.join("\n");
        qDebug() << "datapackFilesList:" << datapackFilesList.join("\n");
        abort();
    }

    this->datapackFilesList=datapackFilesList;
    this->partialHashList=partialHashList;
    if(!datapackFilesList.isEmpty() && hash==CommonSettings::commonSettings.datapackHash)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        haveTheDatapack();
        return;
    }

    if(CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
    {
        if(CommonSettings::commonSettings.datapackHash.isEmpty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        qDebug() << "Datapack is empty or hash don't match, get from server";
        quint8 datapack_content_query_number=queryNumber();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint32)datapackFilesList.size();
        int index=0;
        while(index<datapackFilesList.size())
        {
            out << datapackFilesList.at(index);
            struct stat info;
            stat(QString(mDatapack+datapackFilesList.at(index)).toLatin1().data(),&info);
            out << (quint32)partialHashList.at(index);
            index++;
        }
        packFullOutcommingQuery(0x02,0x000C,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesList.isEmpty())
        {
            index_mirror=0;
            test_with_proxy=(proxy.type()==QNetworkProxy::Socks5Proxy);
            test_mirror();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapack;
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
            if(test_with_proxy)
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
            QNetworkRequest networkRequest(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+QStringLiteral("pack/diff/datapack-%1.tar.xz").arg(QString(hash.toHex())));
            QNetworkReply *reply = qnam.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackList);
            connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgress);
        }
    }
}

void Api_client_real::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if(!datapackTarXz)
    {
        if(bytesReceived>0)
            datapackSize(1,bytesTotal);
    }
    emit progressingDatapackFile(bytesReceived);
}

void Api_client_real::test_mirror()
{
    if(test_with_proxy)
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
    QNetworkReply *reply;
    if(!datapackTarXz)
    {
        QNetworkRequest networkRequest(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+QStringLiteral("pack/datapack.tar.xz"));
        reply = qnam.get(networkRequest);
    }
    else
    {
        QNetworkRequest networkRequest(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+QStringLiteral("datapack-list.txt"));
        reply = qnam.get(networkRequest);
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackList);
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Api_client_real::httpErrorEvent);
        connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgress);
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNext();
        return;
    }
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

void Api_client_real::decodedIsFinish()
{
    if(xzDecodeThread.errorFound())
        test_mirror();
    else
    {
        const QByteArray &decodedData=xzDecodeThread.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QSet<QString> extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(Api_client_real::text_dotcoma).toSet();
            QDir dir;
            const QStringList &fileList=tarDecode.getFileList();
            const QList<QByteArray> &dataList=tarDecode.getDataList();
            int index=0;
            while(index<fileList.size())
            {
                QFile file(mDatapack+fileList.at(index));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.contains(fileInfo.suffix()))
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index));
                        file.close();
                    }
                    else
                    {
                        newError(tr("Disk error"),QStringLiteral("unable to write file of datapack %1: %2").arg(file.fileName()).arg(file.errorString()));
                        return;
                    }
                }
                else
                {
                    newError(tr("Security error, file not allowed: %1").arg(file.fileName()),QStringLiteral("file not allowed: %1").arg(file.fileName()));
                    return;
                }
                index++;
            }
            haveTheDatapack();
        }
        else
            test_mirror();
    }
}

void Api_client_real::mirrorTryNext()
{
    if(!datapackTarXz)
    {
        datapackTarXz=true;
        test_mirror();
    }
    else
    {
        datapackTarXz=false;
        index_mirror++;
        if(index_mirror>=CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).size())
        {
            if(test_with_proxy)
            {
                test_with_proxy=false;
                index_mirror=0;
            }
            else
                newError(tr("Unable to download the datapack"),QStringLiteral("Get the list failed"));
        }
        else
            test_mirror();
    }
}

void Api_client_real::httpFinishedForDatapackList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply for http is NULL"));
        socket->disconnectFromHost();
        return;
    }
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished() || reply->error() || !redirectionTarget.isNull())
    {
        const QNetworkProxy &proxy=qnam.proxy();
        if(proxy==QNetworkProxy::NoProxy)
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  );
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                                                  );
        reply->deleteLater();
        mirrorTryNext();
        return;
    }
    else
    {
        if(!datapackTarXz)
        {
            qDebug() << "datapack.tar.xz size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarXz=true;
            xzDecodeThread.setData(reply->readAll(),100*1024*1024);
            xzDecodeThread.start(QThread::LowestPriority);
            return;
        }
        else
        {
            int sizeToGet=0;
            int fileToGet=0;
            httpError=false;
            const QStringList &content=QString::fromUtf8(reply->readAll()).split(QRegularExpression("[\n\r]+"));
            int index=0;
            QRegularExpression splitReg("^(.*) (([0-9a-f][0-9a-f])+) ([0-9]+)$");
            QRegularExpression fileMatchReg(DATAPACK_FILE_REGEX);
            if(datapackFilesList.size()!=partialHashList.size())
            {
                qDebug() << "datapackFilesList.size()!=partialHashList.size(), CRITICAL";
                abort();
                return;
            }
            while(index<content.size())
            {
                if(content.at(index).contains(splitReg))
                {
                    QString fileString=content.at(index);
                    QString partialHashString=content.at(index);
                    QString sizeString=content.at(index);
                    fileString.replace(splitReg,"\\1");
                    partialHashString.replace(splitReg,"\\2");
                    sizeString.replace(splitReg,"\\4");
                    if(fileString.contains(fileMatchReg))
                    {
                        int indexInDatapackList=datapackFilesList.indexOf(fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const quint32 &hashFileOnDisk=partialHashList.at(indexInDatapackList);
                            QFileInfo file(mDatapack+fileString);
                            if(!file.exists())
                            {
                                getHttpFile(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const quint32 *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                getHttpFile(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                        }
                        else
                        {
                            getHttpFile(CommonSettings::commonSettings.httpDatapackMirror.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                            fileToGet++;
                            sizeToGet+=sizeString.toUInt();
                        }
                        partialHashList.removeAt(indexInDatapackList);
                        datapackFilesList.removeAt(indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesList.size())
            {
                if(!QFile(mDatapack+datapackFilesList.at(index)).remove())
                {
                    qDebug() << "Unable to remove" << datapackFilesList.at(index);
                    abort();
                }
                index++;
            }
            datapackFilesList.clear();
            if(fileToGet==0)
                haveTheDatapack();
            else
                datapackSize(fileToGet,sizeToGet*1000);
        }
    }
}

const QStringList Api_client_real::listDatapack(QString suffix)
{
    QStringList returnFile;
    QDir finalDatapackFolder(mDatapack+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapack(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if((suffix+fileInfo.fileName()).contains(Api_client_real::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QStringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapack+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    returnFile.sort();
    return returnFile;
}

void Api_client_real::cleanDatapack(QString suffix)
{
    QDir finalDatapackFolder(mDatapack+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapack(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapack+suffix);
}

void Api_client_real::setProxy(const QNetworkProxy &proxy)
{
    this->proxy=proxy;
}
