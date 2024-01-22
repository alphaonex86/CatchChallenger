#include "Api_client_real.hpp"

using namespace CatchChallenger;

#include <iostream>
#include <cmath>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QDataStream>
#include <QDebug>
#include <QStandardPaths>
#ifdef CATCHCHALLENGER_CACHE_HPS
#include <QDir>
#include <QDirIterator>
#endif

#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../libcatchchallenger/TarDecode.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include <xxhash.h>

void Api_client_real::writeNewFileMain(const std::string &fileName,const std::string &data)
{
    if(mDatapackMain.empty())
        abort();
    const std::string &fullPath=mDatapackMain+"/"+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(QString::fromStdString(fullPath));
        QFileInfo fileInfo(file);

        QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

        if(file.exists())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            {
                QFile file(QString::fromStdString(fullPath));
                if(file.open(QIODevice::ReadOnly))
                {
                    QByteArray data2=file.readAll();
                    if((size_t)data2.size()==data.size())
                    {
                        if(memcmp(data2.data(),data.data(),data.size())==0)
                        {
                            std::cerr << "duplicate download detected: " << fullPath << ", the file on hdd is same than downloaded file (abort) " << __FILE__ << ":" << __LINE__ << std::endl << std::endl;
                            //abort();
                        }
                    }
                    file.close();
                }
            }
            #endif
            if(!file.remove())
            {
                qDebug() << (QStringLiteral("Can't remove: %1: %2").arg(QString::fromStdString(fileName)).arg(file.errorString()));
                return;
            }
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
        uint32_t h=0;
        XXH32_canonical_t htemp;
        XXH32_canonicalFromHash(&htemp,XXH32(data.data(),data.size(),0));
        memcpy(&h,&htemp.digest,sizeof(h));
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        DatapackChecksum::writeCachePartialHash(fullPath,h);
        #else
        if(!DatapackChecksum::writeCachePartialHash(fullPath,h))
            abort();
        #endif
    }
    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(!mDatapackMainCache.empty() && !cacheRemovedMain)
    {
        cacheRemovedMain=true;
        const QString &path=QString::fromStdString(mDatapackMainCache);
        QDir dir(path);
        QDirIterator it(dir, QDirIterator::Subdirectories);
        while (it.hasNext())
            QFile(it.next()).remove();
        dir.rmpath(path);
    }
    #endif

    //send size
    {
        if(httpModeMain)
            newDatapackFileMain(ceil((double)data.size()/1000)*1000);
        else
            newDatapackFileMain(data.size());
    }
}

bool Api_client_real::getHttpFileMain(const std::string &url, const std::string &fileName)
{
    std::cout << "Api_client_real::getHttpFileMain(): " << url << "," << fileName << std::endl;
    if(httpError)
        return false;
    if(!httpModeMain)
        httpModeMain=true;
    QNetworkRequest networkRequest(QString::fromStdString(url));
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
    if(urlInWaitingListMain.size()>MAXFILETODOWNLOAD)
    {
        std::cerr << "urlInWaitingListMain.size()>MAXFILETODOWNLOAD" << std::endl;
        abort();
    }
    if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedMain))
        abort();
    return true;
}

void Api_client_real::httpFinishedMain()
{
    if(httpError)
        return;
    if(urlInWaitingListMain.empty())
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
                 "<br />get the new update failed: "+reply->errorString().toStdString(),
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
    if(urlInWaitingListMain.find(reply)==urlInWaitingListMain.cend())
    {
        httpError=true;
        newError(tr("Datapack downloading error").toStdString(),"reply of unknown query (main)");
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    QByteArray QtData=reply->readAll();
    const UrlInWaiting &urlInWaiting=urlInWaitingListMain.at(reply);
    writeNewFileMain(urlInWaiting.fileName,std::string(QtData.data(),QtData.size()));

    if(urlInWaitingListMain.find(reply)!=urlInWaitingListMain.cend())
        urlInWaitingListMain.erase(reply);
    else
        qDebug() << (QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingListMain.size()));
    reply->deleteLater();
    if(urlInWaitingListMain.empty())
        checkIfContinueOrFinished();
}

