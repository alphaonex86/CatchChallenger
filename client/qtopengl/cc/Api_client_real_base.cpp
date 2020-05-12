#include "Api_client_real.hpp"

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
#include <QDebug>

#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../tarcompressed/TarDecode.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/xxhash/xxhash.h"

//need host + port here to have datapack base

void Api_client_real::writeNewFileBase(const std::string &fileName,const std::string &data)
{
    if(mDatapackBase.empty())
        abort();
    const std::string &fullPath=mDatapackBase+"/"+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(QString::fromStdString(fullPath));
        QFileInfo fileInfo(file);

        QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

        if(file.exists())
            if(!file.remove())
            {
                qDebug() << (QStringLiteral("Can't remove: %1: %2").arg(QString::fromStdString(fileName)).arg(file.errorString()));
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            qDebug() << (QStringLiteral("Can't open: %1: %2").arg(QString::fromStdString(fileName)).arg(file.errorString()));
            return;
        }
        if(file.write(data.data(),data.size())!=(int)data.size())
        {
            file.close();
            qDebug() << (QStringLiteral("Can't write: %1: %2").arg(QString::fromStdString(fileName)).arg(file.errorString()));
            return;
        }
        file.flush();
        file.close();
        const uint32_t h=XXH32(data.data(),data.size(),0);
        utimbuf butime;butime.actime=h;butime.modtime=h;
        utime(fullPath.c_str(),&butime);
    }

    //send size
    {
        if(httpModeBase)
            newDatapackFileBase(ceil((double)data.size()/1000)*1000);
        else
            newDatapackFileBase(data.size());
    }
}

bool Api_client_real::getHttpFileBase(const std::string &url, const std::string &fileName)
{
    if(httpError)
        return false;
    if(!httpModeBase)
        httpModeBase=true;
    QNetworkRequest networkRequest(QString::fromStdString(url));
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
    urlInWaitingListBase[reply]=urlInWaiting;
    if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedBase))
        abort();
    return true;
}

void Api_client_real::datapackDownloadFinishedBase()
{
    haveTheDatapack();
    datapackStatus=DatapackStatus::Main;
}

void Api_client_real::httpFinishedBase()
{
    if(httpError)
        return;
    if(urlInWaitingListBase.empty())
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"no more reply in waiting");
        socket->disconnectFromHost();
        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"reply for http is NULL");
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
        newError(tr("Unable to download the datapack").toStdString()+"<br />Details:<br />"+reply->url().toString().toStdString()+
                 "<br />get the new update failed: not finished","get the new update failed: not finished");
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    else if(reply->error())
    {
        httpError=true;
        newError(tr("Unable to download the datapack").toStdString()+"<br />Details:<br />"+reply->url().toString().toStdString()+
                 "<br />"+QStringLiteral("get the new update failed: %1").arg(reply->errorString()).toStdString(),
                 QStringLiteral("get the new update failed: %1").arg(reply->errorString()).toStdString());
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    } else if(!redirectionTarget.isNull()) {
        httpError=true;
        newError(tr("Unable to download the datapack").toStdString()+"<br />Details:<br />"+reply->url().toString().toStdString()+
                 "<br />"+QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()).toStdString(),
                 QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()).toStdString());
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    if(urlInWaitingListBase.find(reply)==urlInWaitingListBase.cend())
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"reply of unknown query (Base)");
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingListBase.at(reply);
    QByteArray QtData=reply->readAll();
    writeNewFileBase(urlInWaiting.fileName,std::string(QtData.data(),QtData.size()));

    if(urlInWaitingListBase.find(reply)!=urlInWaitingListBase.cend())
        urlInWaitingListBase.erase(reply);
    else
        qDebug() << (QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListBase.size()));
    reply->deleteLater();
    if(urlInWaitingListBase.empty())
        datapackDownloadFinishedBase();
}

