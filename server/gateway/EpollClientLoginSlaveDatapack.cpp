#include "EpollClientLoginSlave.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"

#include <iostream>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
QHash<QString,EpollClientLoginSlave::DatapackCacheFile> EpollClientLoginSlave::datapack_file_list(const QString &path,const bool withHash)
{
    QHash<QString,DatapackCacheFile> filesList;
    QRegularExpression datapack_rightFileName(DATAPACK_FILE_REGEX);

    const QStringList &returnList=FacilityLibGeneral::listFolder(path);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef Q_OS_WIN32
        QString fileName=returnList.at(index);
        #else
        const QString &fileName=returnList.at(index);
        #endif
        if(fileName.contains(datapack_rightFileName))
        {
            QFileInfo fileInfo(fileName);
            if(!fileInfo.suffix().isEmpty() && DatapackDownloaderBase::extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(path+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        DatapackCacheFile datapackCacheFile;
                        #ifdef Q_OS_WIN32
                        fileName.replace(EpollClientLoginSlave::text_antislash,EpollClientLoginSlave::text_slash);//remplace if is under windows server
                        #endif
                        if(withHash)
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(file.readAll());
                            datapackCacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        else
                            datapackCacheFile.partialHash=0;
                        filesList[fileName]=datapackCacheFile;
                        file.close();
                    }
                }
            }
        }
        index++;
    }
    return filesList;
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void EpollClientLoginSlave::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &partialHashList)
{
    if(linkToGameServer==NULL)
    {
        std::cerr << "EpollClientLoginSlave::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &partialHashList) linkToGameServer==NULL" << std::endl;
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    /// \see EpollClientLoginSlave::parseFullQuery() already checked here
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    EpollClientLoginSlave::rawFilesBuffer.clear();
    EpollClientLoginSlave::compressedFilesBuffer.clear();
    EpollClientLoginSlave::rawFilesBufferCount=0;
    EpollClientLoginSlave::compressedFilesBufferCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    QString datapackPath;
    QHash<QString,DatapackCacheFile> filesList;
    switch(datapackStatus)
    {
        case DatapackStatus::Base:
            filesList=datapack_file_base.datapack_file_hash_cache;
            datapackPath=linkToGameServer->mDatapackBase;
        break;
        case DatapackStatus::Main:
            if(!datapack_file_main.contains(linkToGameServer->main))
            {
                std::cerr << "!datapack_file_main.contains " << linkToGameServer->main.toStdString() << std::endl;
                return;
            }
            if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.contains(linkToGameServer->main))
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main.toStdString() << std::endl;
                return;
            }
            if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.value(linkToGameServer->main).contains(linkToGameServer->sub))
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main.toStdString() << " " << linkToGameServer->sub.toStdString() << std::endl;
                return;
            }
            filesList=datapack_file_main.value(linkToGameServer->main).datapack_file_hash_cache;
            datapackPath=DatapackDownloaderMainSub::datapackDownloaderMainSub.value(linkToGameServer->main).value(linkToGameServer->sub)->mDatapackMain;
        break;
        case DatapackStatus::Sub:
            if(!datapack_file_sub.contains(linkToGameServer->main))
            {
                std::cerr << "!datapack_file_sub.contains " << linkToGameServer->main.toStdString() << std::endl;
                return;
            }
            if(!datapack_file_sub.value(linkToGameServer->main).contains(linkToGameServer->sub))
            {
                std::cerr << "!datapack_file_sub.contains " << linkToGameServer->main.toStdString() << " " << linkToGameServer->sub.toStdString() << std::endl;
                return;
            }
            if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.contains(linkToGameServer->main))
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main.toStdString() << std::endl;
                return;
            }
            if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.value(linkToGameServer->main).contains(linkToGameServer->sub))
            {
                std::cerr << "!DatapackDownloaderMainSub::datapackDownloaderMainSub " << linkToGameServer->main.toStdString() << " " << linkToGameServer->sub.toStdString() << std::endl;
                return;
            }
            filesList=datapack_file_sub.value(linkToGameServer->main).value(linkToGameServer->sub).datapack_file_hash_cache;
            datapackPath=DatapackDownloaderMainSub::datapackDownloaderMainSub.value(linkToGameServer->main).value(linkToGameServer->sub)->mDatapackSub;
        break;
        default:
        return;
    }
    QList<FileToSend> fileToSendList;

    const int &loop_size=files.size();
    //send the size to download on the client
    {
        //clone to drop the ask file and remain the missing client files
        QHash<QString,DatapackCacheFile> filesListForSize(filesList);
        int index=0;
        quint32 datapckFileNumber=0;
        quint32 datapckFileSize=0;
        while(index<loop_size)
        {
            const QString &fileName=files.at(index);
            const quint32 &clientPartialHash=partialHashList.at(index);
            if(fileName.contains(EpollClientLoginSlave::text_dotslash) || fileName.contains(EpollClientLoginSlave::text_antislash) || fileName.contains(EpollClientLoginSlave::text_double_slash))
            {
                std::cerr << QStringLiteral("file name contains illegale char: %1").arg(fileName).toStdString() << std::endl;
                return;
            }
            if(fileName.contains(fileNameStartStringRegex) || fileName.startsWith(EpollClientLoginSlave::text_slash))
            {
                std::cerr << QStringLiteral("start with wrong string: %1").arg(fileName).toStdString() << std::endl;
                return;
            }
            if(!fileName.contains(EpollClientLoginSlave::datapack_rightFileName))
            {
                std::cerr << QStringLiteral("file name sended wrong: %1").arg(fileName).toStdString() << std::endl;
                return;
            }
            if(filesListForSize.contains(fileName))
            {
                const quint32 &serverPartialHash=filesListForSize.value(fileName).partialHash;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(serverPartialHash==0)
                {
                    std::cerr << QStringLiteral("serverPartialHash==0 at %1").arg(__FILE__).arg(__LINE__).toStdString() << std::endl;
                    abort();
                }
                #endif
                if(clientPartialHash==serverPartialHash)
                    addDatapackListReply(false);//file found don't need be updated
                else
                {
                    QFile file(datapackPath+fileName);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        addDatapackListReply(false);//found but need an update
                        datapckFileNumber++;
                        datapckFileSize+=file.size();
                        FileToSend fileToSend;
                        fileToSend.file=fileName;
                        fileToSendList << fileToSend;
                        file.close();
                    }
                }
                filesListForSize.remove(fileName);
            }
            else
                addDatapackListReply(true);//to delete
            index++;
        }
        QHashIterator<QString,DatapackCacheFile> i(filesListForSize);
        while (i.hasNext()) {
            i.next();
            QFile file(datapackPath+i.key());
            if(file.open(QIODevice::ReadOnly))
            {
                datapckFileNumber++;
                datapckFileSize+=file.size();
                FileToSend fileToSend;
                fileToSend.file=i.key();
                fileToSendList << fileToSend;
                file.close();
            }
        }
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint32)datapckFileNumber;
        out << (quint32)datapckFileSize;
        sendFullPacket(0xC2,0x0C,outputData.constData(),outputData.size());
    }
    if(fileToSendList.isEmpty())
    {
        std::cerr << "Ask datapack list where the checksum match" << std::endl;
        return;
    }
    qSort(fileToSendList);
    //validate, remove or update the file actualy on the client
    if(tempDatapackListReplyTestCount!=files.size())
    {
        std::cerr << "Bit count return not match" << std::endl;
        return;
    }
    //send not in the list
    {
        int index=0;
        while(index<fileToSendList.size())
        {
            if(!sendFile(datapackPath,fileToSendList.at(index).file))
                return;
            index++;
        }
    }
    sendFileContent();
    sendCompressedFileContent();
    purgeDatapackListReply(query_id);

    switch(datapackStatus)
    {
        case DatapackStatus::Base:
            datapackStatus=DatapackStatus::Main;
        break;
        case DatapackStatus::Main:
            datapackStatus=DatapackStatus::Sub;
        break;
        case DatapackStatus::Sub:
            datapackStatus=DatapackStatus::Finished;
        break;
        default:
        return;
    }
}

