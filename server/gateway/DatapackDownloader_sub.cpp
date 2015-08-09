#include "DatapackDownloaderMainSub.h"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QProcess>
#include <QDebug>

#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"
#include "LinkToGameServer.h"
#include "EpollServerLoginSlave.h"

void DatapackDownloaderMainSub::writeNewFileSub(const QString &fileName,const QByteArray &data)
{
    const QString &fullPath=mDatapackSub+text_slash+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }

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

bool DatapackDownloaderMainSub::getHttpFileSub(const QString &url, const QString &fileName)
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(httpError)
        return false;
    if(!httpModeSub)
        httpModeSub=true;

    const QString &fullPath=mDatapackSub+text_slash+fileName;
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        if(!QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath()))
        {
            qDebug() << "unable to make the path: " << fileInfo.absolutePath();
            abort();
        }
    }

    FILE *fp = fopen(fullPath.toLocal8Bit().constData(),"wb");
    if(fp!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        fclose(fp);
        if(res!=CURLE_OK || http_code!=200)
        {
            httpError=true;
            qDebug() << (QStringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            datapackDownloadError();
            return false;
        }
        else
            return true;
    }
    else
    {
        qDebug() << "unable to open file to write:" << fileName;
        return false;
    }
}

void DatapackDownloaderMainSub::datapackDownloadFinishedSub()
{
    haveTheDatapackMainSub();
    resetAll();
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
        {
            QFile file(mDatapackBase+QString("/pack/datapack-sub-%1-%2.tar.xz").arg(mainDatapackCode).arg(subDatapackCode));
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        {
            QFile file(mDatapackBase+QString("/datapack-list/sub-%1-%2.txt").arg(mainDatapackCode).arg(subDatapackCode));
            if(file.exists())
                if(!file.remove())
                {
                    qDebug() << "Unable to remove "+file.fileName();
                    return;
                }
        }
        if(sendedHashSub.isEmpty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            abort();//need CommonSettings::commonSettings.datapackHash send by the server
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

            const QString url=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_sub)+QStringLiteral("pack/diff/datapack-sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral("-%1.tar.xz").arg(QString(hash.toHex()));

            struct MemoryStruct chunk;
            chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
            chunk.size = 0;    /* no data at this point */
            curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            const CURLcode res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if(res!=CURLE_OK || http_code!=200)
            {
                qDebug() << (QStringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
                httpFinishedForDatapackListSub();
                return;
            }
            httpFinishedForDatapackListSub(QByteArray(chunk.memory,chunk.size));
            free(chunk.memory);
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
    const QStringList &httpDatapackMirrorList=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(DatapackDownloaderMainSub::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarXzSub)
    {
        const QString url=httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("pack/datapack-sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral(".tar.xz");

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            qDebug() << (QStringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            httpFinishedForDatapackListSub();
            return;
        }
        httpFinishedForDatapackListSub(QByteArray(chunk.memory,chunk.size));
        free(chunk.memory);
    }
    else
    {
        if(index_mirror_sub>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.xz and after the datapack-list/sub-XXXXX-YYYYYY.txt, and only after that's quit */
            return;

        const QString url=httpDatapackMirrorList.at(index_mirror_sub)+QStringLiteral("datapack-list/sub-")+mainDatapackCode+QStringLiteral("-")+subDatapackCode+QStringLiteral(".txt");

        struct MemoryStruct chunk;
        chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, EpollServerLoginSlave::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        const CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res!=CURLE_OK || http_code!=200)
        {
            qDebug() << (QStringLiteral("get url %1: %2 failed with code %3").arg(url).arg(res).arg(http_code));
            httpFinishedForDatapackListSub();
            return;
        }
        httpFinishedForDatapackListSub(QByteArray(chunk.memory,chunk.size));
        free(chunk.memory);
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

void DatapackDownloaderMainSub::httpFinishedForDatapackListSub(const QByteArray data)
{
    if(subDatapackCode.isEmpty())
    {
        qDebug() << "subDatapackCode.isEmpty() to get from mirror";
        abort();
    }
    if(data.isEmpty())
    {
        mirrorTryNextSub();
        return;
    }
    else
    {
        if(!datapackTarXzSub)
        {
            qDebug() << "datapack.tar.xz size:" << QString("%1KB").arg(data.size()/1000);
            datapackTarXzSub=true;
            xzDecodeThreadSub.setData(data,16*1024*1024);
            xzDecodeThreadSub.run();
            decodedIsFinishSub();
            return;
        }
        else
        {
            httpError=false;
            const QStringList &content=QString::fromUtf8(data).split(QRegularExpression("[\n\r]+"));
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
                                if(!getHttpFileSub(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=*reinterpret_cast<const quint32 *>(QByteArray::fromHex(partialHashString.toLatin1()).constData()))
                            {
                                if(!getHttpFileSub(selectedMirror+fileString,fileString))
                                {
                                    datapackDownloadError();
                                    return;
                                }
                            }
                        }
                        else
                        {
                            if(!getHttpFileSub(selectedMirror+fileString,fileString))
                            {
                                datapackDownloadError();
                                return;
                            }
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
                qDebug() << "Error, no valid content: correctContent==0\n" << content.join("\n");
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
