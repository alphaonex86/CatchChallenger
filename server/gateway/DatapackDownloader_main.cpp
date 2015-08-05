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

void DatapackDownloaderMainSub::writeNewFileMain(const QString &fileName,const QByteArray &data)
{
    const QString &fullPath=mDatapackMain+text_slash+fileName;
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

void DatapackDownloaderMainSub::getHttpFileMain(const QString &url, const QString &fileName)
{
    if(httpError)
        return;
    if(!httpModeMain)
        httpModeMain=true;
    QNetworkRequest networkRequest(url);
    QNetworkReply *reply;
    //choice the right queue
    {
        if(qnamQueueCount3 < qnamQueueCount2 && qnamQueueCount3 < qnamQueueCount)
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
        else
            qDebug() << "queue detection bug to add";
    }
    UrlInWaiting urlInWaiting;
    urlInWaiting.fileName=fileName;
    urlInWaitingListMain[reply]=urlInWaiting;
    connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedMain);
}

void DatapackDownloaderMainSub::httpFinishedMain()
{
    if(httpError)
        return;
    if(urlInWaitingListMain.isEmpty())
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
    {
        QNetworkAccessManager * manager=reply->manager();
        if(manager==&qnam)
            qnamQueueCount--;
        else if(manager==&qnam2)
            qnamQueueCount2--;
        else if(manager==&qnam3)
            qnamQueueCount3--;
        else
            qDebug() << "queue detection bug to remove";
    }
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
    if(!urlInWaitingListMain.contains(reply))
    {
        httpError=true;
        qDebug() << (QStringLiteral("reply of unknown query"));
        datapackDownloadError();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingListMain.value(reply);
    writeNewFileMain(urlInWaiting.fileName,reply->readAll());

    if(urlInWaitingListMain.remove(reply)!=1)
        DebugClass::debugConsole(QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListMain.size()));
    reply->deleteLater();
    if(urlInWaitingListMain.isEmpty())
        checkIfContinueOrFinished();
}

void DatapackDownloaderMainSub::datapackChecksumDoneMain(const QStringList &datapackFilesList,const QByteArray &hash,const QList<quint32> &partialHashList)
{
    if(datapackFilesListMain.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListMain.size()!=partialHash.size():" << datapackFilesListMain.size() << "!=" << partialHashList.size();
        qDebug() << "this->datapackFilesList:" << this->datapackFilesListMain.join("\n");
        qDebug() << "datapackFilesList:" << datapackFilesListMain.join("\n");
        abort();
    }
    {
        if(mainDatapackCode=="[main]")
        {
            qDebug() << "mainDatapackCode==[main]";
            abort();
        }
        if(subDatapackCode=="[sub]")
        {
            qDebug() << "subDatapackCode==[sub]";
            abort();
        }
    }

    hashMain=hash;
    this->datapackFilesListMain=datapackFilesList;
    this->partialHashListMain=partialHashList;
    if(!datapackFilesListMain.isEmpty() && hash==sendedHashMain)
    {
        qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote";
        wait_datapack_content_main=false;
        checkIfContinueOrFinished();
        return;
    }

    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.isEmpty())
    {
        if(!QFile(mDatapackBase+QString("/pack/datapack-main-%1.tar.xz").arg(mainDatapackCode)).remove())
        {
            qDebug() << "Unable to remove "+mDatapackBase+QString("/pack/datapack-main-%1.tar.xz").arg(mainDatapackCode);
            return;
        }
        if(sendedHashMain.isEmpty())
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
        out << (quint8)DatapackStatus::Main;
        out << (quint32)datapackFilesListMain.size();
        int index=0;
        while(index<datapackFilesListMain.size())
        {
            const QByteArray &rawFileName=FacilityLibGeneral::toUTF8WithHeader(datapackFilesListMain.at(index));
            if(rawFileName.size()>255 || rawFileName.isEmpty())
            {
                DebugClass::debugConsole(QStringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            outputData+=rawFileName;
            out.device()->seek(out.device()->size());

            /*struct stat info;
            stat(QString(mDatapackBase+datapackFilesListMain.at(index)).toLatin1().data(),&info);*/
            out << (quint32)partialHashList.at(index);
            index++;
        }
        client->packFullOutcommingQuery(0x02,0x0C,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListMain.isEmpty())
        {
            {
                if(mainDatapackCode=="[main]")
                {
                    qDebug() << "mainDatapackCode==[main]";
                    abort();
                }
                if(subDatapackCode=="[sub]")
                {
                    qDebug() << "subDatapackCode==[sub]";
                    abort();
                }
            }
            index_mirror_main=0;
            test_mirror_main();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackMain;
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_main)+QStringLiteral("pack/diff/datapack-main-")+mainDatapackCode+QStringLiteral("-%1.tar.xz").arg(QString(hash.toHex())));
            QNetworkReply *reply = qnam.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListMain);
        }
    }
}

void DatapackDownloaderMainSub::test_mirror_main()
{
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarXzMain)
    {
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("pack/datapack-main-")+mainDatapackCode+QStringLiteral(".tar.xz"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListMain);//fix it, put httpFinished* broke it
    }
    else
    {
        if(index_mirror_main>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/main-XXXX.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("datapack-list/main-")+mainDatapackCode+QStringLiteral(".txt"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &DatapackDownloaderMainSub::httpFinishedForDatapackListMain);
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &DatapackDownloaderMainSub::httpErrorEventMain);
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextMain();
        return;
    }
}