bool CatchChallenger::operator<(const FileToSend &fileToSend1,const FileToSend &fileToSend2)
{
    if(fileToSend1.file<fileToSend2.file)
        return false;
    return true;
}

void EpollClientLoginSlave::addDatapackListReply(const bool &fileRemove)
{
    tempDatapackListReplyTestCount++;
    switch(tempDatapackListReplySize)
    {
        case 0:
            if(fileRemove)
                tempDatapackListReply|=0x01;
            else
                tempDatapackListReply&=~0x01;
        break;
        case 1:
            if(fileRemove)
                tempDatapackListReply|=0x02;
            else
                tempDatapackListReply&=~0x02;
        break;
        case 2:
            if(fileRemove)
                tempDatapackListReply|=0x04;
            else
                tempDatapackListReply&=~0x04;
        break;
        case 3:
            if(fileRemove)
                tempDatapackListReply|=0x08;
            else
                tempDatapackListReply&=~0x08;
        break;
        case 4:
            if(fileRemove)
                tempDatapackListReply|=0x10;
            else
                tempDatapackListReply&=~0x10;
        break;
        case 5:
            if(fileRemove)
                tempDatapackListReply|=0x20;
            else
                tempDatapackListReply&=~0x20;
        break;
        case 6:
            if(fileRemove)
                tempDatapackListReply|=0x40;
            else
                tempDatapackListReply&=~0x40;
        break;
        case 7:
            if(fileRemove)
                tempDatapackListReply|=0x80;
            else
                tempDatapackListReply&=~0x80;
        break;
        default:
        break;
    }
    tempDatapackListReplySize++;
    if(tempDatapackListReplySize>=8)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
}