void Api_client_real::datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList)
{
    if((uint32_t)datapackFilesListMain.size()!=partialHashList.size())
    {
        qDebug() << "datapackFilesListMain.size()!=partialHash.size():" << datapackFilesListMain.size() << "!=" << partialHashList.size();
        qDebug() << "datapackFilesListMain:" << QString::fromStdString(stringimplode(datapackFilesListMain,'\n'));
        qDebug() << "datapackFilesList:" << QString::fromStdString(stringimplode(datapackFilesList,'\n'));
        abort();
    }
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.mainDatapackCode==[main]";
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #else
            return;
            #endif
        }
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode=="[sub]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode==[sub]";
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #else
            return;
            #endif
        }
        if(mDatapackMain==(mDatapackBase+"map/main/[main]/"))
        {
            qDebug() << "mDatapackMain==(mDatapackBase+\"map/main/[main]/\")";
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #else
            return;
            #endif
        }
    }

    this->datapackFilesListMain=datapackFilesList;
    this->partialHashListMain=partialHashList;
    if(!datapackFilesListMain.empty() && hash==CommonSettingsServer::commonSettingsServer.datapackHashServerMain)
    {
        std::cerr << "Main: Datapack is not empty and get nothing from serveur because the local datapack hash match with the remote " << binarytoHexa(hash) << std::endl;
        wait_datapack_content_main=false;
        checkIfContinueOrFinished();
        return;
    }

    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
    {
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.empty())
        {
            qDebug() << "Datapack checksum done but not send by the server";
            return;//need CommonSettings::commonSettings.datapackHash send by the server
        }
        qDebug() << "Main: Datapack is empty or hash don't match, get from server, hash local: " << QString::fromStdString(binarytoHexa(hash)) << ", hash on server: " << QString::fromStdString(binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerMain));
        uint8_t datapack_content_query_number=queryNumber();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)DatapackStatus::Main;
        out << (uint32_t)datapackFilesListMain.size();
        unsigned int index=0;
        while(index<datapackFilesListMain.size())
        {
            if(datapackFilesListMain.at(index).size()>254 || datapackFilesListMain.at(index).empty())
            {
                qDebug() << (QStringLiteral("rawFileName too big or not compatible with utf8"));
                return;
            }
            out << (uint8_t)datapackFilesListMain.at(index).size();
            outputData+=QByteArray(datapackFilesListMain.at(index).data(),static_cast<uint32_t>(datapackFilesListMain.at(index).size()));
            out.device()->seek(out.device()->size());

            index++;
        }
        index=0;
        while(index<datapackFilesListMain.size())
        {
            /*struct stat info;
            stat(std::string(mDatapackBase+datapackFilesListMain.at(index)).toLatin1().data(),&info);*/
            out << (uint32_t)partialHashList.at(index);

            index++;
        }
        packOutcommingQuery(0xA1,datapack_content_query_number,outputData.constData(),outputData.size());
    }
    else
    {
        if(datapackFilesListMain.empty())
        {
            {
                if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
                {
                    qDebug() << "CommonSettingsServer::commonSettingsServer.mainDatapackCode==[main]";
                    abort();
                }
                if(CommonSettingsServer::commonSettingsServer.subDatapackCode=="[sub]")
                {
                    qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode==[sub]";
                    abort();
                }
                if(mDatapackMain==(mDatapackBase+"map/main/[main]/"))
                {
                    qDebug() << "mDatapackMain==(mDatapackBase+\"map/main/[main]/\")";
                    abort();
                }
            }
            index_mirror_main=0;
            test_mirror_main();
            qDebug() << "Datapack is empty, get from mirror into" << QString::fromStdString(mDatapackMain);
        }
        else
        {
            qDebug() << "Datapack don't match with server hash, get from mirror";
            QNetworkRequest networkRequest(
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer)
                        .split(Api_client_real::text_dotcoma, Qt::SkipEmptyParts).at(index_mirror_main)+
                        QStringLiteral("pack/diff/datapack-main-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+
                        QStringLiteral("-%1.tar.zst").arg(QString::fromStdString(binarytoHexa(hash)))
                        );
            QNetworkReply *reply = qnam.get(networkRequest);
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListMain))
                abort();
            if(!connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackMain))
                abort();
        }
    }
}

