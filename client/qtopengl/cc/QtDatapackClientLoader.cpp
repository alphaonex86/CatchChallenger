#include "QtDatapackClientLoader.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/DatapackGeneralLoader.hpp"
#include "../../qt/QtDatapackChecksum.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/DatapackGeneralLoader.hpp"
#include "../../../general/base/Map_loader.hpp"
#include "../../qt/tiled/tiled_tileset.hpp"
#include "../../qt/tiled/tiled_mapreader.hpp"
#include "../../qt/FacilityLibClient.hpp"
#include "../Language.hpp"

#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <iostream>

QtDatapackClientLoader *QtDatapackClientLoader::datapackLoader=nullptr;

QtDatapackClientLoader::QtDatapackClientLoader()
{
    mDefaultInventoryImage=NULL;
    #ifndef NOTHREADS
    start();
    moveToThread(this);
    #endif
    setObjectName("DatapackClientLoader");
}

QtDatapackClientLoader::~QtDatapackClientLoader()
{
    if(mDefaultInventoryImage==NULL)
        delete mDefaultInventoryImage;
    #ifndef NOTHREADS
    quit();
    wait();
    #endif
}

QPixmap QtDatapackClientLoader::defaultInventoryImage()
{
    return *mDefaultInventoryImage;
}

#ifndef NOTHREADS
void QtDatapackClientLoader::run()
{
    exec();
}
#endif

void QtDatapackClientLoader::parseDatapack(const std::string &datapackPath)
{
    if(inProgress)
    {
        qDebug() << QStringLiteral("already in progress");
        return;
    }
    DatapackClientLoader::parseDatapack(datapackPath);
    parseMonstersExtra();
    parseBuffExtra();
    parsePlantsExtra();
    inProgress=false;
    emit datapackParsed();
}

void QtDatapackClientLoader::parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode)
{
    DatapackClientLoader::parseDatapackMainSub(mainDatapackCode,subDatapackCode);
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/CC/images/inventory/unknown-object.png"));
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(
                datapackPath,mainDatapackCode,subDatapackCode);

    parseTileset();

    inProgress=false;

    emit datapackParsedMainSub();
}

void QtDatapackClientLoader::emitdatapackParsed()
{
    emit datapackParsed();
}

void QtDatapackClientLoader::emitdatapackParsedMainSub()
{
    emit datapackParsedMainSub();
}

void QtDatapackClientLoader::emitdatapackChecksumError()
{
    emit datapackChecksumError();
}

void QtDatapackClientLoader::parseTopLib()
{
    parseTileset();
}

void QtDatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    if(mDefaultInventoryImage==NULL)
    {
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/CC/images/inventory/unknown-object.png"));
        if(mDefaultInventoryImage->isNull())
        {
            qDebug() << "default internal image bug for item";
            abort();
        }
    }
    for (const auto &n : QtplantExtra)
        delete n.second.tileset;
    QtplantExtra.clear();
    {
         QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
         while (i.hasNext()) {
             i.next();
             delete i.value();
         }
         Tiled::Tileset::preloadedTileset.clear();
    }
    QtitemsExtra.clear();
    QtmonsterExtra.clear();
    QtmonsterBuffsExtra.clear();
    QtplantExtra.clear();
    DatapackClientLoader::resetAll();
}

void QtDatapackClientLoader::parseTileset()
{
    const std::vector<std::string> &fileList=
                CatchChallenger::FacilityLibGeneral::listFolder(datapackPath+DATAPACK_BASE_PATH_MAPBASE);
    unsigned int index=0;
    while(index<fileList.size())
    {
        const std::string &filePath=fileList.at(index);
        if(stringEndsWith(filePath,".tsx"))
        {
            const std::string &source=QFileInfo(QString::fromStdString(datapackPath+DATAPACK_BASE_PATH_MAPBASE+filePath))
                    .absoluteFilePath().toStdString();
            QFile file(QString::fromStdString(source));
            if(file.open(QIODevice::ReadOnly))
            {
                Tiled::MapReader mapReader;
                Tiled::Tileset *tileset = mapReader.readTileset(&file, QString::fromStdString(source));
                if (tileset)
                {
                    tileset->setFileName(QString::fromStdString(source));
                    Tiled::Tileset::preloadedTileset[QString::fromStdString(source)]=tileset;
                }
                file.close();
            }
            else
                qDebug() << QStringLiteral("Tileset: %1 can't be open: %2")
                            .arg(QString::fromStdString(source))
                            .arg(file.errorString());
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 tileset(s) loaded").arg(Tiled::Tileset::preloadedTileset.size());
}

void QtDatapackClientLoader::parseItemsExtra()
{
    DatapackClientLoader::parseItemsExtra();
    for( const auto& n : itemsExtra ) {
        QString path=QString::fromStdString(n.second.imagePath);
        QPixmap pix(path);
        if(pix.isNull())
            qDebug() << "bug: " << path;
        else
            QtitemsExtra[n.first].image=pix;
    }
}

std::string QtDatapackClientLoader::getLanguage()
{
    #ifndef CATCHCHALLENGER_BOT
    return Language::language.getLanguage().toStdString();
    #else
    return "en";
    #endif
}

std::string QtDatapackClientLoader::getSkinPath(const std::string &skinName,const std::string &type) const
{
    {
        QFileInfo pngFile(QString::fromStdString(getDatapackPath())+
                          DATAPACK_BASE_PATH_SKIN+QString::fromStdString(skinName)+
                          QStringLiteral("/%1.png").arg(QString::fromStdString(type)));
        if(pngFile.exists())
            return pngFile.absoluteFilePath().toStdString();
    }
    {
        QFileInfo gifFile(QString::fromStdString(getDatapackPath())+
                          DATAPACK_BASE_PATH_SKIN+QString::fromStdString(skinName)+
                          QStringLiteral("/%1.gif").arg(QString::fromStdString(type)));
        if(gifFile.exists())
            return gifFile.absoluteFilePath().toStdString();
    }
    QDir folderList(QString::fromStdString(getDatapackPath())+DATAPACK_BASE_PATH_SKINBASE);
    const QStringList &entryList=folderList.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    int entryListIndex=0;
    while(entryListIndex<entryList.size())
    {
        {
            QFileInfo pngFile(QStringLiteral("%1/skin/%2/%3/%4.png")
                              .arg(QString::fromStdString(getDatapackPath()))
                              .arg(entryList.at(entryListIndex))
                              .arg(QString::fromStdString(skinName))
                              .arg(QString::fromStdString(type)));
            if(pngFile.exists())
                return pngFile.absoluteFilePath().toStdString();
        }
        {
            QFileInfo gifFile(QStringLiteral("%1/skin/%2/%3/%4.gif")
                              .arg(QString::fromStdString(getDatapackPath()))
                              .arg(entryList.at(entryListIndex))
                              .arg(QString::fromStdString(skinName))
                              .arg(QString::fromStdString(type)));
            if(gifFile.exists())
                return gifFile.absoluteFilePath().toStdString();
        }
        entryListIndex++;
    }
    return std::string();
}

std::string QtDatapackClientLoader::getFrontSkinPath(const uint32_t &skinId) const
{
    /// \todo merge it cache string + id
    const std::vector<std::string> &skinFolderList=QtDatapackClientLoader::datapackLoader->skins;
    //front image
    if(skinId<(uint32_t)skinFolderList.size())
        return getSkinPath(skinFolderList.at(skinId),"front");
    else
        qDebug() << "The skin id: "+QString::number(skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
    return ":/CC/images/player_default/front.png";
}
