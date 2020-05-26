#include "QtDatapackClientLoader.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/DatapackGeneralLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/DatapackGeneralLoader.hpp"
#include "../../../general/base/Map_loader.hpp"
#include "../../qt/QtDatapackChecksum.hpp"
#include "../../qt/tiled/tiled_tileset.hpp"
#include "../../qt/tiled/tiled_mapreader.hpp"
#include "../../qt/FacilityLibClient.hpp"
#include "../../qt/Settings.hpp"
#include "../Language.hpp"
#ifdef CATCHCHALLENGER_CACHE_HPS
#include "../../../general/hps/hps.h"
#include <fstream>
#include <QStandardPaths>
#endif
#include "QtDatapackClientLoaderThread.hpp"
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
    auto start_time = std::chrono::high_resolution_clock::now();

    std::string hash;
    if(Settings::settings->contains("DatapackHashBase-"+QString::fromStdString(datapackPath)))
    {
        const QByteArray &data=QByteArray::fromHex(Settings::settings->value("DatapackHashBase-"+QString::fromStdString(datapackPath)).toString().toUtf8());
        hash=std::string(data.constData(),data.size());
    }
    #ifdef CATCHCHALLENGER_CACHE_HPS
    this->language=Language::language.getLanguage().toStdString();
    std::string cachepath;
    std::string cachepathdir;
    QStringList l=QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    if(!l.empty())
    {
        std::string b=datapackPath;
        if(stringEndsWith(b,"/"))
            b=b.substr(0,b.size()-1);
        b+="-cache/";
        cachepathdir=b;

        QDir().mkdir(QString::fromStdString(cachepathdir));
        cachepath=cachepathdir+"datapackbase-"+this->language+".cache";
    }

        this->datapackPath=datapackPath;
        DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN "na/";
        DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=std::string(DATAPACK_BASE_PATH_MAPSUB1)+"na/"+std::string(DATAPACK_BASE_PATH_MAPSUB2)+"nabis/";

        bool loaded=false;
        if(!cachepath.empty())
        {
            std::ifstream in_file(cachepath, std::ifstream::binary);
            if(in_file.good() && in_file.is_open())
            {
                auto start_time = std::chrono::high_resolution_clock::now();
                hps::StreamInputBuffer serialBuffer(in_file);
                serialBuffer >> CatchChallenger::CommonDatapack::commonDatapack;
                serialBuffer >> visualCategories;
                serialBuffer >> typeExtra;
                serialBuffer >> itemsExtra;
                serialBuffer >> skins;
                serialBuffer >> monsterSkillsExtra;
                serialBuffer >> audioAmbiance;
                serialBuffer >> reputationExtra;
                serialBuffer >> monsterExtra;
                serialBuffer >> monsterBuffsExtra;
                auto end_time = std::chrono::high_resolution_clock::now();
                auto time = end_time - start_time;
                std::cout << "CatchChallenger::CommonDatapack::commonDatapack.parseDatapack() took " << time/std::chrono::milliseconds(1) << "ms to parse " << datapackPath << std::endl;
                loaded=true;
            }
        }
        if(!loaded)
        {
        #endif

        DatapackClientLoader::parseDatapack(datapackPath,hash,Language::language.getLanguage().toStdString());
        parseMonstersExtra();
        parseBuffExtra();

        #ifdef CATCHCHALLENGER_CACHE_HPS
            if(!cachepath.empty())
            {
                std::ofstream out_file(cachepath, std::ofstream::binary);
                if(out_file.good() && out_file.is_open())
                {
                    hps::to_stream(CatchChallenger::CommonDatapack::commonDatapack, out_file);
                    hps::to_stream(visualCategories, out_file);
                    hps::to_stream(typeExtra, out_file);
                    hps::to_stream(itemsExtra, out_file);
                    hps::to_stream(skins, out_file);
                    hps::to_stream(monsterSkillsExtra, out_file);
                    hps::to_stream(audioAmbiance, out_file);
                    hps::to_stream(reputationExtra, out_file);
                    hps::to_stream(monsterExtra, out_file);
                    hps::to_stream(monsterBuffsExtra, out_file);
                    hps::to_stream(CatchChallenger::CommonDatapack::commonDatapack.monstersCollisionTemp, out_file);
                }
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            auto time = end_time - start_time;
            std::cout << "CatchChallenger::CommonDatapack::commonDatapack.parseDatapack() took " << time/std::chrono::milliseconds(1) << "ms to parse " << datapackPath << std::endl;
        }

    #endif

    parsePlantsExtra();

    for( const auto& n : QtDatapackClientLoader::datapackLoader->monsterBuffsExtra )
    {
        const uint8_t &id=n.first;
        QtBuff QtmonsterBuffExtraEntry;
        const std::string basePath=datapackPath+DATAPACK_BASE_PATH_BUFFICON+std::to_string(id);
        const std::string pngFile=basePath+".png";
        const std::string gifFile=basePath+".gif";
        if(QFile(QString::fromStdString(pngFile)).exists())
            QtmonsterBuffExtraEntry.icon=QPixmap(QString::fromStdString(pngFile));
        else if(QFile(QString::fromStdString(gifFile)).exists())
            QtmonsterBuffExtraEntry.icon=QPixmap(QString::fromStdString(gifFile));
        if(QtmonsterBuffExtraEntry.icon.isNull())
            QtmonsterBuffExtraEntry.icon=QPixmap(QStringLiteral(":/CC/images/interface/buff.png"));
        QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra[id]=QtmonsterBuffExtraEntry;
    }

    for( const auto& n : itemsExtra )
        ImageitemsToLoad.push_back(n.first);
    for( const auto& n : monsterExtra )
        ImagemonsterToLoad.push_back(n.first);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "CatchChallenger::CommonDatapack::commonDatapack.parseDatapack end took " << time/std::chrono::milliseconds(1) << "ms to parse " << datapackPath << std::endl;
    startThread();
}