void Api_client_real::datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    std::cerr << "Api_client_real::datapackChecksumDoneBase" << std::endl;
    if((uint32_t)datapackFilesListBase.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListBase.size()!=partialHash.size():" << datapackFilesListBase.size() << "!=" << partialHashList.size();
        qDebug() << "datapackFilesListdatapackFilesListBase:" << QString::fromStdString(stringimplode(datapackFilesListBase,'\n'));
        qDebug() << "datapackFilesList:" << QString::fromStdString(stringimplode(datapackFilesList,'\n'));
        abort();
    }

    this->datapackFilesListBase=datapackFilesList;
    this->partialHashListBase=partialHashList;
    if(!datapackFilesListBase.empty() && hash==CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
    {
        std::cerr << "Base: Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote " << binarytoHexa(hash) << std::endl;
        datapackDownloadFinishedBase();
        return;
    }

    if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.empty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        qDebug() << "Base: Datapack is empty or hash don't match, get from server, hash local: " << QString::fromStdString(binarytoHexa(hash)) << ", hash on server: " << QString::fromStdString(binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase));
        uint8_t datapack_content_query_number=queryNumber();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)DatapackStatus::Base;
        out << (uint32_t)datapackFilesListBase.size();
        unsigned int index=0;
        while(index<datapackFilesListBase.size())
        {
            if(datapackFilesListBase.at(index).size()>254 || datapackFilesListBase.at(index).empty())
            {
                qDebug() << (QStringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            out << (uint8_t)datapackFilesListBase.at(index).size();
            outputData+=QByteArray(datapackFilesListBase.at(index).data(),static_cast<uint32_t>(datapackFilesListBase.at(index).size()));
            out.device()->seek(out.device()->size());

            index++;
        }
        index=0;
        while(index<datapackFilesListBase.size())
        {
            /*struct stat info;
            stat(std::string(mDatapackBase+datapackFilesListBase.at(index)).toLatin1().data(),&info);*/
            out << (uint32_t)partialHashList.at(index);

            index++;
        }
        packOutcommingQuery(0xA1,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListBase.empty())
        {
            index_mirror_base=0;
            test_mirror_base();
            qDebug() << "Datapack is empty, get from mirror into" << QString::fromStdString(mDatapackBase);
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(
                        QString::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)
                        .split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).at(index_mirror_base)+
                        QStringLiteral("pack/diff/datapack-base-%1.tar.zst").arg(QString::fromStdString(binarytoHexa(hash)))
                        );
            QNetworkReply *reply = qnam.get(networkRequest);
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase))
                abort();
            if(!connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackBase))
                abort();
        }
    }
}

void Api_client_real::downloadProgressDatapackBase(int64_t bytesReceived, int64_t bytesTotal)
{
    if(!datapackTarBase)
    {
        if(bytesReceived>0)
            datapackSizeBase(1,static_cast<uint32_t>(bytesTotal));
    }
    emit progressingDatapackFileBase(static_cast<uint32_t>(bytesReceived));
}

void Api_client_real::test_mirror_base()
{
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=QString::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)
            .split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
    if(!datapackTarBase)
    {
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_base)+QStringLiteral("pack/datapack.tar.zst"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase))//fix it, put httpFinished* broke it
                abort();
    }
    else
    {
        if(index_mirror_base>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.zst and after the datapack-list/base.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_base)+QStringLiteral("datapack-list/base.txt"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase))
                abort();
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        if(!connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Api_client_real::httpErrorEventBase))
            abort();
        if(!connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackBase))
            abort();
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextBase(reply->url().toString().toStdString()+": "+reply->errorString().toStdString());
        return;
    }
}

void Api_client_real::decodedIsFinishBase()
{
    qDebug() << "Api_client_real::decodedIsFinishBase()";
    if(zstdDecodeThreadBase.errorFound())
    {
        qDebug() << "Api_client_real::decodedIsFinishBase() error";
        test_mirror_base();
    }
    else
    {
        const std::vector<char> &decodedData=zstdDecodeThreadBase.decodedData();
        TarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QSet<QString> extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(Api_client_real::text_dotcoma).toSet();
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                QFile file(QString::fromStdString(mDatapackBase)+QString::fromStdString(fileList.at(index)));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.contains(fileInfo.suffix()))
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index).data(),dataList.at(index).size());
                        file.close();
                        const uint32_t h=XXH32(dataList.at(index).data(),dataList.at(index).size(),0);
                        utimbuf butime;butime.actime=h;butime.modtime=h;
                        utime((mDatapackMain+fileList.at(index)).c_str(),&butime);
                    }
                    else
                    {
                        newError(tr("Disk error").toStdString(),
                                 QStringLiteral("unable to write file of datapack %1: %2")
                                 .arg(file.fileName()).arg(file.errorString()).toStdString());
                        return;
                    }
                }
                else
                {
                    newError(tr("Security error, file not allowed: %1").arg(file.fileName()).toStdString(),
                             QStringLiteral("file not allowed: %1").arg(file.fileName()).toStdString());
                    return;
                }
                index++;
            }
            wait_datapack_content_base=false;
            qDebug() << "Api_client_real::decodedIsFinishBase() finish";
            datapackDownloadFinishedBase();
        }
        else
        {
            qDebug() << "Api_client_real::decodedIsFinishBase(): test_mirror_base()";
            test_mirror_base();
        }
    }
}

