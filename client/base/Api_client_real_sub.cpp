#include "Api_client_real.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include <iostream>
#include <cmath>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QDataStream>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"

void Api_client_real::writeNewFileSub(const QString &fileName,const QByteArray &data)
{
    if(mDatapackSub.isEmpty())
        abort();
    const QString &fullPath=mDatapackSub+text_slash+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

        if(file.exists())
            if(!file.remove())
            {
                qDebug() << (QStringLiteral("Can't remove: %1: %2").arg(fileName).arg(file.errorString()));
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            qDebug() << (QStringLiteral("Can't open: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        if(file.write(data)!=data.size())
        {
            file.close();
            qDebug() << (QStringLiteral("Can't write: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        file.flush();
        file.close();
    }

    //send size
    {
        if(httpModeSub)
            newDatapackFileSub(ceil((double)data.size()/1000)*1000);
        else
            newDatapackFileSub(data.size());
    }
}

bool Api_client_real::getHttpFileSub(const QString &url, const QString &fileName)
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(httpError)
        return false;
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
    connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedSub);
    return true;
}

void Api_client_real::datapackDownloadFinishedSub()
{
    haveTheDatapackMainSub();
    datapackStatus=DatapackStatus::Finished;
}

void Api_client_real::httpFinishedSub()
{
    if(httpError)
        return;
    if(urlInWaitingListSub.isEmpty())
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
    qnamQueueCount4--;
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished())
    {
        httpError=true;
        newError(tr("Unable to download the datapack")+"<br />Details:<br />"+reply->url().toString()+"<br />get the new update failed: not finished",QStringLiteral("get the new update failed: not finished"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    else if(reply->error())
    {
        httpError=true;
        newError(tr("Unable to download the datapack")+"<br />Details:<br />"+reply->url().toString()+"<br />"+QStringLiteral("get the new update failed: %1").arg(reply->errorString()),QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    } else if(!redirectionTarget.isNull()) {
        httpError=true;
        newError(tr("Unable to download the datapack")+"<br />Details:<br />"+reply->url().toString()+"<br />"+QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()),QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    if(!urlInWaitingListSub.contains(reply))
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply of unknown query"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingListSub.value(reply);
    writeNewFileSub(urlInWaiting.fileName,reply->readAll());

    if(urlInWaitingListSub.remove(reply)!=1)
        qDebug() << (QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListSub.size()));
    reply->deleteLater();
    if(urlInWaitingListSub.isEmpty() && !wait_datapack_content_main)
        datapackDownloadFinishedSub();
}

void Api_client_real::datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList, const std::vector<char> &hash, const std::vector<uint32_t> &partialHashList)
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }

    if((uint32_t)datapackFilesListSub.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListSub.size()!=partialHash.size():" << datapackFilesListSub.size() << "!=" << partialHashList.size();
        qDebug() << "datapackFilesListSub:" << QString::fromStdString(stringimplode(datapackFilesListSub,'\n'));
        qDebug() << "datapackFilesList:" << QString::fromStdString(stringimplode(datapackFilesList,'\n'));
        abort();
    }

    this->datapackFilesListSub=datapackFilesList;
    this->partialHashListSub=partialHashList;
    if(!datapackFilesListSub.empty() && hash==CommonSettingsServer::commonSettingsServer.datapackHashServerSub)
    {
        std::cerr << "Sub: Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << binarytoHexa(hash) << std::endl;
        wait_datapack_content_sub=false;
        if(!wait_datapack_content_main && !wait_datapack_content_sub)
            datapackDownloadFinishedSub();
        return;
    }

    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
    {
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.empty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        qDebug() << "Sub: Datapack is empty or hash don't match, get from server, hash local: " << QString::fromStdString(binarytoHexa(hash)) << ", hash on server: " << QString::fromStdString(binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub));
        const uint8_t datapack_content_query_number=queryNumber();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)DatapackStatus::Sub;
        out << (uint32_t)datapackFilesListSub.size();
        unsigned int index=0;
        while(index<datapackFilesListSub.size())
        {
            if(datapackFilesListSub.at(index).size()>254 || datapackFilesListSub.at(index).empty())
            {
                qDebug() << (QStringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            out << (uint8_t)datapackFilesListSub.at(index).size();
            outputData+=QByteArray(datapackFilesListSub.at(index).c_str(),static_cast<uint32_t>(datapackFilesListSub.at(index).size()));
            out.device()->seek(out.device()->size());

            index++;
        }
        index=0;
        while(index<datapackFilesListSub.size())
        {
            /*struct stat info;
            stat(QString(mDatapackBase+datapackFilesListSub.at(index)).toLatin1().data(),&info);*/
            out << (uint32_t)partialHashList.at(index);

            index++;
        }
        packOutcommingQuery(0xA1,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListSub.empty())
        {
            index_mirror_sub=0;
            test_mirror_sub();
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackSub;
        }
        else
        {
            if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            {
                qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
                abort();
            }
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer)
                                           .split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_sub)+
                        QStringLiteral("pack/diff/datapack-sub-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+
                        QStringLiteral("-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+
                        QStringLiteral("-%1.tar.xz").arg(QString::fromStdString(binarytoHexa(hash)))
                        );
            QNetworkReply *reply = qnam4.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListSub);
            connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackSub);
        }
    }
}