void QtDatapackClientLoader::startThread()
{
    const int tc=QThread::idealThreadCount();
    threads.clear();
    std::vector<QtDatapackClientLoaderThread *> threads;
    int index=0;
    while(index<tc || index==0)//always create 1 thread
    {
        QtDatapackClientLoaderThread *t=new QtDatapackClientLoaderThread();
        connect(t,&QThread::finished,this,&QtDatapackClientLoader::threadFinished,Qt::QueuedConnection);
        this->threads.insert(t);
        threads.push_back(t);
        index++;
    }
    index=0;
    {
        unsigned int index=0;
        while(index<threads.size())
        {
            threads.at(index)->start(QThread::LowestPriority);
            index++;
        }
    }
}

void QtDatapackClientLoader::threadFinished()
{
    QtDatapackClientLoaderThread *thread = qobject_cast<QtDatapackClientLoaderThread *>(sender());
    if(thread==nullptr)
        abort();
    if(threads.find(thread)!=threads.cend())
        threads.erase(thread);
    else
    {
        std::cerr << "GameLoader::threadFinished() thread not found" << std::endl;
        abort();
        return;
    }
    /*not need, no event loop: thread->exit();
    thread->quit();
    thread->terminate();*/
    thread->deleteLater();
    if(threads.empty())
    {
        QMutexLocker a(&mutex);
        inProgress=false;
        emit datapackParsed();
    }
}

void QtDatapackClientLoader::parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string hashMain;

    std::string datapackPathMain=datapackPath+DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode;
    std::string datapackPathSub=datapackPath+std::string(DATAPACK_BASE_PATH_MAPSUB1)+mainDatapackCode+std::string(DATAPACK_BASE_PATH_MAPSUB2)+subDatapackCode;

    if(Settings::settings->contains("DatapackHashMain-"+QString::fromStdString(datapackPathMain)))
    {
        const QByteArray &data=QByteArray::fromHex(Settings::settings->value("DatapackHashMain-"+QString::fromStdString(datapackPathMain)).toString().toUtf8());
        hashMain=std::string(data.constData(),data.size());
    }
    std::string hashSub;
    if(Settings::settings->contains("DatapackHashSub-"+QString::fromStdString(datapackPathSub)))
    {
        const QByteArray &data=QByteArray::fromHex(Settings::settings->value("DatapackHashSub-"+QString::fromStdString(datapackPathSub)).toString().toUtf8());
        hashSub=std::string(data.constData(),data.size());
    }
    DatapackClientLoader::parseDatapackMainSub(mainDatapackCode,subDatapackCode,hashMain,hashSub);
    if(mDefaultInventoryImage==NULL)
        mDefaultInventoryImage=new QPixmap(QStringLiteral(":/CC/images/inventory/unknown-object.png"));
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(
                datapackPath,mainDatapackCode,subDatapackCode);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "parseDatapackMainSub took " << time/std::chrono::milliseconds(1) << "ms to parse " << datapackPath <<
                 "(" << mainDatapackCode << "," << subDatapackCode << ")" << std::endl;
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
    ImageitemsExtra.clear();
    ImagemonsterExtra.clear();
    ImageitemsToLoad.clear();
    ImagemonsterToLoad.clear();
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