bool Api_client_real::mirrorTryNextBase(const std::string &error)
{
    if(datapackTarBase==false)
    {
        datapackTarBase=true;
        test_mirror_base();
    }
    else
    {
        datapackTarBase=false;
        index_mirror_base++;
        if(index_mirror_base>=QString::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)
                .split(Api_client_real::text_dotcoma,QString::SkipEmptyParts).size())
        {
            newError(tr("Unable to download the datapack").toStdString()+"<br />Details:<br />"+error,QStringLiteral("Get the list failed: ").toStdString()+error);
            return false;
        }
        else
            test_mirror_base();
    }
    return true;
}

void Api_client_real::httpFinishedForDatapackListBase()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"reply for http is NULL");
        socket->disconnectFromHost();
        return;
    }
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!reply->isFinished() || reply->error() || !redirectionTarget.isNull())
    {
        const QNetworkProxy &proxy=qnam.proxy();
        std::string errorString;
        if(proxy==QNetworkProxy::NoProxy)
            errorString=(QStringLiteral("Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                         .toStdString()
                                                  );
        else
            errorString=(QStringLiteral("Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                         .toStdString()
                                                  );
        qDebug() << QString::fromStdString(errorString);
        reply->deleteLater();
        mirrorTryNextBase(errorString);
        return;
    }
    else
    {
        if(!datapackTarBase)
        {
            qDebug() << "datapack.tar.zst size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarBase=true;
            QByteArray olddata=reply->readAll();
            std::vector<char> newdata;
            newdata.resize(olddata.size());
            memcpy(newdata.data(),olddata.constData(),olddata.size());
            zstdDecodeThreadBase.setData(newdata);
            zstdDecodeThreadBase.setObjectName("zstddtb");
            #ifndef NOTHREADS
            zstdDecodeThreadBase.start(QThread::LowestPriority);
            #else
            zstdDecodeThreadBase.run();
            #endif
            qDebug() << "datapack.tar.zst start processing";
            return;
        }
        else
        {
            int sizeToGet=0;
            int fileToGet=0;
            std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
            /*ref crash here*/const std::string selectedMirror=stringsplit(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase,';').at(index_mirror_base);
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
                newError(tr("Wrong datapack format").toStdString(),"no text delimitor into file list: "+reply->url().toString().toStdString());
                return;
            }
            std::vector<std::string> content;
            std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend());
            {
                if(partialHashListRaw.size()%4!=0)
                {
                    std::cerr << "partialHashList not divisible by 4" << std::endl;
                    newError(tr("Wrong datapack format").toStdString(),"partialHashList not divisible by 4");
                    return;
                }
                {
                    std::string text(data.data(),endOfText);
                    content=stringsplit(text,'\n');
                }
                if(partialHashListRaw.size()/4!=content.size())
                {
                    std::cerr << "partialHashList/4!=content.size()" << std::endl;
                    newError(tr("Wrong datapack format").toStdString(),"partialHashList/4!=content.size()");
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
                        int indexInDatapackList=vectorindexOf(datapackFilesListBase,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListBase.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(mDatapackBase+fileString))
                            {
                                fileToGet++;
                                if(!getHttpFileBase(selectedMirror+fileString,fileString))
                                {
                                    newError(tr("Unable to get datapack file").toStdString(),"");
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                fileToGet++;
                                if(!getHttpFileBase(selectedMirror+fileString,fileString))
                                {
                                    newError(tr("Unable to get datapack file").toStdString(),"");
                                    return;
                                }
                            }
                            partialHashListBase.erase(partialHashListBase.cbegin()+indexInDatapackList);
                            datapackFilesListBase.erase(datapackFilesListBase.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            fileToGet++;
                            if(!getHttpFileBase(selectedMirror+fileString,fileString))
                            {
                                newError(tr("Unable to get datapack file").toStdString(),"");
                                return;
                            }
                        }
                    }
                }
                index++;
            }
            index=0;
            while(index<datapackFilesListBase.size())
            {
                if(!QFile(QString::fromStdString(mDatapackBase)+QString::fromStdString(datapackFilesListBase.at(index))).remove())
                {
                    qDebug() << "Unable to remove" << QString::fromStdString(datapackFilesListBase.at(index));
                    abort();
                }
                index++;
            }
            datapackFilesListBase.clear();
            if(correctContent==0)
            {
                qDebug() << "Error, no valid content: correctContent==0\n" << QString::fromStdString(stringimplode(content,"\n")) << "\nFor:" << reply->url().toString();
                abort();
                return;
            }
            if(fileToGet==0)
                datapackDownloadFinishedBase();
            else
                datapackSizeBase(fileToGet,sizeToGet*1000);
        }
    }
}

