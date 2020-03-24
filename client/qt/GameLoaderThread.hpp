#ifndef GAMELOADERTHREAD_H
#define GAMELOADERTHREAD_H

#include <QThread>
#include <unordered_set>
#include <vector>
#include <QHash>
#include <QString>
#ifndef NOAUDIO
#include <QByteArray>
#endif
#include <QImage>

class GameLoaderThread : public QThread
{
    Q_OBJECT
public:
    GameLoaderThread();
    void run() override;

    std::vector<QString> toLoad;
    #ifndef NOAUDIO
    QHash<QString,QByteArray *> musics;
    #endif
    QHash<QString,QImage *> images;

    static uint32_t audio;
    static uint32_t image;
signals:
    void addSize(uint32_t size);
};

#endif // GAMELOADER_H
