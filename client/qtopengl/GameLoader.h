#ifndef GAMELOADER_H
#define GAMELOADER_H

#include <QThread>
#include <unordered_set>
#include <vector>
#include <QPixmap>
#include "GameLoaderThread.h"

class GameLoader : public QObject
{
    Q_OBJECT
public:
    explicit GameLoader();
    static GameLoader *gameLoader;

    #ifndef NOAUDIO
    QHash<QString,QByteArray> musics;
    #endif
    QHash<QString,QPixmap> images;
private:
    void threadFinished();

    void addSize(uint32_t size);
    std::unordered_set<GameLoaderThread *> threads;
    uint64_t sizeToProcess;
    uint64_t sizeProcessed;
signals:
    void dataIsParsed();
    void progression(uint32_t size,uint32_t total);
};

#endif // GAMELOADER_H