const std::vector<std::string> Api_client_real::listDatapackBase(std::string suffix)
{
    if(regex_search(suffix,excludePathBase))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    QDir finalDatapackFolder(QString::fromStdString(mDatapackBase)+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &listToAdd=listDatapackBase(suffix+fileInfo.fileName().toStdString()+"/");
            returnFile.insert(returnFile.end(),listToAdd.cbegin(),listToAdd.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if((QString::fromStdString(suffix)+fileInfo.fileName()).contains(
                        Api_client_real::regex_DATAPACK_FILE_REGEX) && extensionAllowed.find(fileInfo.suffix().toStdString())!=extensionAllowed.cend())
                returnFile.push_back(suffix+fileInfo.fileName().toStdString());
            //is invalid
            else
            {
                qDebug() << (QStringLiteral("listDatapack(): remove invalid file: %1").arg(QString::fromStdString(suffix)+fileInfo.fileName()));
                QFile file(QString::fromStdString(mDatapackBase)+QString::fromStdString(suffix)+fileInfo.fileName());
                if(!file.remove())
                    qDebug() << (QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(QString::fromStdString(suffix)+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void Api_client_real::cleanDatapackBase(std::string suffix)
{
    QDir finalDatapackFolder(QString::fromStdString(mDatapackBase)+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackBase(suffix+fileInfo.fileName().toStdString()+"/");//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(QString::fromStdString(mDatapackBase)+QString::fromStdString(suffix));
}

void Api_client_real::httpErrorEventBase()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"reply for http is NULL");
        socket->disconnectFromHost();
        return;
    }
    qDebug() << reply->url().toString() << reply->errorString();
    //mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void Api_client_real::sendDatapackContentBase(const std::string &hashBase)
{
    std::cerr << "Api_client_real::sendDatapackContentBase()" << std::endl;
    if(wait_datapack_content_base)
    {
        qDebug() << (QStringLiteral("already in wait of datapack content base"));
        return;
    }
    if(!hashBase.empty())
        if((unsigned int)hashBase.size()==(unsigned int)CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size() &&
                memcmp(hashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),hashBase.size())==0)
        {
            qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote"
                     << QString::fromStdString(binarytoHexa(hashBase.data(),hashBase.size()));
            datapackDownloadFinishedBase();
            return;
        }

    //compute the mirror
    {
        QStringList values=QString::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)
                .split(Api_client_real::text_dotcoma,QString::SkipEmptyParts);
        {
            QString slash("/");
            int index=0;
            while(index<values.size())
            {
                if(!values.at(index).endsWith(slash))
                    values[index]+=slash;
                index++;
            }
        }
        QString::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)=values.join(Api_client_real::text_dotcoma);
    }

    datapackTarBase=false;
    wait_datapack_content_base=true;
    datapackFilesListBase=listDatapackBase();
    std::sort(datapackFilesListBase.begin(),datapackFilesListBase.end());
    std::cerr << "Api_client_real::sendDatapackContentBase() doDifferedChecksumBase" << std::endl;
    emit doDifferedChecksumBase(mDatapackBase);
}