void Api_client_real::test_mirror_main()
{
    QNetworkReply *reply;
    const QStringList &httpDatapackMirrorList=QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer).split(Api_client_real::text_dotcoma, Qt::SkipEmptyParts);
    if(!datapackTarMain)
    {
        std::cout << (httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("pack/datapack-main-")+
                      QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+QStringLiteral(".tar.zst")).toStdString() << std::endl;
        QString fullDatapack=httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("pack/datapack-main-")+
                QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+QStringLiteral(".tar.zst");
        QNetworkRequest networkRequest(fullDatapack);
        qDebug() << "Try download: " << fullDatapack;
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListMain))//todo fix it, put httpFinished* broke it
                abort();
    }
    else
    {
        if(index_mirror_main>=httpDatapackMirrorList.size())
            /* here and not above because at last mirror you need try the tar.zst and after the datapack-list/main-XXXX.txt, and only after that's quit */
            return;

        std::cout << (httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("datapack-list/main-")+
                      QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+QStringLiteral(".txt")).toStdString() << std::endl;
        QNetworkRequest networkRequest(httpDatapackMirrorList.at(index_mirror_main)+QStringLiteral("datapack-list/main-")+
                                       QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+QStringLiteral(".txt"));
        reply = qnam.get(networkRequest);
        if(reply->error()==QNetworkReply::NoError)
            if(!connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinishedForDatapackListMain))
                abort();
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        if(!connect(reply, &QNetworkReply::errorOccurred, this, &Api_client_real::httpErrorEventMain))
            abort();
        if(!connect(reply, &QNetworkReply::downloadProgress, this, &Api_client_real::downloadProgressDatapackMain))
            abort();
    }
    else
    {
        qDebug() << reply->url().toString() << reply->errorString();
        mirrorTryNextMain(reply->url().toString().toStdString()+": "+reply->errorString().toStdString());
        return;
    }
}

