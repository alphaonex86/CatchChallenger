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

QThread DatapackChecksum::thread;

DatapackChecksum::DatapackChecksum()
{
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
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

void DatapackChecksum::doDifferedChecksumBase(const QString &datapackPath)
{
    QRegularExpression excludePath("^map[/\\\\]main[/\\\\]");
    QList<quint32> partialHashList;
    QStringList datapackFilesList;

    //do the by file partial hash
    {
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
                            datapackFilesList << returnList.at(index);

                            const QByteArray &data=file.readAll();
                            {
                                QCryptographicHash hashFile(QCryptographicHash::Sha224);
                                hashFile.addData(data);
                                partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                            }
                            file.close();
                        }
                        else
                        {
                            partialHashList << 0;
                            qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        }
                    }
                }
            }
            index++;
        }
    }

    emit datapackChecksumDoneBase(datapackFilesList,doChecksumBase(datapackPath),partialHashList);
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

void DatapackChecksum::doDifferedChecksumMain(const QString &datapackPath)
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
    }

    QRegularExpression excludePath("^sub[/\\\\]");
    QList<quint32> partialHashList;
    QStringList datapackFilesList;

    //do the by file partial hash
    {
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
                            datapackFilesList << returnList.at(index);

                            const QByteArray &data=file.readAll();
                            {
                                QCryptographicHash hashFile(QCryptographicHash::Sha224);
                                hashFile.addData(data);
                                partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                            }
                            file.close();
                        }
                        else
                        {
                            partialHashList << 0;
                            qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        }
                    }
                }
            }
            index++;
        }
    }

    emit datapackChecksumDoneMain(datapackFilesList,doChecksumMain(datapackPath),partialHashList);
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

void DatapackChecksum::doDifferedChecksumSub(const QString &datapackPath)
{
    QList<quint32> partialHashList;
    QStringList datapackFilesList;

    //do the by file partial hash
    {
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
                            datapackFilesList << returnList.at(index);

                            const QByteArray &data=file.readAll();
                            {
                                QCryptographicHash hashFile(QCryptographicHash::Sha224);
                                hashFile.addData(data);
                                partialHashList << *reinterpret_cast<const int *>(hashFile.result().constData());
                            }
                            file.close();
                        }
                        else
                        {
                            partialHashList << 0;
                            qDebug() << QStringLiteral("Unable to open the file to do the checksum: %1").arg(file.fileName());
                        }
                    }
                }
            }
            index++;
        }
    }

    emit datapackChecksumDoneSub(datapackFilesList,doChecksumSub(datapackPath),partialHashList);
}
