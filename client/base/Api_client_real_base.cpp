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

void Api_client_real::writeNewFile(const QString &fileName,const QByteArray &data)
{
    const QString &fullPath=mDatapackBase+text_slash+fileName;
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

void Api_client_real::datapackChecksumDoneBase(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList)
{
    if(datapackFilesListBase.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListBase.size()!=partialHash.size():" << datapackFilesListBase.size() << "!=" << partialHashList.size();
        qDebug() << "this->datapackFilesList:" << this->datapackFilesListBase.join("\n");
        qDebug() << "datapackFilesList:" << datapackFilesListBase.join("\n");
        abort();
    }

    this->datapackFilesListBase=datapackFilesList;
    this->partialHashListBase=partialHashList;
    if(!datapackFilesListBase.isEmpty() && hash==CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        haveTheDatapack();
        return;
    }

    if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.isEmpty())
    {
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.isEmpty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        qDebug() << "Datapack is empty or hash don't match, get from server";
        quint8 datapack_content_query_number=queryNumber();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint32)datapackFilesListBase.size();
        int index=0;
        while(index<datapackFilesListBase.size())
        {
            out << datapackFilesListBase.at(index);
            struct stat info;
            stat(QString(mDatapackBase+datapackFilesListBase.at(index)).toLatin1().data(),&info);
            out << (quint32)partialHashList.at(index);
            index++;
        }
        packFullOutcommingQuery(0x02,0x000C,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListBase.isEmpty())
        {
            index_mirror=0;
            test_mirror();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackBase;
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
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
            QNetworkRequest networkRequest(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+QStringLiteral("pack/diff/datapack-%1.tar.xz").arg(QString(hash.toHex())));
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

void Api_client_real::test_mirror_base()
{
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarXz)
    {
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror)+QStringLiteral("pack/datapack.tar.xz"));
        reply = qnam.get(networkRequest);
    }
    else
    {
        if(index_mirror>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/base.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror)+QStringLiteral("datapack-list/base.txt"));
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

void Api_client_real::decodedIsFinishBase()
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
                QFile file(mDatapackBase+fileList.at(index));
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
            wait_datapack_content=false;
            haveTheDatapack();
        }
        else
            test_mirror();
    }
}

bool Api_client_real::mirrorTryNextBase()
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
        if(index_mirror>=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).size())
        {
            newError(tr("Unable to download the datapack"),QStringLiteral("Get the list failed"));
            return false;
        }
        else
            test_mirror();
    }
    return true;
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
            if(datapackFilesListBase.size()!=partialHashListBase.size())
            {
                qDebug() << "datapackFilesListBase.size()!=partialHashList.size(), CRITICAL";
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
                        int indexInDatapackList=datapackFilesListBase.indexOf(fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const quint32 &hashFileOnDisk=partialHashListBase.at(indexInDatapackList);
                            QFileInfo file(mDatapackBase+fileString);
                            if(!file.exists())
                            {
                                getHttpFile(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const quint32 *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                getHttpFile(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                        }
                        else
                        {
                            getHttpFile(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror)+fileString,fileString);
                            fileToGet++;
                            sizeToGet+=sizeString.toUInt();
                        }
                        partialHashListBase.removeAt(indexInDatapackList);
                        datapackFilesListBase.removeAt(indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListBase.size())
            {
                if(!QFile(mDatapackBase+datapackFilesListBase.at(index)).remove())
                {
                    qDebug() << "Unable to remove" << datapackFilesListBase.at(index);
                    abort();
                }
                index++;
            }
            datapackFilesListBase.clear();
            if(fileToGet==0)
                haveTheDatapack();
            else
                datapackSize(fileToGet,sizeToGet*1000);
        }
    }
}

const QStringList Api_client_real::listDatapackBase(QString suffix)
{
    if(suffix.contains(excludePathBase))
        return QStringList();

    QStringList returnFile;
    QDir finalDatapackFolder(mDatapackBase+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapackBase(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if((suffix+fileInfo.fileName()).contains(Api_client_real::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QStringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapackBase+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    returnFile.sort();
    return returnFile;
}

void Api_client_real::cleanDatapackBase(QString suffix)
{
    QDir finalDatapackFolder(mDatapackBase+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackBase(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapackBase+suffix);
}
