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
#include "../../general/base/FacilityLibGeneral.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "LinkToGameServer.h"

void DatapackDownloaderMainSub::writeNewFileSub(const QString &fileName,const QByteArray &data)
{
    const QString &fullPath=mDatapackSub+text_slash+fileName;
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
}

void DatapackDownloaderMainSub::getHttpFileSub(const QString &url, const QString &fileName)
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(httpError)
        return;
    if(!httpModeSub)
        httpModeSub=true;
    QNetworkRequest networkRequest(url);
    QNetworkReply *reply;
    //choice the right queue
    reply = qnam4.get(networkRequest);
    //add to queue count
    qnamQueueCount4++;
    UrlInWaiting urlInWaiting;
    urlInWaiting.fileName=fileName;
    urlInWaitingListSub[reply]=urlInWaiting;
    connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedSub);
}

void DatapackDownloaderMainSub::datapackDownloadFinishedSub()
{
    haveTheDatapackMainSub();
    resetAll();
}

void DatapackDownloaderMainSub::httpFinishedSub()
{
    if(httpError)
        return;
    if(urlInWaitingListSub.isEmpty())
    {
        httpError=true;
        qDebug() << (QStringLiteral("no more reply in waiting"));
        datapackDownloadError();
        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        qDebug() << (QStringLiteral("reply for http is NULL"));
        datapackDownloadError();
        return;
    }
    //remove to queue count
    qnamQueueCount4--;
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished())
    {
        httpError=true;
        qDebug() << (QStringLiteral("get the new update failed: not finished"));
        datapackDownloadError();
        reply->deleteLater();
        return;
    }
    else if(reply->error())
    {
        httpError=true;
        qDebug() << (QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        datapackDownloadError();
        reply->deleteLater();
        return;
    } else if(!redirectionTarget.isNull()) {
        httpError=true;
        qDebug() << (QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        datapackDownloadError();
        reply->deleteLater();
        return;
    }
    if(!urlInWaitingListSub.contains(reply))
    {
        httpError=true;
        qDebug() << (QStringLiteral("reply of unknown query"));
        datapackDownloadError();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingListSub.value(reply);
    writeNewFileSub(urlInWaiting.fileName,reply->readAll());

    if(urlInWaitingListSub.remove(reply)!=1)
        DebugClass::debugConsole(QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListSub.size()));
    reply->deleteLater();
    if(urlInWaitingListSub.isEmpty() && !wait_datapack_content_main)
        datapackDownloadFinishedSub();
}

void DatapackDownloaderMainSub::datapackChecksumDoneSub(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList)
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }

    if(datapackFilesListSub.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListSub.size()!=partialHash.size():" << datapackFilesListSub.size() << "!=" << partialHashList.size();
        qDebug() << "this->datapackFilesList:" << this->datapackFilesListSub.join("\n");
        qDebug() << "datapackFilesList:" << datapackFilesListSub.join("\n");
        abort();
    }

    hashSub=hash;
    this->datapackFilesListSub=datapackFilesList;
    this->partialHashListSub=partialHashList;
    if(!datapackFilesListSub.isEmpty() && hash==sendedHashSub)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        wait_datapack_content_sub=false;
        if(!wait_datapack_content_main && !wait_datapack_content_sub)
            datapackDownloadFinishedSub();
        return;
    }

    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.isEmpty())
    {
        if(sendedHashSub.isEmpty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        quint8 datapack_content_query_number=0;
        LinkToGameServer * client=NULL;
        {
            unsigned int indexForClient=0;
            while(indexForClient<clientInSuspend.size())
            {
                if(clientInSuspend.at(indexForClient)!=NULL)
                {
                    client=static_cast<LinkToGameServer *>(clientInSuspend.at(0));
                    datapack_content_query_number=client->freeQueryNumberToServer();
                    break;
                }
                indexForClient++;
            }
            if(indexForClient>=clientInSuspend.size())
            {
                qDebug() << "no client in suspend to do the query to do in protocol datapack download";
                resetAll();
                return;//need CommonSettings::commonSettings.datapackHash send by the server
            }
        }
        qDebug() << "Datapack is empty or hash don't match, get from server, hash local: " << QString(hash.toHex()) << ", hash on server: " << QString(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.toHex());
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)DatapackStatus::Sub;
        out << (quint32)datapackFilesListSub.size();
        int index=0;
        while(index<datapackFilesListSub.size())
        {
            const QByteArray &rawFileName=FacilityLibGeneral::toUTF8WithHeader(datapackFilesListSub.at(index));
            if(rawFileName.size()>255 || rawFileName.isEmpty())
            {
                DebugClass::debugConsole(QStringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            outputData+=rawFileName;
            out.device()->seek(out.device()->size());

            /*struct stat info;
            stat(QString(mDatapackBase+datapackFilesListSub.at(index)).toLatin1().data(),&info);*/
            out << (quint32)partialHashList.at(index);
            index++;
        }
        client->packFullOutcommingQuery(0x02,0x0C,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListSub.isEmpty())
        {
            index_mirror_sub=0;
            test_mirror_sub();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackSub;
        }
        else
        {
            if(subDatapackCode.isEmpty())
            {
                qDebug() << "subDatapackCode.isEmpty() to get from mirror";
                abort();
            }
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_sub)+QStringLiteral("pack/diff/datapack-sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral("-%1.tar.xz").arg(QString(hash.toHex())));
            QNetworkReply *reply = qnam4.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListSub);
        }
    }
}

