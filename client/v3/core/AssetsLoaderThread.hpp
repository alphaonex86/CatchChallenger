// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_ASSETSLOADERTHREAD_HPP_
#define CLIENT_QTOPENGL_CORE_ASSETSLOADERTHREAD_HPP_

#include <QHash>
#include <QString>
#include <QThread>
#include <unordered_set>
#include <vector>
#ifndef NOAUDIO
  #include <QByteArray>
#endif
#include <QImage>

class AssetsLoaderThread : public QThread {
  Q_OBJECT

 public:
  AssetsLoaderThread(uint32_t index);
  void run() override;

  std::vector<QString> toLoad;
#ifndef NOAUDIO
  QHash<QString, QByteArray *> musics;
#endif
  QHash<QString, QImage *> images;

  static uint32_t audio;
  static uint32_t image;

 private:
  uint32_t m_index;
 signals:
  void AddSize(uint32_t size);
};

#endif  // CLIENT_QTOPENGL_CORE_ASSETSLOADERTHREAD_HPP_