void QtDatapackClientLoader::parseMonstersExtra()
{
    uint64_t imagens=0;
    auto start_time = std::chrono::high_resolution_clock::now();
    QDir dir(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MONSTERS);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        tinyxml2::XMLDocument *domDocument=NULL;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=
                CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"monsters")!=0)
        {
            //qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"monsters\" root balise not found for the xml file").arg(QString::fromStdString(file)));
            file_index++;
            continue;
        }

        #ifndef CATCHCHALLENGER_BOT
        const std::string &language=Language::language.getLanguage().toStdString();
        #else
        const std::string language("en");
        #endif
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("monster");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t tempid=stringtouint32(item->Attribute("id"),&ok);
                if(!ok)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                else
                {
                    const uint16_t id=static_cast<uint16_t>(tempid);
                    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(id)
                            ==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster list into monster extra: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        QtDatapackClientLoader::MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        {
                            bool found=false;
                            const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.name=name->GetText();
                                        found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("name");
                                }
                            if(!found)
                            {
                                name = item->FirstChildElement("name");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                        if(name->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.name=name->GetText();
                                            break;
                                        }
                                    name = name->NextSiblingElement("name");
                                }
                            }
                        }
                        {
                            bool found=false;
                            const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                            if(!language.empty() && language!="en")
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language && description->GetText()!=NULL)
                                    {
                                            monsterExtraEntry.description=description->GetText();
                                            found=true;
                                            break;
                                    }
                                    description = description->NextSiblingElement("description");
                                }
                            if(!found)
                            {
                                description = item->FirstChildElement("description");
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                        if(description->GetText()!=NULL)
                                        {
                                                monsterExtraEntry.description=description->GetText();
                                                break;
                                        }
                                    description = description->NextSiblingElement("description");
                                }
                            }
                        }
                        //load the kind
                        {
                            bool kind_found=false;
                            const tinyxml2::XMLElement *kind = item->FirstChildElement("kind");
                            if(!language.empty() && language!="en")
                                while(kind!=NULL)
                                {
                                    if(kind->Attribute("lang")!=NULL && kind->Attribute("lang")==language && kind->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.kind=kind->GetText();
                                        kind_found=true;
                                        break;
                                    }
                                    kind = kind->NextSiblingElement("kind");
                                }
                            if(!kind_found)
                            {
                                kind = item->FirstChildElement("kind");
                                while(kind!=NULL)
                                {
                                    if(kind->Attribute("lang")==NULL || strcmp(kind->Attribute("lang"),"en")==0)
                                        if(kind->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.kind=kind->GetText();
                                            kind_found=true;
                                            break;
                                        }
                                    kind = kind->NextSiblingElement("kind");
                                }
                            }
                        }

                        //load the habitat
                        {
                            bool habitat_found=false;
                            const tinyxml2::XMLElement *habitat = item->FirstChildElement("habitat");
                            if(!language.empty() && language!="en")
                                while(habitat!=NULL)
                                {
                                    if(habitat->Attribute("lang") && habitat->Attribute("lang")==language && habitat->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.habitat=habitat->GetText();
                                        habitat_found=true;
                                        break;
                                    }
                                    habitat = habitat->NextSiblingElement("habitat");
                                }
                            if(!habitat_found)
                            {
                                habitat = item->FirstChildElement("habitat");
                                while(habitat!=NULL)
                                {
                                    if(habitat->Attribute("lang")==NULL || strcmp(habitat->Attribute("lang"),"en")==0)
                                        if(habitat->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.habitat=habitat->GetText();
                                            habitat_found=true;
                                            break;
                                        }
                                    habitat = habitat->NextSiblingElement("habitat");
                                }
                            }
                        }
                        const std::string basepath=datapackPath+"/"+DATAPACK_BASE_PATH_MONSTERS+std::to_string(id);
                        if(monsterExtraEntry.name.empty())
                            monsterExtraEntry.name=tr("Unknown").toStdString();
                        if(monsterExtraEntry.description.empty())
                            monsterExtraEntry.description=tr("Unknown").toStdString();
                        auto start_time2 = std::chrono::high_resolution_clock::now();
                        monsterExtraEntry.frontPath=basepath+"/front.png";

                        QtDatapackClientLoader::datapackLoader->monsterExtra[id]=monsterExtraEntry;
                        auto end_time2 = std::chrono::high_resolution_clock::now();
                        auto time2 = end_time2 - start_time2;
                        imagens+=time2/std::chrono::nanoseconds(1);
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster id at monster extra: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement("monster");
        }

        file_index++;
    }

    auto start_time2 = std::chrono::high_resolution_clock::now();
    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsters.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(i->first)==
                QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i->first));
            QtDatapackClientLoader::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown").toStdString();
            monsterExtraEntry.description=tr("Unknown").toStdString();
            QtDatapackClientLoader::datapackLoader->monsterExtra[i->first]=monsterExtraEntry;
        }
        ++i;
    }
    auto end_time2 = std::chrono::high_resolution_clock::now();
    auto time2 = end_time2 - start_time2;
    imagens+=time2/std::chrono::nanoseconds(1);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->monsterExtra.size()) << "into" << time/std::chrono::milliseconds(1)-imagens/1000000
             << "ms/" << imagens/1000000 << "ms";
}


