#ifndef GAMELOADER_H
#define GAMELOADER_H

#include <QThread>
#include <unordered_set>
#include <vector>
#include <QPixmap>
#include "GameLoaderThread.hpp"

/* only load here internal ressources, NOT LOAD here datapack ressources
Load datapack ressources into another object, async and ondemand */
class GameLoader : public QObject
{
    Q_OBJECT
public:
    explicit GameLoader();
    static GameLoader *gameLoader;

    #ifndef NOAUDIO
    //only load here internal small musics, NOT LOAD here datapack musics
    QHash<QString,const QByteArray *> musics;
    #endif
    const QPixmap *getImage(const QString &path);
protected:
    //only load here internal images, NOT LOAD here datapack images
    QHash<QString,QPixmap *> pixmaps;
    QHash<QString,QImage *> images;
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
