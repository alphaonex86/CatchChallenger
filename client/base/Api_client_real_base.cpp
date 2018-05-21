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
#include "qt-tar-compressed/QTarDecode.h"
#include "../../general/base/GeneralVariable.h"

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
    connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedBase);
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
    if(!urlInWaitingListBase.contains(reply))
    {
        httpError=true;
        newError(tr("Datapack downloading error"),QStringLiteral("reply of unknown query"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingListBase.value(reply);
    writeNewFileBase(urlInWaiting.fileName,reply->readAll());

    if(urlInWaitingListBase.remove(reply)!=1)
        qDebug() << (QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListBase.size()));
    reply->deleteLater();
    if(urlInWaitingListBase.empty())
        datapackDownloadFinishedBase();
}

void Api_client_real::datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    if((uint32_t)datapackFilesListBase.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListBase.size()!=partialHash.size():" << datapackFilesListBase.size() << "!=" << partialHashList.size();
        qDebug() << "datapackFilesListdatapackFilesListBase:" << std::string::fromStdString(stringimplode(datapackFilesListBase,'\n'));
        qDebug() << "datapackFilesList:" << std::string::fromStdString(stringimplode(datapackFilesList,'\n'));
        abort();
    }

    this->datapackFilesListBase=datapackFilesList;
    this->partialHashListBase=partialHashList;
    if(!datapackFilesListBase.empty() && hash==CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
    {
        std::cerr << "Base: Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << binarytoHexa(hash) << std::endl;
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
        qDebug() << "Base: Datapack is empty or hash don't match, get from server, hash local: " << std::string::fromStdString(binarytoHexa(hash)) << ", hash on server: " << std::string::fromStdString(binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase));
        uint8_t datapack_content_query_number=queryNumber();
        std::string outputData;
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
            outputData+=std::string(datapackFilesListBase.at(index).c_str(),static_cast<uint32_t>(datapackFilesListBase.at(index).size()));
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
            qDebug() << "Datapack is empty, get from mirror into" << mDatapackBase;
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(
                        std::string::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)
                        .split(Api_client_real::text_dotcoma,std::string::SkipEmptyParts).at(index_mirror_base)+
                        QStringLiteral("pack/diff/datapack-base-%1.tar.zst").arg(std::string::fromStdString(binarytoHexa(hash)))
                        );
            QNetworkReply *reply = qnam.get(networkRequest);
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase);
            connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackBase);
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
    const std::stringList &httpDatapackMirrorList=std::string::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase).split(Api_client_real::text_dotcoma,std::string::SkipEmptyParts);
    if(!datapackTarBase)
    {
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_base)+QStringLiteral("pack/datapack.tar.zst"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase);//fix it, put httpFinished* broke it
    }
    else
    {
        if(index_mirror_base>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.zst and after the datapack-list/base.txt, and only after that's quit */
            return;

        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_base)+QStringLiteral("datapack-list/base.txt"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListBase);
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &Api_client_real::httpErrorEventBase);
        connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackBase);
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextBase(reply->url().toString()+": "+reply->errorString());
        return;
    }
}