void QtDatapackClientLoader::parseBuffExtra()
{
    QDir dir(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_BUFF);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        const std::string dotpng(".png");
        const std::string dotgif(".gif");
        tinyxml2::XMLDocument *domDocument;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=
                CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"buffs")!=0)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"buffs\" root balise not found for the xml file")
                         .arg(QString::fromStdString(file)));
            file_index++;
            continue;
        }

        #ifndef CATCHCHALLENGER_BOT
        const std::string &language=Language::language.getLanguage().toStdString();
        #else
        const std::string language("en");
        #endif
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("buff");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t tempid=stringtouint8(item->Attribute("id"),&ok);
                if(!ok || tempid>255)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                else
                {
                    const uint8_t &id=static_cast<uint8_t>(tempid);
                    if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(id)==
                            CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster buff list into buff extra: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        QtDatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        bool found=false;
                        const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                        if(!language.empty() && language!="en")
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                {
                                    monsterBuffExtraEntry.name=name->GetText();
                                    found=true;
                                    break;
                                }
                                name = name->NextSiblingElement("name");
                            }
                        if(!found)
                        {
                            name = item->FirstChildElement("name");
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    if(name->GetText()!=NULL)
                                    {
                                        monsterBuffExtraEntry.name=name->GetText();
                                        break;
                                    }
                                name = name->NextSiblingElement("name");
                            }
                        }
                        found=false;
                        const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                        if(!language.empty() && language!="en")
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang") && description->Attribute("lang")==language && description->GetText()!=NULL)
                                {
                                    monsterBuffExtraEntry.description=description->GetText();
                                    found=true;
                                    break;
                                }
                                description = description->NextSiblingElement("description");
                            }
                        if(!found)
                        {
                            description = item->FirstChildElement("description");
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                    if(description->GetText()!=NULL)
                                    {
                                        monsterBuffExtraEntry.description=description->GetText();
                                        break;
                                    }
                                description = description->NextSiblingElement("description");
                            }
                        }
                        if(monsterBuffExtraEntry.name.empty())
                            monsterBuffExtraEntry.name=tr("Unknown").toStdString();
                        if(monsterBuffExtraEntry.description.empty())
                            monsterBuffExtraEntry.description=tr("Unknown").toStdString();
                        QtDatapackClientLoader::datapackLoader->monsterBuffsExtra[id]=monsterBuffExtraEntry;
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the buff id: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement("buff");
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(i->first)==
                QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster buffer extra for id: %1").arg(i->first));
            QtDatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
            QtDatapackClientLoader::QtBuff QtmonsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown").toStdString();
            monsterBuffExtraEntry.description=tr("Unknown").toStdString();
            QtmonsterBuffExtraEntry.icon=QPixmap(":/CC/images/interface/buff.png");
            QtDatapackClientLoader::datapackLoader->monsterBuffsExtra[i->first]=monsterBuffExtraEntry;
            QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra[i->first]=QtmonsterBuffExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 buff(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.size());
}

