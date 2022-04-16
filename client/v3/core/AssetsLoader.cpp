// Copyright 2021 CatchChallenger
#include "AssetsLoader.hpp"

#include <QCoreApplication>
#include <QDirIterator>
#include <QTime>
#include <iostream>

AssetsLoader *AssetsLoader::instance_ = nullptr;

AssetsLoader::AssetsLoader() {
  is_loaded_ = false;
  const int tc = QThread::idealThreadCount();
  threads_.clear();
  std::vector<AssetsLoaderThread *> threads;
  int index = 0;
  while (index < tc || index == 0) {  // always create 1 thread
    AssetsLoaderThread *t = new AssetsLoaderThread(index);
    connect(t, &QThread::finished, this, &AssetsLoader::ThreadFinished,
            Qt::QueuedConnection);
    connect(t, &AssetsLoaderThread::AddSize, this, &AssetsLoader::AddSize,
            Qt::QueuedConnection);
    threads_.insert(t);
    threads.push_back(t);
    index++;
  }

  index = 0;
  sizeToProcess = 0;
  sizeProcessed = 0;
#ifndef NOAUDIO
  {
    QDirIterator it(":/CC/music/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QString file = it.next();
      if (file.endsWith(QStringLiteral(".opus"))) {
        if (tc < 2)
          threads.at(0)->toLoad.push_back(file);
        else
          threads.at(index % threads.size())->toLoad.push_back(file);
        index++;
        sizeToProcess += QFileInfo(file).size();
      }
    }
  }
#endif
  {
    QDirIterator it(":/CC/images/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QString file = it.next();
      if (file.endsWith(QStringLiteral(".png")) ||
          file.endsWith(QStringLiteral(".jpg")) ||
          file.endsWith(QStringLiteral(".webp"))) {
        if (tc < 2)
          threads.at(0)->toLoad.push_back(file);
        else
          threads.at(index % threads.size())->toLoad.push_back(file);
        index++;
        sizeToProcess += QFileInfo(file).size();
      }
    }
  }

  {
    unsigned int index = 0;
    while (index < threads.size()) {
      threads.at(index)->start(QThread::LowestPriority);
      index++;
    }
  }
}

AssetsLoader *AssetsLoader::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new AssetsLoader();
  }
  return instance_;
}

void AssetsLoader::AddSize(uint32_t size) {
  sizeProcessed += size;
  if (sizeProcessed <= sizeToProcess)
    NotifyProgress(sizeProcessed, sizeToProcess);
  else
    abort();
}

void AssetsLoader::ThreadFinished() {
  AssetsLoaderThread *thread = qobject_cast<AssetsLoaderThread *>(sender());
  if (thread == nullptr) abort();
  if (threads_.find(thread) != threads_.cend()) {
    threads_.erase(thread);
  } else {
    std::cerr << "AssetsLoader::threadFinished() thread not found" << std::endl;
    abort();
    return;
  }
  // QImage to QPixmap
  QTime myTimer;
  myTimer.start();
  {
    QHashIterator<QString, QImage *> i(thread->images);
    while (i.hasNext()) {
      i.next();
      QImage *image = i.value();
      images_[i.key()] = image;
      // QCoreApplication::processEvents();
    }
  }
  int myTimerElapsed = myTimer.elapsed();
  QTime myTimer2;
  myTimer2.start();
#ifndef NOAUDIO
  // merge the music
  {
    QHashIterator<QString, QByteArray *> i(thread->musics);
    while (i.hasNext()) {
      i.next();
      musics[i.key()] = i.value();
    }
  }
#endif

  int myTimer2Elapsed = myTimer2.elapsed();
  // clean the old work
  /*not need, no event loop: thread->exit();
  thread->quit();
  thread->terminate();*/
  thread->deleteLater();
  if (threads_.empty()) {
    is_loaded_ = true;
    on_data_parsed_();

    std::cout << "time audio: " << AssetsLoaderThread::audio << "ms"
              << std::endl;
    std::cout << "time image: " << AssetsLoaderThread::image << "ms"
              << std::endl;
    std::cout << "QImage to QPixmap: " << myTimerElapsed << "ms" << std::endl;
    std::cout << "merge the music: " << myTimer2Elapsed << "ms" << std::endl;
  }
}

const QPixmap *AssetsLoader::GetImage(const QString &path) {
  if (pixmaps_.contains(path)) {
    return pixmaps_.value(path);
  } else {
    if (images_.contains(path)) {
      QPixmap *p = new QPixmap(QPixmap::fromImage(*(images_.value(path))));
      pixmaps_[path] = p;
      images_.remove(path);
      return pixmaps_.value(path);
    }
  }
  std::cerr << "AssetsLoader::getImage failed on: " << path.toStdString()
            << std::endl;
  QPixmap *p = new QPixmap(path);
  pixmaps_[path] = p;
  return p;
}

void AssetsLoader::SetOnDataParsed(std::function<void()> callback) {
  on_data_parsed_ = callback;
}

bool AssetsLoader::IsLoaded() {
#ifndef NOTHREADS
  return is_loaded_;
#endif
  return true; 
}
