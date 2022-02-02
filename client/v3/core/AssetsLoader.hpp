// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_ASSETSLOADER_HPP_
#define CLIENT_QTOPENGL_CORE_ASSETSLOADER_HPP_

#include <QPixmap>
#include <QThread>
#include <functional>
#include <unordered_set>
#include <vector>

#include "AssetsLoaderThread.hpp"
#include "ProgressNotifier.hpp"

/* only load here internal ressources, NOT LOAD here datapack ressources
Load datapack ressources into another object, async and ondemand */
class AssetsLoader : public QObject, public ProgressNotifier {
  Q_OBJECT

 public:
  static AssetsLoader *GetInstance();

  void SetOnDataParsed(std::function<void()> callback);
  bool IsLoaded();

#ifndef NOAUDIO
  // only load here internal small musics, NOT LOAD here datapack musics
  QHash<QString, const QByteArray *> musics;
#endif
  const QPixmap *GetImage(const QString &path);

 protected:
  // only load here internal images, NOT LOAD here datapack images
  QHash<QString, QPixmap *> pixmaps_;
  QHash<QString, QImage *> images_;

 private:
  AssetsLoader();
  void ThreadFinished();

  void AddSize(uint32_t size);
  std::unordered_set<AssetsLoaderThread *> threads_;
  uint64_t sizeToProcess;
  uint64_t sizeProcessed;
  static AssetsLoader *instance_;
  std::function<void()> on_data_parsed_;
  bool is_loaded_;
};

#endif  // CLIENT_QTOPENGL_CORE_ASSETSLOADER_HPP_
