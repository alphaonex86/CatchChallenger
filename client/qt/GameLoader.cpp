#include "GameLoader.hpp"

#include <QDirIterator>
#include <QCoreApplication>
#include <iostream>
#include <QTime>

GameLoader *GameLoader::gameLoader=nullptr;

GameLoader::GameLoader()
{
    const int tc=QThread::idealThreadCount();
    std::cout << "idealThreadCount: " << tc << std::endl;
    std::vector<GameLoaderThread *> threads;
    int index=0;
    while(index<tc || index==0)//always create 1 thread
    {
        GameLoaderThread *t=new GameLoaderThread(index);
        connect(t,&QThread::finished,this,&GameLoader::threadFinished,Qt::QueuedConnection);
        connect(t,&GameLoaderThread::addSize,this,&GameLoader::addSize,Qt::QueuedConnection);
        this->threads.insert(t);
        threads.push_back(t);
        index++;
    }

    index=0;
    sizeToProcess=0;
    sizeProcessed=0;
    #ifndef NOAUDIO
    {
        QDirIterator it(":/CC/music/", QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString file=it.next();
            if(file.endsWith(QStringLiteral(".opus")))
            {
                if(tc<2)
                    threads.at(0)->toLoad.push_back(file);
                else
                    threads.at(index%threads.size())->toLoad.push_back(file);
                index++;
                sizeToProcess+=QFileInfo(file).size();
            }
        }
    }
    #endif
    {
        QDirIterator it(":/CC/images/", QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString file=it.next();
            if(
                    file.endsWith(QStringLiteral(".png")) ||
                    file.endsWith(QStringLiteral(".jpg")) ||
                    file.endsWith(QStringLiteral(".webp"))
                    )
            {
                if(tc<2)
                    threads.at(0)->toLoad.push_back(file);
                else
                    threads.at(index%threads.size())->toLoad.push_back(file);
                index++;
                sizeToProcess+=QFileInfo(file).size();
            }
        }
    }

    {
        unsigned int index=0;
        while(index<threads.size())
        {
            threads.at(index)->start(QThread::LowestPriority);
            index++;
        }
    }
}

void GameLoader::addSize(uint32_t size)
{
    sizeProcessed+=size;
    if(sizeProcessed<=sizeToProcess)
        emit progression(sizeProcessed,sizeToProcess);
    else
        abort();
}

void GameLoader::threadFinished()
{
    GameLoaderThread *thread = qobject_cast<GameLoaderThread *>(sender());
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
    //QImage to QPixmap
    QTime myTimer;
    myTimer.start();
    {
        QHashIterator<QString,QImage *> i(thread->images);
        while (i.hasNext()) {
            i.next();
            QImage * image=i.value();
            images[i.key()]=image;
            //QCoreApplication::processEvents();
        }
    }
    int myTimerElapsed=myTimer.elapsed();
    QTime myTimer2;
    myTimer2.start();
    #ifndef NOAUDIO
    //merge the music
    {
        QHashIterator<QString,QByteArray *> i(thread->musics);
        while (i.hasNext()) {
            i.next();
            musics[i.key()]=i.value();
        }
    }
    #endif
    int myTimer2Elapsed=myTimer2.elapsed();
    //clean the old work
    /*not need, no event loop: thread->exit();
    thread->quit();
    thread->terminate();*/
    thread->deleteLater();
    if(threads.empty())
    {
        //emit progression(sizeToProcess,sizeToProcess);
        emit dataIsParsed();

        std::cout << "time audio: " << GameLoaderThread::audio << "ms" << std::endl;
        std::cout << "time image: " << GameLoaderThread::image << "ms" << std::endl;
        std::cout << "QImage to QPixmap: " << myTimerElapsed << "ms" << std::endl;
        std::cout << "merge the music: " << myTimer2Elapsed << "ms" << std::endl;
    }
}

const QPixmap *GameLoader::getImage(const QString &path)
{
    if(pixmaps.contains(path))
        return pixmaps.value(path);
    else
    {
        if(images.contains(path))
        {
            QPixmap *p=new QPixmap(
                        QPixmap::fromImage(
                            *(images.value(path))
                            )
                        );
            pixmaps[path]=p;
            images.remove(path);
            return pixmaps.value(path);
        }
    }
    std::cerr << "GameLoader::getImage failed on: " << path.toStdString() << std::endl;
    QPixmap *p=new QPixmap(path);
    pixmaps[path]=p;
    return p;
/*    abort();
    return nullptr;*/
}