void DatapackDownloaderMainSub::test_mirror_sub()
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarXzSub)
    {
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("pack/datapack-sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral(".tar.xz"));
        reply = qnam4.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListSub);//fix it, put httpFinished* broke it
    }
    else
    {
        if(index_mirror_sub>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/sub-XXXXX-YYYYYY.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("datapack-list/sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral(".txt"));
        reply = qnam4.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListSub);
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &DatapackDownloaderMainSub::httpErrorEventSub);
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextSub();
        return;
    }
}

void DatapackDownloaderMainSub::decodedIsFinishSub()
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(xzDecodeThreadSub.errorFound())
        test_mirror_sub();
    else
    {
        const QByteArray &decodedData=xzDecodeThreadSub.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QSet<QString> extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(DatapackDownloaderMainSub::text_dotcoma).toSet();
            QDir dir;
            const QStringList &fileList=tarDecode.getFileList();
            const QList<QByteArray> &dataList=tarDecode.getDataList();
            int index=0;
            while(index<fileList.size())
            {
                QFile file(mDatapackSub+fileList.at(index));
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
                        qDebug() << (QStringLiteral("unable to write file of datapack %1: %2").arg(file.fileName()).arg(file.errorString()));
                        return;
                    }
                }
                else
                {
                    qDebug() << (QStringLiteral("file not allowed: %1").arg(file.fileName()));
                    return;
                }
                index++;
            }
            wait_datapack_content_sub=false;
            if(!wait_datapack_content_main && !wait_datapack_content_sub)
                datapackDownloadFinishedSub();
        }
        else
            test_mirror_sub();
    }
}

bool DatapackDownloaderMainSub::mirrorTryNextSub()
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(!datapackTarXzSub)
    {
        datapackTarXzSub=true;
        test_mirror_sub();
    }
    else
    {
        datapackTarXzSub=false;
        index_mirror_sub++;
        if(index_mirror_sub>=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).size())
        {
            qDebug() << (QStringLiteral("Get the list failed"));
            return false;
        }
        else
            test_mirror_sub();
    }
    return true;
}

