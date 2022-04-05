#include "QtDatapackClientLoaderThread.hpp"
#include "QtDatapackClientLoader.hpp"

QtDatapackClientLoaderThread::QtDatapackClientLoaderThread()
{
    stopIt=false;
}

QtDatapackClientLoaderThread::~QtDatapackClientLoaderThread()
{
    stopIt=true;
    #ifndef NOTHREADS
    exit();
    wait();
    #endif
}

void QtDatapackClientLoaderThread::stop()
{
    stopIt=true;
    #ifndef NOTHREADS
    exit();
    #endif
}

void QtDatapackClientLoaderThread::run()
{
    stopIt=false;
    const std::string basepath=QtDatapackClientLoader::datapackLoader->getDatapackPath()+"/monsters/";
    while(1)
    {
        std::vector<uint16_t> ImageitemsToLoad;
        std::vector<uint16_t> ImagemonsterToLoad;
        //load the work
        {
            #ifndef NOTHREADS
            QMutexLocker a(&QtDatapackClientLoader::datapackLoader->mutex);
            #endif
            while(ImageitemsToLoad.size()<20 && !QtDatapackClientLoader::datapackLoader->ImageitemsToLoad.empty())
            {
                ImageitemsToLoad.push_back(QtDatapackClientLoader::datapackLoader->ImageitemsToLoad.back());
                QtDatapackClientLoader::datapackLoader->ImageitemsToLoad.pop_back();
            }
            while(ImagemonsterToLoad.size()<20 && !QtDatapackClientLoader::datapackLoader->ImagemonsterToLoad.empty())
            {
                ImagemonsterToLoad.push_back(QtDatapackClientLoader::datapackLoader->ImagemonsterToLoad.back());
                QtDatapackClientLoader::datapackLoader->ImagemonsterToLoad.pop_back();
            }
            if(ImageitemsToLoad.empty() && ImagemonsterToLoad.empty())
                return;
        }
        unsigned int index=0;
        while(index<ImagemonsterToLoad.size())
        {
            const uint16_t &id=ImagemonsterToLoad.at(index);
            QtDatapackClientLoader::MonsterExtra monsterExtraEntry=QtDatapackClientLoader::datapackLoader->monsterExtra.at(id);
            QtDatapackClientLoader::ImageMonsterExtra *entry=new QtDatapackClientLoader::ImageMonsterExtra;
            entry->front=QImage(QString::fromStdString(basepath+std::to_string(id)+"/front.png"));
            if(stopIt)
                return;
            if(entry->front.isNull())
            {
                monsterExtraEntry.frontPath=basepath+std::to_string(id)+"/front.gif";
                entry->front=QImage(QString::fromStdString(monsterExtraEntry.frontPath));
                if(stopIt)
                    return;
                if(entry->front.isNull())
                {
                    monsterExtraEntry.frontPath=":/CC/images/monsters/default/front.png";
                    entry->front=QImage(QString::fromStdString(monsterExtraEntry.frontPath));
                    if(stopIt)
                        return;
                }
            }
            if(stopIt)
                return;
            entry->back=QImage(QString::fromStdString(basepath+std::to_string(id)+"/back.png"));
            if(stopIt)
                return;
            if(entry->back.isNull())
            {
                monsterExtraEntry.backPath=basepath+std::to_string(id)+"/back.gif";
                entry->back=QImage(QString::fromStdString(monsterExtraEntry.backPath));
                if(stopIt)
                    return;
                if(entry->back.isNull())
                {
                    monsterExtraEntry.backPath=":/CC/images/monsters/default/back.png";
                    entry->back=QImage(QString::fromStdString(monsterExtraEntry.backPath));
                    if(stopIt)
                        return;
                }
            }if(stopIt)
                return;
            entry->thumb=QImage(QString::fromStdString(basepath+std::to_string(id)+"/small.png"));
            if(stopIt)
                return;
            if(entry->thumb.isNull())
            {
                entry->thumb=QImage(QString::fromStdString(basepath+std::to_string(id)+"/small.gif"));
                if(stopIt)
                    return;
                if(entry->thumb.isNull())
                    entry->thumb=QImage(":/CC/images/monsters/default/small.png");
                if(stopIt)
                    return;
            }
            if(stopIt)
                return;
            //entry->thumb=entry->thumb.scaled(64,64);
            if(stopIt)
                return;
            emit loadMonsterImage(id,entry);
            index++;
        }

        index=0;
        while(index<ImageitemsToLoad.size())
        {
            const uint16_t &id=ImageitemsToLoad.at(index);
            QtDatapackClientLoader::ImageItemExtra *entry=new QtDatapackClientLoader::ImageItemExtra;
            if(stopIt)
                return;
            if(QtDatapackClientLoader::datapackLoader!=nullptr && QtDatapackClientLoader::datapackLoader->itemsExtra.find(id)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                const DatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->itemsExtra.at(id);
                entry->image=QImage(QString::fromStdString(extra.imagePath));
                if(stopIt)
                    return;
                emit loadItemImage(id,entry);
            }
            if(stopIt)
                return;
            index++;
        }

        ImageitemsToLoad.clear();
        ImagemonsterToLoad.clear();
    }
}