void Api_client_real::test_mirror_sub()
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer).split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarXzSub)
    {
        QString fullDatapack=httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("pack/datapack-sub-")+
                QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+
                QStringLiteral("-")+
                QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+
                QStringLiteral(".tar.xz");
        qDebug() << "Try download: " << fullDatapack;
        QNetworkRequest networkRequest(fullDatapack);
        reply = qnam4.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListSub);//fix it, put httpFinished* broke it
    }
    else
    {
        if(index_mirror_sub>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/sub-XXXXX-YYYYYY.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("datapack-list/sub-")+
                                       QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+
                                       QStringLiteral("-")+
                                       QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+
                                       QStringLiteral(".txt"));
        reply = qnam4.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListSub);
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Api_client_real::httpErrorEventSub);
        connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackSub);
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextSub(reply->url().toString()+": "+reply->errorString());
        return;
    }
}

void Api_client_real::decodedIsFinishSub()
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(xzDecodeThreadSub.errorFound())
        test_mirror_sub();
    else
    {
        const std::vector<char> &decodedData=xzDecodeThreadSub.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QSet<QString> extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(Api_client_real::text_dotcoma).toSet();
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                QFile file(mDatapackSub+QString::fromStdString(fileList.at(index)));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.contains(fileInfo.suffix()))
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index).data(),dataList.at(index).size());
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
            wait_datapack_content_sub=false;
            if(!wait_datapack_content_main && !wait_datapack_content_sub)
                datapackDownloadFinishedSub();
        }
        else
            test_mirror_sub();
    }
}

bool Api_client_real::mirrorTryNextSub(const QString &error)
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
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
        if(index_mirror_sub>=QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer).split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).size())
        {
            newError(tr("Unable to download the datapack")+"<br />Details:<br />"+error,QStringLiteral("Get the list failed: ")+error);
            return false;
        }
        else
            test_mirror_sub();
    }
    return true;
}