void DatapackDownloaderMainSub::httpFinishedForDatapackListSub()
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        qDebug() << (QStringLiteral("reply for http is NULL"));
        datapackDownloadError();
        return;
    }
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished() || reply->error() || !redirectionTarget.isNull())
    {
        const QNetworkProxy &proxy=qnam.proxy();
        if(proxy==QNetworkProxy::NoProxy)
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Sub Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  );
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Sub Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                                                  );
        reply->deleteLater();
        mirrorTryNextSub();
        return;
    }
    else
    {
        if(!datapackTarXzSub)
        {
            qDebug() << "datapack.tar.xz size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarXzSub=true;
            xzDecodeThreadSub.setData(reply->readAll(),100*1024*1024);
            xzDecodeThreadSub.run();
            decodedIsFinishSub();
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
            if(datapackFilesListSub.size()!=partialHashListSub.size())
            {
                qDebug() << "datapackFilesListSub.size()!=partialHashListSub.size(), CRITICAL";
                abort();
                return;
            }
            /*ref crash here*/const QString selectedMirror=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_sub)+"map/main/"+mainDatapackCode+"/sub/"+subDatapackCode+"/";
            int correctContent=0;
            while(index<content.size())
            {
                if(content.at(index).contains(splitReg))
                {
                    correctContent++;
                    QString fileString=content.at(index);
                    QString partialHashString=content.at(index);
                    QString sizeString=content.at(index);
                    fileString.replace(splitReg,"\\1");
                    partialHashString.replace(splitReg,"\\2");
                    sizeString.replace(splitReg,"\\4");
                    if(fileString.contains(fileMatchReg))
                    {
                        int indexInDatapackList=datapackFilesListSub.indexOf(fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const quint32 &hashFileOnDisk=partialHashListSub.at(indexInDatapackList);
                            QFileInfo file(mDatapackSub+fileString);
                            if(!file.exists())
                            {
                                getHttpFileSub(selectedMirror+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const quint32 *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                getHttpFileSub(selectedMirror+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                        }
                        else
                        {
                            getHttpFileSub(selectedMirror+fileString,fileString);
                            fileToGet++;
                            sizeToGet+=sizeString.toUInt();
                        }
                        partialHashListSub.removeAt(indexInDatapackList);
                        datapackFilesListSub.removeAt(indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListSub.size())
            {
                if(!QFile(mDatapackSub+datapackFilesListSub.at(index)).remove())
                {
                    qDebug() << "Unable to remove" << datapackFilesListSub.at(index);
                    abort();
                }
                index++;
            }
            datapackFilesListSub.clear();
            if(correctContent==0)
                qDebug() << "Error, no valid content: correctContent==0\n" << content.join("\n") << "\nFor:" << reply->url().toString();
            if(fileToGet==0)
                datapackDownloadFinishedSub();
        }
    }
}

const QStringList DatapackDownloaderMainSub::listDatapackSub(QString suffix)
{
    QStringList returnFile;
    QDir finalDatapackFolder(mDatapackSub+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapackSub(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if((suffix+fileInfo.fileName()).contains(DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QStringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapackSub+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    returnFile.sort();
    return returnFile;
}

void DatapackDownloaderMainSub::cleanDatapackSub(QString suffix)
{
    QDir finalDatapackFolder(mDatapackSub+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackSub(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapackSub+suffix);
}

void DatapackDownloaderMainSub::httpErrorEventSub()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        qDebug() << (QStringLiteral("reply for http is NULL"));
        datapackDownloadError();
        return;
    }
    qDebug() << reply->url().toString() << reply->errorString();
    //mirrorTryNextSub();//mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void DatapackDownloaderMainSub::sendDatapackContentSub()
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(wait_datapack_content_sub)
    {
        DebugClass::debugConsole(QStringLiteral("already in wait of datapack content"));
        return;
    }

    datapackTarXzSub=false;
    wait_datapack_content_sub=true;
    datapackFilesListSub=listDatapackSub(QString());
    datapackFilesListSub.sort();
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumSub(mDatapackSub);
    datapackChecksumDoneSub(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