void DatapackDownloaderMainSub::decodedIsFinishMain()
{
    if(xzDecodeThreadMain.errorFound())
        test_mirror_main();
    else
    {
        const QByteArray &decodedData=xzDecodeThreadMain.decodedData();
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
                QFile file(mDatapackMain+fileList.at(index));
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
            wait_datapack_content_main=false;
            checkIfContinueOrFinished();
        }
        else
            test_mirror_main();
    }
}

bool DatapackDownloaderMainSub::mirrorTryNextMain()
{
    if(!datapackTarXzMain)
    {
        datapackTarXzMain=true;
        test_mirror_main();
    }
    else
    {
        datapackTarXzMain=false;
        index_mirror_main++;
        if(index_mirror_main>=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).size())
        {
            qDebug() << (QStringLiteral("Get the list failed"));
            return false;
        }
        else
            test_mirror_main();
    }
    return true;
}

void DatapackDownloaderMainSub::httpFinishedForDatapackListMain()
{
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
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Main Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  );
        else
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Main Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                                                  );
        reply->deleteLater();
        mirrorTryNextMain();
        return;
    }
    else
    {
        if(!datapackTarXzMain)
        {
            qDebug() << "datapack.tar.xz size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarXzMain=true;
            xzDecodeThreadMain.setData(reply->readAll(),100*1024*1024);
            xzDecodeThreadMain.run();
            decodedIsFinishMain();
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
            if(datapackFilesListMain.size()!=partialHashListMain.size())
            {
                qDebug() << "datapackFilesListMain.size()!=partialHashListMain.size(), CRITICAL";
                abort();
                return;
            }
            /*ref crash here*/const QString selectedMirror=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_main)+"map/main/"+mainDatapackCode+"/";
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
                        int indexInDatapackList=datapackFilesListMain.indexOf(fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const quint32 &hashFileOnDisk=partialHashListMain.at(indexInDatapackList);
                            QFileInfo file(mDatapackMain+fileString);
                            if(!file.exists())
                            {
                                getHttpFileMain(selectedMirror+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const quint32 *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                getHttpFileMain(selectedMirror+fileString,fileString);
                                fileToGet++;
                                sizeToGet+=sizeString.toUInt();
                            }
                        }
                        else
                        {
                            getHttpFileMain(selectedMirror+fileString,fileString);
                            fileToGet++;
                            sizeToGet+=sizeString.toUInt();
                        }
                        partialHashListMain.removeAt(indexInDatapackList);
                        datapackFilesListMain.removeAt(indexInDatapackList);
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListMain.size())
            {
                if(!QFile(mDatapackMain+datapackFilesListMain.at(index)).remove())
                {
                    qDebug() << "Unable to remove" << datapackFilesListMain.at(index);
                    abort();
                }
                index++;
            }
            datapackFilesListMain.clear();
            if(correctContent==0)
            {
                qDebug() << "Error, no valid content: correctContent==0\n" << content.join("\n") << "\nFor:" << reply->url().toString();
                abort();
                return;
            }
            if(fileToGet==0)
                checkIfContinueOrFinished();
        }
    }
}

const QStringList DatapackDownloaderMainSub::listDatapackMain(QString suffix)
{
    if(suffix.contains(excludePathMain))
        return QStringList();

    QStringList returnFile;
    QDir finalDatapackFolder(mDatapackMain+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapackMain(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if((suffix+fileInfo.fileName()).contains(DatapackDownloaderMainSub::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QStringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapackMain+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    returnFile.sort();
    return returnFile;
}

void DatapackDownloaderMainSub::cleanDatapackMain(QString suffix)
{
    QDir finalDatapackFolder(mDatapackMain+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackMain(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapackMain+suffix);
}

void DatapackDownloaderMainSub::datapackDownloadFinishedMain()
{

}

void DatapackDownloaderMainSub::httpErrorEventMain()
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
    //mirrorTryNextMain();//mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void DatapackDownloaderMainSub::sendDatapackContentMain()
{
    {
        if(mainDatapackCode=="[main]")
        {
            qDebug() << "mainDatapackCode==[main]";
            abort();
        }
        if(subDatapackCode=="[sub]")
        {
            qDebug() << "subDatapackCode==[sub]";
            abort();
        }
    }
    if(wait_datapack_content_main || wait_datapack_content_sub)
    {
        DebugClass::debugConsole(QStringLiteral("already in wait of datapack content"));
        return;
    }

    //compute the mirror
    {
        QStringList values=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts);
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
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=values.join(DatapackDownloaderMainSub::text_dotcoma);
    }

    datapackTarXzMain=false;
    wait_datapack_content_main=true;
    datapackFilesListMain=listDatapackMain(QString());
    datapackFilesListMain.sort();
    const DatapackChecksum::FullDatapackChecksumReturn &fullDatapackChecksumReturn=DatapackChecksum::doFullSyncChecksumMain(mDatapackMain);
    datapackChecksumDoneMain(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}

void DatapackDownloaderMainSub::checkIfContinueOrFinished()
{
    wait_datapack_content_main=false;
    if(subDatapackCode.isEmpty())
        haveTheDatapackMainSub();
    else
    {
        datapackStatus=DatapackStatus::Sub;
        sendDatapackContentSub();
    }
}