void QtDatapackClientLoader::parsePlantsExtra()
{
    const std::string &text_plant="plant";
    const std::string &text_dotpng=".png";
    const std::string &text_dotgif=".gif";
    const std::string &basePath=datapackPath+DATAPACK_BASE_PATH_PLANTS;
    auto i = CatchChallenger::CommonDatapack::commonDatapack.plants.begin();
    while (i != CatchChallenger::CommonDatapack::commonDatapack.plants.cend()) {
        //try load the tileset
        QtDatapackClientLoader::QtPlantExtra plant;
        plant.tileset = new Tiled::Tileset(QString::fromStdString(text_plant),16,32);
        const std::string &path=basePath+std::to_string(i->first)+text_dotpng;
        if(!plant.tileset->loadFromImage(QImage(QString::fromStdString(path)),QString::fromStdString(path)))
        {
            const std::string &path=basePath+std::to_string(i->first)+text_dotgif;
            if(!plant.tileset->loadFromImage(QImage(QString::fromStdString(path)),QString::fromStdString(path)))
                if(!plant.tileset->loadFromImage(QImage(QStringLiteral(":/CC/images/plant/unknow_plant.png")),QStringLiteral(":/CC/images/plant/unknow_plant.png")))
                    qDebug() << "Unable the load the default plant tileset";
        }
        QtDatapackClientLoader::datapackLoader->QtplantExtra[i->first]=plant;
        itemToPlants[CatchChallenger::CommonDatapack::commonDatapack.plants[i->first].itemUsed]=i->first;
        ++i;
    }

    qDebug() << QStringLiteral("%1 plant(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->QtplantExtra.size());
}

const QtDatapackClientLoader::QtMonsterExtra &QtDatapackClientLoader::getMonsterExtra(const uint16_t &id)
{
    if(QtmonsterExtra.find(id)!=QtmonsterExtra.cend())
        return QtmonsterExtra.at(id);
    else
    {
        if(ImagemonsterExtra.find(id)!=ImagemonsterExtra.cend())
        {
            const ImageMonsterExtra &i=ImagemonsterExtra.at(id);
            QtMonsterExtra n;
            n.front=QPixmap::fromImage(i.front);
            n.back=QPixmap::fromImage(i.back);
            n.thumb=QPixmap::fromImage(i.thumb);
            QtmonsterExtra[id]=n;
            ImagemonsterExtra.erase(id);
            return QtmonsterExtra.at(id);
        }
    }
    std::cerr << "QtDatapackClientLoader::getImage failed on: " << std::to_string(id) << std::endl;
    abort();
}

const QtDatapackClientLoader::QtItemExtra &QtDatapackClientLoader::getImageExtra(const uint16_t &id)
{
    if(QtitemsExtra.find(id)!=QtitemsExtra.cend())
        return QtitemsExtra.at(id);
    else
    {
        if(ImageitemsExtra.find(id)!=ImageitemsExtra.cend())
        {
            const ImageItemExtra &i=ImageitemsExtra.at(id);
            QtItemExtra n;
            n.image=QPixmap::fromImage(i.image);
            QtitemsExtra[id]=n;
            ImageitemsExtra.erase(id);
            return QtitemsExtra.at(id);
        }
    }
    std::cerr << "QtDatapackClientLoader::getImage failed on: " << std::to_string(id) << std::endl;
    abort();
}