void Api_client_real::httpFinishedForDatapackListSub()
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
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
        QString errorString;
        if(proxy==QNetworkProxy::NoProxy)
            errorString=(QStringLiteral("Sub Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  );
        else
            errorString=(QStringLiteral("Sub Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                                                  );
        qDebug() << errorString;
        reply->deleteLater();
        mirrorTryNextSub(errorString);
        return;
    }
    else
    {
        if(!datapackTarXzSub)
        {
            qDebug() << QStringLiteral("pack/datapack-sub-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+
                        QStringLiteral("-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+
                        QStringLiteral(".tar.xz") << " size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarXzSub=true;
            QByteArray olddata=reply->readAll();
            std::vector<char> newdata;
            newdata.resize(olddata.size());
            memcpy(newdata.data(),olddata.constData(),olddata.size());
            xzDecodeThreadSub.setData(newdata,100*1024*1024);
            xzDecodeThreadSub.start(QThread::LowestPriority);
            return;
        }
        else
        {
            int sizeToGet=0;
            int fileToGet=0;
            std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
            /*ref crash here*/const std::string selectedMirror=stringsplit(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,';').at(index_mirror_main)+
                    "map/main/"+
                    CommonSettingsServer::commonSettingsServer.mainDatapackCode+
                    "/sub/"+
                    CommonSettingsServer::commonSettingsServer.subDatapackCode+
                    "/";
            std::vector<char> data;
            QByteArray olddata=reply->readAll();
            data.resize(olddata.size());
            memcpy(data.data(),olddata.constData(),olddata.size());

            httpError=false;

            size_t endOfText;
            {
                std::string text(data.data(),data.size());
                endOfText=text.find("\n-\n");
            }
            if(endOfText==std::string::npos)
            {
                std::cerr << "no text delimitor into file list: " << reply->url().toString().toStdString() << std::endl;
                newError("Wrong datapack format","no text delimitor into file list: "+reply->url().toString());
                return;
            }
            std::vector<std::string> content;
            std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend());
            {
                if(partialHashListRaw.size()%4!=0)
                {
                    std::cerr << "partialHashList not divisible by 4" << std::endl;
                    newError("Wrong datapack format","partialHashList not divisible by 4");
                    return;
                }
                {
                    std::string text(data.data(),endOfText);
                    content=stringsplit(text,'\n');
                }
                if(partialHashListRaw.size()/4!=content.size())
                {
                    std::cerr << "partialHashList/4!=content.size()" << std::endl;
                    newError("Wrong datapack format","partialHashList/4!=content.size()");
                    return;
                }
            }

            unsigned int correctContent=0;
            unsigned int index=0;
            while(index<content.size())
            {
                const std::string &line=content.at(index);
                size_t const &found=line.find(' ');
                if(found!=std::string::npos)
                {
                    correctContent++;
                    const std::string &fileString=line.substr(0,found);
                    sizeToGet+=stringtouint8(line.substr(found+1,(line.size()-1-found)));
                    const uint32_t &partialHashString=*reinterpret_cast<uint32_t *>(partialHashListRaw.data()+index*4);
                    //const std::string &sizeString=line.substr(found+1,line.size()-found-1);
                    if(regex_search(fileString,datapack_rightFileName))
                    {
                        int indexInDatapackList=vectorindexOf(datapackFilesListSub,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListSub.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(mDatapackSub.toStdString()+fileString))
                            {
                                fileToGet++;
                                if(!getHttpFileSub(QString::fromStdString(selectedMirror+fileString),QString::fromStdString(fileString)))
                                {
                                    newError(tr("Unable to get datapack file"),"");
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                fileToGet++;
                                if(!getHttpFileSub(QString::fromStdString(selectedMirror+fileString),QString::fromStdString(fileString)))
                                {
                                    newError(tr("Unable to get datapack file"),"");
                                    return;
                                }
                            }
                            partialHashListSub.erase(partialHashListSub.cbegin()+indexInDatapackList);
                            datapackFilesListSub.erase(datapackFilesListSub.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            fileToGet++;
                            if(!getHttpFileSub(QString::fromStdString(selectedMirror+fileString),QString::fromStdString(fileString)))
                            {
                                newError(tr("Unable to get datapack file"),"");
                                return;
                            }
                        }
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListSub.size())
            {
                if(!QFile(mDatapackSub+QString::fromStdString(datapackFilesListSub.at(index))).remove())
                {
                    qDebug() << "Unable to remove" << QString::fromStdString(datapackFilesListSub.at(index));
                    abort();
                }
                index++;
            }
            datapackFilesListSub.clear();
            if(correctContent==0)
                qDebug() << "Error, no valid content: correctContent==0\n" << QString::fromStdString(stringimplode(content,"\n")) << "\nFor:" << reply->url().toString();
            if(fileToGet==0)
                datapackDownloadFinishedSub();
            else
                datapackSizeSub(fileToGet,sizeToGet*1000);
        }
    }
}

const std::vector<std::string> Api_client_real::listDatapackSub(std::string suffix)
{
    std::vector<std::string> returnFile;
    QDir finalDatapackFolder(mDatapackSub+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &listToAdd=listDatapackSub(suffix+fileInfo.fileName().toStdString()+"/");
            returnFile.insert(returnFile.end(),listToAdd.cbegin(),listToAdd.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if((QString::fromStdString(suffix)+fileInfo.fileName()).contains(Api_client_real::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile.push_back(suffix+fileInfo.fileName().toStdString());
            //is invalid
            else
            {
                qDebug() << (QStringLiteral("listDatapack(): remove invalid file: %1").arg(QString::fromStdString(suffix)+fileInfo.fileName()));
                QFile file(mDatapackSub+QString::fromStdString(suffix)+fileInfo.fileName());
                if(!file.remove())
                    qDebug() << (QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(QString::fromStdString(suffix)+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void Api_client_real::cleanDatapackSub(std::string suffix)
{
    QDir finalDatapackFolder(mDatapackSub+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackSub(suffix+fileInfo.fileName().toStdString()+"/");//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapackSub+QString::fromStdString(suffix));
}

void Api_client_real::downloadProgressDatapackSub(int64_t bytesReceived, int64_t bytesTotal)
{
    if(!datapackTarXzMain && !datapackTarXzSub)
    {
        if(bytesReceived>0)
            datapackSizeSub(1,static_cast<uint32_t>(bytesTotal));
    }
    emit progressingDatapackFileSub(static_cast<uint32_t>(bytesReceived));
}

void Api_client_real::httpErrorEventSub()
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
    //mirrorTryNextSub();//mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void Api_client_real::sendDatapackContentSub()
{
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(wait_datapack_content_sub)
    {
        qDebug() << (QStringLiteral("already in wait of datapack content"));
        return;
    }

    datapackTarXzSub=false;
    wait_datapack_content_sub=true;
    datapackFilesListSub=listDatapackSub();
    std::sort(datapackFilesListSub.begin(),datapackFilesListSub.end());
    emit doDifferedChecksumSub(mDatapackSub.toStdString());
}
