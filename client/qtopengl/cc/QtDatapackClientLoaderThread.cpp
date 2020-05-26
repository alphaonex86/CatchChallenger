#include "QtDatapackClientLoaderThread.hpp"
#include "QtDatapackClientLoader.hpp"

QtDatapackClientLoaderThread::QtDatapackClientLoaderThread()
{
}


void QtDatapackClientLoaderThread::run()
{
    const std::string basepath=QtDatapackClientLoader::datapackLoader->getDatapackPath()+"/monsters/";
    while(1)
    {
        std::unordered_map<uint16_t,QtDatapackClientLoader::ImageItemExtra> ImageitemsExtra;
        std::unordered_map<uint16_t,QtDatapackClientLoader::ImageMonsterExtra> ImagemonsterExtra;
        std::vector<uint16_t> ImageitemsToLoad;
        std::vector<uint16_t> ImagemonsterToLoad;
        //load the work
        {
            QMutexLocker a(&QtDatapackClientLoader::datapackLoader->mutex);
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
            if(ImageitemsToLoad.empty() && ImagemonsterExtra.empty())
                return;
        }
        unsigned int index=0;
        while(index<ImagemonsterToLoad.size())
        {
            const uint16_t &id=ImagemonsterToLoad.at(index);
            QtDatapackClientLoader::MonsterExtra monsterExtraEntry=QtDatapackClientLoader::datapackLoader->monsterExtra.at(id);
            QtDatapackClientLoader::ImageMonsterExtra entry;
            entry.front=QImage(QString::fromStdString(monsterExtraEntry.frontPath));
            if(entry.front.isNull())
            {
                monsterExtraEntry.frontPath=basepath+std::to_string(id)+"/front.gif";
                entry.front=QImage(QString::fromStdString(monsterExtraEntry.frontPath));
                if(entry.front.isNull())
                {
                    monsterExtraEntry.frontPath=":/CC/images/monsters/default/front.png";
                    entry.front=QImage(QString::fromStdString(monsterExtraEntry.frontPath));
                }
            }
            entry.back=QImage(QString::fromStdString(monsterExtraEntry.backPath));
            if(entry.back.isNull())
            {
                monsterExtraEntry.backPath=basepath+std::to_string(id)+"/back.gif";
                entry.back=QImage(QString::fromStdString(monsterExtraEntry.backPath));
                if(entry.back.isNull())
                {
                    monsterExtraEntry.backPath=":/CC/images/monsters/default/back.png";
                    entry.back=QImage(QString::fromStdString(monsterExtraEntry.backPath));
                }
            }
            entry.thumb=QImage(QString::fromStdString(basepath+std::to_string(id)+"/small.png"));
            if(entry.thumb.isNull())
            {
                entry.thumb=QImage(QString::fromStdString(basepath+std::to_string(id)+"/small.gif"));
                if(entry.thumb.isNull())
                    entry.thumb=QImage(":/CC/images/monsters/default/small.png");
            }
            entry.thumb=entry.thumb.scaled(64,64);
            ImagemonsterExtra[id]=entry;
            index++;
        }

        index=0;
        while(index<ImageitemsToLoad.size())
        {
            const uint16_t &id=ImageitemsToLoad.at(index);
            QtDatapackClientLoader::ImageItemExtra entry;
            const DatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->itemsExtra.at(id);
            entry.image=QImage(QString::fromStdString(extra.imagePath));
            ImageitemsExtra[id]=entry;
            index++;
        }
        //commit the work
        {
            QMutexLocker a(&QtDatapackClientLoader::datapackLoader->mutex);
            index=0;
            while(index<ImageitemsToLoad.size())
            {
                const uint16_t &key=ImageitemsToLoad.at(index);
                QtDatapackClientLoader::datapackLoader->ImageitemsExtra[key]=ImageitemsExtra.at(key);
                index++;
            }
            ImageitemsToLoad.clear();
            index=0;
            while(index<ImagemonsterToLoad.size())
            {
                const uint16_t &key=ImagemonsterToLoad.at(index);
                QtDatapackClientLoader::datapackLoader->ImagemonsterExtra[key]=ImagemonsterExtra.at(key);
                index++;
            }
            ImagemonsterToLoad.clear();
        }
    }
}