void EpollClientLoginSlave::purgeDatapackListReply(const quint8 &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray[tempDatapackListReplyArray.size()]=tempDatapackListReply;
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.isEmpty())
        tempDatapackListReplyArray[0x00]=0x00;
    postReply(query_id,tempDatapackListReplyArray.constData(),tempDatapackListReplyArray.size());
    tempDatapackListReplyArray.clear();
}

void EpollClientLoginSlave::sendFileContent()
{
    if(EpollClientLoginSlave::rawFilesBuffer.size()>0 && EpollClientLoginSlave::rawFilesBufferCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)EpollClientLoginSlave::rawFilesBufferCount;
        const QByteArray newData(outputData+EpollClientLoginSlave::rawFilesBuffer);
        sendFullPacket(0xC2,0x03,newData.constData(),newData.size());
        EpollClientLoginSlave::rawFilesBuffer.clear();
        EpollClientLoginSlave::rawFilesBufferCount=0;
    }
}

void EpollClientLoginSlave::sendCompressedFileContent()
{
    if(EpollClientLoginSlave::compressedFilesBuffer.size()>0 && EpollClientLoginSlave::compressedFilesBufferCount>0)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)EpollClientLoginSlave::compressedFilesBufferCount;
        const QByteArray newData(outputData+EpollClientLoginSlave::compressedFilesBuffer);
        sendFullPacket(0xC2,0x04,newData.constData(),newData.size());
        EpollClientLoginSlave::compressedFilesBuffer.clear();
        EpollClientLoginSlave::compressedFilesBufferCount=0;
    }
}

bool EpollClientLoginSlave::sendFile(const QString &datapackPath,const QString &fileName)
{
    if(fileName.size()>255 || fileName.isEmpty())
    {
        std::cerr << "Unable to open into CatchChallenger::sendFile(): fileName.size()>255 || fileName.isEmpty()" << std::endl;
        return false;
    }
    const QByteArray &fileNameRaw=FacilityLibGeneral::toUTF8WithHeader(fileName);
    if(fileNameRaw.size()>255 || fileNameRaw.isEmpty())
    {
        std::cerr << "Unable to open into CatchChallenger::sendFile(): fileNameRaw.size()>255 || fileNameRaw.isEmpty()" << std::endl;
        return false;
    }
    QFile file(datapackPath+fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        const QByteArray &content=file.readAll();
        const int &contentsize=content.size();
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint32)contentsize;
        const QString &suffix=QFileInfo(file).suffix();
        if(EpollClientLoginSlave::compressedExtension.contains(suffix) &&
                ProtocolParsing::compressionTypeServer!=ProtocolParsing::CompressionType::None &&
                (
                    contentsize<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
                    ||
                    contentsize>CATCHCHALLENGER_MAX_PACKET_SIZE
                )
            )
        {
            EpollClientLoginSlave::compressedFilesBuffer+=fileNameRaw+outputData+content;
            EpollClientLoginSlave::compressedFilesBufferCount++;
            switch(ProtocolParsing::compressionTypeServer)
            {
                case ProtocolParsing::CompressionType::Xz:
                if(EpollClientLoginSlave::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || EpollClientLoginSlave::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
                default:
                case ProtocolParsing::CompressionType::Zlib:
                if(EpollClientLoginSlave::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || EpollClientLoginSlave::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
            }
        }
        else
        {
            if(contentsize>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if((1+fileNameRaw.size()+outputData.size()+contentsize)>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                {
                    std::cerr << "Unable to open into CatchChallenger::sendFile(): " << file.errorString().toStdString() << std::endl;
                    std::cerr << QString("Error: outputData2(%1)+fileNameRaw(%2)+outputData(%3)+content(%4)>CATCHCHALLENGER_MAX_PACKET_SIZE for file %5")
                                 .arg(1)
                                 .arg(fileNameRaw.size())
                                 .arg(outputData.size())
                                 .arg(contentsize)
                                 .arg(fileName)
                                 .toStdString() << std::endl;
                }
                #endif
                QByteArray outputData2;
                outputData2[0x00]=0x01;
                const QByteArray newData(outputData2+fileNameRaw+outputData+content);
                sendFullPacket(0xC2,0x03,newData.constData(),newData.size());
            }
            else
            {
                EpollClientLoginSlave::rawFilesBuffer+=fileNameRaw+outputData+content;
                EpollClientLoginSlave::rawFilesBufferCount++;
                if(EpollClientLoginSlave::rawFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || EpollClientLoginSlave::rawFilesBufferCount>=255)
                    sendFileContent();
            }
        }
        file.close();
        return true;
    }
    else
    {
        std::cerr << "Unable to open into CatchChallenger::sendFile(): " << file.errorString().toStdString() << std::endl;
        return false;
    }
}
#endif