void Api_client_real::decodedIsFinishMain()
{
    if(zstdDecodeThreadMain.errorFound())
        test_mirror_main();
    else
    {
        const std::vector<char> &decodedData=zstdDecodeThreadMain.decodedData();
        TarDecode tarDecode;
        if(tarDecode.decodeData(decodedData))
        {
            QStringList l=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(Api_client_real::text_dotcoma);
            QSet<QString> extensionAllowed(l.cbegin(),l.cend());
            QDir dir;
            const std::vector<std::string> &fileList=tarDecode.getFileList();
            const std::vector<std::vector<char> > &dataList=tarDecode.getDataList();
            unsigned int index=0;
            while(index<fileList.size())
            {
                QFile file(QString::fromStdString(mDatapackMain)+QString::fromStdString(fileList.at(index)));
                QFileInfo fileInfo(file);
                dir.mkpath(fileInfo.absolutePath());
                if(extensionAllowed.contains(fileInfo.suffix()))
                {
                    if(file.open(QIODevice::Truncate|QIODevice::WriteOnly))
                    {
                        file.write(dataList.at(index).data(),dataList.at(index).size());
                        file.close();
                        uint32_t h=0;
                        XXH32_canonical_t htemp;
                        XXH32_canonicalFromHash(&htemp,XXH32(dataList.at(index).data(),dataList.at(index).size(),0));
                        memcpy(&h,&htemp.digest,sizeof(h));
                        #ifndef CATCHCHALLENGER_EXTRA_CHECK
                        DatapackChecksum::writeCachePartialHash(fileInfo.absoluteFilePath().toStdString(),h);
                        #else
                        if(!DatapackChecksum::writeCachePartialHash(fileInfo.absoluteFilePath().toStdString(),h))
                            abort();
                        #endif
                    }
                    else
                    {
                        newError(tr("Disk error").toStdString(),
                                 QStringLiteral("unable to write file of datapack %1: %2").arg(file.fileName()).arg(file.errorString()).toStdString());
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
            wait_datapack_content_main=false;
            checkIfContinueOrFinished();
        }
        else
            test_mirror_main();
    }
}

bool Api_client_real::mirrorTryNextMain(const std::string &error)
{
    if(!datapackTarMain)
    {
        datapackTarMain=true;
        test_mirror_main();
    }
    else
    {
        datapackTarMain=false;
        index_mirror_main++;
        if(index_mirror_main>=QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer).split(Api_client_real::text_dotcoma, Qt::SkipEmptyParts).size())
        {
            newError(tr("Unable to download the datapack").toStdString()+"<br />Details:<br />"+error,"Get the list failed: "+error);
            return false;
        }
        else
            test_mirror_main();
    }
    return true;
}

void Api_client_real::httpFinishedForDatapackListMain()
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
            errorString=(QStringLiteral("Main Problem with the datapack list reply:%1 %2 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                         .toStdString()
                                                  );
        else
            errorString=(QStringLiteral("Main Problem with the datapack list reply:%1 %2 with proxy: %3 %4 type %5 (try next)")
                                                  .arg(reply->url().toString())
                                                  .arg(reply->errorString())
                                                  .arg(proxy.hostName())
                                                  .arg(proxy.port())
                                                  .arg(proxy.type())
                         .toStdString()
                                                  );
        std::cerr << errorString << std::endl;
        reply->deleteLater();
        mirrorTryNextMain(errorString);
        return;
    }
    else
    {
        if(!datapackTarMain)
        {
            qDebug() << QStringLiteral("pack/datapack-main-")+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)
                        +QStringLiteral(".tar.zst") << " size:" << QString("%1KB").arg(reply->size()/1000);
            datapackTarMain=true;
            QByteArray olddata=reply->readAll();
            std::vector<char> newdata;
            newdata.resize(olddata.size());
            memcpy(newdata.data(),olddata.constData(),olddata.size());
            zstdDecodeThreadMain.setData(newdata);
            zstdDecodeThreadMain.setObjectName("zstddtm");
            #ifndef NOTHREADS
            zstdDecodeThreadMain.start(QThread::LowestPriority);
            #else
            zstdDecodeThreadMain.run();
            #endif
            return;
        }
        else
        {
            int sizeToGet=0;
            int fileToGet=0;
            std::regex datapack_rightFileName(DATAPACK_FILE_REGEX);
            /*ref crash here*/const std::string selectedMirror=stringsplit(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,';').at(index_mirror_main)+
                    "map/main/"+
                    CommonSettingsServer::commonSettingsServer.mainDatapackCode
                    +"/";
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
                    std::cerr << "partialHashList not divisible by 4 (main), check: "+reply->url().toString().toStdString() << std::endl;
                    newError(tr("Wrong datapack format").toStdString(),"partialHashList not divisible by 4 (main), check: "+reply->url().toString().toStdString());
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
                        int indexInDatapackList=vectorindexOf(datapackFilesListMain,fileString);
                        if(indexInDatapackList!=-1)
                        {
                            const uint32_t &hashFileOnDisk=partialHashListMain.at(indexInDatapackList);
                            if(!FacilityLibGeneral::isFile(mDatapackMain+fileString))
                            {
                                fileToGet++;
                                if(!getHttpFileMain(selectedMirror+fileString,fileString))
                                {
                                    newError(tr("Unable to get datapack file").toStdString(),"");
                                    return;
                                }
                            }
                            else if(hashFileOnDisk!=partialHashString)
                            {
                                fileToGet++;
                                if(!getHttpFileMain(selectedMirror+fileString,fileString))
                                {
                                    newError(tr("Unable to get datapack file").toStdString(),"");
                                    return;
                                }
                            }
                            partialHashListMain.erase(partialHashListMain.cbegin()+indexInDatapackList);
                            datapackFilesListMain.erase(datapackFilesListMain.cbegin()+indexInDatapackList);
                        }
                        else
                        {
                            fileToGet++;
                            if(!getHttpFileMain(selectedMirror+fileString,fileString))
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
            while(index<datapackFilesListMain.size())
            {
                if(!QFile(QString::fromStdString(mDatapackMain)+QString::fromStdString(datapackFilesListMain.at(index))).remove())
                {
                    qDebug() << "Unable to remove" << QString::fromStdString(datapackFilesListMain.at(index));
                    abort();
                }
                index++;
            }
            datapackFilesListMain.clear();
            if(correctContent==0)
            {
                qDebug() << "Error, no valid content: correctContent==0\n" << QString::fromStdString(stringimplode(content,"\n")) << "\nFor:" << reply->url().toString();
                abort();
                return;
            }
            if(fileToGet==0)
                checkIfContinueOrFinished();
            else
                datapackSizeMain(fileToGet,sizeToGet*1000);
        }
    }
}

const std::vector<std::string> Api_client_real::listDatapackMain(std::string suffix)
{
    if(regex_search(suffix,excludePathMain))
        return std::vector<std::string>();

    std::vector<std::string> returnFile;
    QDir finalDatapackFolder(QString::fromStdString(mDatapackMain)+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for(int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
        {
            const std::vector<std::string> &listToAdd=listDatapackMain(suffix+fileInfo.fileName().toStdString()+"/");
            returnFile.insert(returnFile.end(),listToAdd.cbegin(),listToAdd.cend());//put unix separator because it's transformed into that's under windows too
        }
        else
        {
            //if match with correct file name, considere as valid
            if((QString::fromStdString(suffix)+fileInfo.fileName()).contains(Api_client_real::regex_DATAPACK_FILE_REGEX) &&
                    extensionAllowed.find(fileInfo.suffix().toStdString())!=extensionAllowed.cend())
                returnFile.push_back(suffix+fileInfo.fileName().toStdString());
            //is invalid
            else
            {
                qDebug() << (QStringLiteral("listDatapack(): remove invalid file: %1").arg(QString::fromStdString(suffix)+fileInfo.fileName()));
                QFile file(QString::fromStdString(mDatapackMain)+QString::fromStdString(suffix)+fileInfo.fileName());
                if(!file.remove())
                    qDebug() << (QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(QString::fromStdString(suffix)+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    std::sort(returnFile.begin(),returnFile.end());
    return returnFile;
}

void Api_client_real::cleanDatapackMain(std::string suffix)
{
    QDir finalDatapackFolder(QString::fromStdString(mDatapackMain)+QString::fromStdString(suffix));
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapackMain(suffix+fileInfo.fileName().toStdString()+"/");//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(QString::fromStdString(mDatapackMain)+QString::fromStdString(suffix));
}

void Api_client_real::datapackDownloadFinishedMain()
{
    datapackStatus=DatapackStatus::Sub;
}

void Api_client_real::downloadProgressDatapackMain(int64_t bytesReceived, int64_t bytesTotal)
{
    if(!datapackTarMain && !datapackTarSub)
    {
        if(bytesReceived>0)
            datapackSizeMain(1,static_cast<uint32_t>(bytesTotal));
    }
    emit progressingDatapackFileMain(static_cast<uint32_t>(bytesReceived));
}

void Api_client_real::httpErrorEventMain(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        newError(tr("Datapack downloading error: %1").arg(code).toStdString(),"reply for http is NULL");
        socket->disconnectFromHost();
        return;
    }
    qDebug() << reply->url().toString() << reply->errorString();
    //mirrorTryNextMain();//mirrorTryNextBase();-> double mirrorTryNext*() call due to httpFinishedForDatapackList*()
    return;
}

void Api_client_real::sendDatapackContentMain()
{
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.mainDatapackCode==[main]";
            abort();
        }
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode=="[sub]")
        {
            qDebug() << "CommonSettingsServer::commonSettingsServer.subDatapackCode==[sub]";
            abort();
        }
        if(mDatapackMain==(mDatapackBase+"map/main/[main]/"))
        {
            qDebug() << "mDatapackMain==(mDatapackBase+\"map/main/[main]/\")";
            abort();
        }
    }
    if(wait_datapack_content_main)
    {
        qDebug() << (QStringLiteral("already in wait of datapack content main (main)"));
        return;
    }
    if(wait_datapack_content_sub)
    {
        qDebug() << (QStringLiteral("already in wait of datapack content main (sub)"));
        return;
    }

#ifdef CATCHCHALLENGER_CACHE_HPS
    {
        cacheRemovedMain=false;
        QStringList l=QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
        if(!l.empty())
        {
            std::string b=datapackPathBase();
            if(stringEndsWith(b,"/"))
                b=b.substr(0,b.size()-1);
            b+="-cache/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/";
            mDatapackBaseCache=b;
        }
    }
#endif
    //compute the mirror
    {
        QStringList values=QString::fromStdString(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer).split(Api_client_real::text_dotcoma, Qt::SkipEmptyParts);
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
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=values.join(Api_client_real::text_dotcoma).toStdString();
    }

    datapackTarMain=false;
    wait_datapack_content_main=true;
    datapackFilesListMain=listDatapackMain();
    std::sort(datapackFilesListMain.begin(),datapackFilesListMain.end());
    emit doDifferedChecksumMain(mDatapackMain);
}

void Api_client_real::checkIfContinueOrFinished()
{
    wait_datapack_content_main=false;
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        datapackStatus=DatapackStatus::Finished;
        haveTheDatapackMainSub();
    }
    else
    {
        datapackStatus=DatapackStatus::Sub;
        sendDatapackContentSub();
    }
}