void Api_client_real::decodedIsFinishBase()
{
    if(zstdDecodeThreadBase.errorFound())
        test_mirror_base();
    else
    {
        const std::vector<char> &decodedData=zstdDecodeThreadBase.decodedData();
        QTarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QSet<std::string> extensionAllowed=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED).split(Api_client_real::text_dotcoma).toSet();
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                QFile file(mDatapackBase+std::string::fromStdString(fileList.at(index)));
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
            wait_datapack_content_base=false;
            datapackDownloadFinishedBase();
        }
        else
            test_mirror_base();
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
        if(index_mirror_base>=std::string::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase).split(Api_client_real::text_dotcoma,std::string::SkipEmptyParts).size())
        {
            newError(tr("Unable to download the datapack")+"<br />Details:<br />"+error,QStringLiteral("Get the list failed: ")+error);
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
        newError(tr("Datapack downloading error"),QStringLiteral("reply for http is NULL"));
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
                                                  );
        else
            errorString=(QStringLiteral("Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                                                  );
        qDebug() << errorString;
        reply->deleteLater();
        mirrorTryNextBase(errorString);
        return;
    }
    else
    {
        if(!datapackTarBase)
        {
            qDebug() << "datapack.tar.zst size:" << std::string("%1KB").arg(reply->size()/1000);
            datapackTarBase=true;
            std::string olddata=reply->readAll();
            std::vector<char> newdata;
            newdata.resize(olddata.size());
            memcpy(newdata.data(),olddata.constData(),olddata.size());
            zstdDecodeThreadBase.setData(newdata);
            zstdDecodeThreadBase.start(QThread::LowestPriority);
            zstdDecodeThreadBase.setObjectName("zstddtb");
            return;
        }
        else
        {
            int sizeToGet=0;
            int fileToGet=0;
            std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
            /*ref crash here*/const std::string selectedMirror=stringsplit(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase,';').at(index_mirror_base);
            std::vector<char> data;
            std::string olddata=reply->readAll();
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
                newError(tr("Wrong datapack format"),"no text delimitor into file list: "+reply->url().toString());
                return;
            }
            std::vector<std::string> content;
            std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend());
            {
                if(partialHashListRaw.size()%4!=0)
                {
                    std::cerr << "partialHashList not divisible by 4" << std::endl;
                    newError(tr("Wrong datapack format"),"partialHashList not divisible by 4");
                    return;
                }
                {
                    std::string text(data.data(),endOfText);
                    content=stringsplit(text,'\n');
                }
                if(partialHashListRaw.size()/4!=content.size())
                {
                    std::cerr << "partialHashList/4!=content.size()" << std::endl;
                    newError(tr("Wrong datapack format"),"partialHashList/4!=content.size()");
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
                            if(!FacilityLibGeneral::isFile(mDatapackBase.toStdString()+fileString))
                            {
                                fileToGet++;
                                if(!getHttpFileBase(std::string::fromStdString(selectedMirror+fileString),std::string::fromStdString(fileString)))
                                {
                                    newError(tr("Unable to get datapack file"),"");
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                fileToGet++;
                                if(!getHttpFileBase(std::string::fromStdString(selectedMirror+fileString),std::string::fromStdString(fileString)))
                                {
                                    newError(tr("Unable to get datapack file"),"");
                                    return;
                                }
                            }
                            partialHashListBase.erase(partialHashListBase.cbegin()+indexInDatapackList);
                            datapackFilesListBase.erase(datapackFilesListBase.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            fileToGet++;
                            if(!getHttpFileBase(std::string::fromStdString(selectedMirror+fileString),std::string::fromStdString(fileString)))
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
            while(index<datapackFilesListBase.size())
            {
                if(!QFile(mDatapackBase+std::string::fromStdString(datapackFilesListBase.at(index))).remove())
                {
                    qDebug() << "Unable to remove" << std::string::fromStdString(datapackFilesListBase.at(index));
                    abort();
                }
                index++;
            }
            datapackFilesListBase.clear();
            if(correctContent==0)
            {
                qDebug() << "Error, no valid content: correctContent==0\n" << std::string::fromStdString(stringimplode(content,"\n")) << "\nFor:" << reply->url().toString();
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
    QDir finalDatapackFolder(mDatapackBase+std::string::fromStdString(suffix));
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
            if((std::string::fromStdString(suffix)+fileInfo.fileName()).contains(Api_client_real::regex_DATAPACK_FILE_REGEX) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile.push_back(suffix+fileInfo.fileName().toStdString());
            //is invalid
            else
            {
                qDebug() << (QStringLiteral("listDatapack(): remove invalid file: %1").arg(std::string::fromStdString(suffix)+fileInfo.fileName()));
                QFile file(mDatapackBase+std::string::fromStdString(suffix)+fileInfo.fileName());
                if(!file.remove())
                    qDebug() << (QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(std::string::fromStdString(suffix)+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void Api_client_real::cleanDatapackBase(std::string suffix)
{
    QDir finalDatapackFolder(mDatapackBase+std::string::fromStdString(suffix));
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
        finalDatapackFolder.rmpath(mDatapackBase+std::string::fromStdString(suffix));
}

void Api_client_real::httpErrorEventBase()
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
    //mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void Api_client_real::sendDatapackContentBase(const std::string &hashBase)
{
    if(wait_datapack_content_base)
    {
        qDebug() << (QStringLiteral("already in wait of datapack content"));
        return;
    }
    if(!hashBase.empty())
        if((unsigned int)hashBase.size()==(unsigned int)CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size() &&
                memcmp(hashBase.constData(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),hashBase.size())==0)
        {
            qDebug() << "Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote" << std::string(hashBase.toHex());
            datapackDownloadFinishedBase();
            return;
        }

    //compute the mirror
    {
        std::stringList values=std::string::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase).split(Api_client_real::text_dotcoma,std::string::SkipEmptyParts);
        {
            std::string slash(QStringLiteral("/"));
            int index=0;
            while(index<values.size())
            {
                if(!values.at(index).endsWith(slash))
                    values[index]+=slash;
                index++;
            }
        }
        std::string::fromStdString(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase)=values.join(Api_client_real::text_dotcoma);
    }

    datapackTarBase=false;
    wait_datapack_content_base=true;
    datapackFilesListBase=listDatapackBase();
    std::sort(datapackFilesListBase.begin(),datapackFilesListBase.end());
    emit doDifferedChecksumBase(mDatapackBase.toStdString());
}
