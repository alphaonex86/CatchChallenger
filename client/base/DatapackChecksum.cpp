#include "DatapackChecksum.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QSet>
#include <QDebug>

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

#ifndef QT_NO_EMIT
QThread DatapackChecksum::thread;
#endif

DatapackChecksum::DatapackChecksum()
{
    #ifndef QT_NO_EMIT
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    #endif
}

DatapackChecksum::~DatapackChecksum()
{
}

QByteArray DatapackChecksum::doChecksumBase(const QString &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);
    QRegularExpression excludePath("^map[/\\\\]main[/\\\\]");

    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return QByteArray();
                    }
                }
            }
        }
        index++;
    }
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumBase(const QString &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneBase(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumBase(const QString &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    QRegularExpression excludePath("^map[/\\\\]main[/\\\\]");
    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumBase(datapackPath);
    return fullDatapackChecksumReturn;
}

QByteArray DatapackChecksum::doChecksumMain(const QString &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);
    QRegularExpression excludePath("^sub[/\\\\]");

    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return QByteArray();
                    }
                }
            }
        }
        index++;
    }
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumMain(const QString &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneMain(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumMain(const QString &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    QRegularExpression excludePath("^sub[/\\\\]");
    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName) && !fileName.contains(excludePath))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumMain(datapackPath);
    return fullDatapackChecksumReturn;
}

QByteArray DatapackChecksum::doChecksumSub(const QString &datapackPath)
{
    QCryptographicHash hash(QCryptographicHash::Sha224);

    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        hash.addData(data);
                        file.close();
                    }
                    else
                    {
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        return QByteArray();
                    }
                }
            }
        }
        index++;
    }
    qDebug() << QStringLiteral("sub hash 1: %1").arg(QString(hash.result().toHex()));
    return hash.result();
}

#ifndef QT_NO_EMIT
void DatapackChecksum::doDifferedChecksumSub(const QString &datapackPath)
{
    const FullDatapackChecksumReturn &fullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneSub(fullDatapackChecksumReturn.datapackFilesList,fullDatapackChecksumReturn.hash,fullDatapackChecksumReturn.partialHashList);
}
#endif

DatapackChecksum::FullDatapackChecksumReturn DatapackChecksum::doFullSyncChecksumSub(const QString &datapackPath)
{
    DatapackChecksum::FullDatapackChecksumReturn fullDatapackChecksumReturn;
    const QSet<QString> &extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();
    QRegularExpression datapack_rightFileName=QRegularExpression(DATAPACK_FILE_REGEX);
    QStringList returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath);
    returnList.sort();
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        const QString &fileName=returnList.at(index);
        if(fileName.contains(datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(datapackPath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    fullDatapackChecksumReturn.datapackFilesList << returnList.at(index);
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            fullDatapackChecksumReturn.partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                        }
                        file.close();
                    }
                    else
                    {
                        fullDatapackChecksumReturn.partialHashList << 0;
                        qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                    }
                }
            }
        }
        index++;
    }
    fullDatapackChecksumReturn.hash=doChecksumSub(datapackPath);
    return fullDatapackChecksumReturn;
}
