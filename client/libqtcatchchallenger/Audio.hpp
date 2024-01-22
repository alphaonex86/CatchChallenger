#ifndef Audio_H
#define Audio_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include <opusfile.h>
#include "../../general/base/lib.h"
#include "QInfiniteBuffer.hpp"

#include <QString>
#include <QAudioOutput>
#include <QList>
#include <QBuffer>
#include <QAudioFormat>

class DLL_PUBLIC Audio
{
public:
    Audio();
    ~Audio();
    static Audio *audio;
    void setVolume(const int &volume);
    QStringList output_list();
    void setPlayerVolume(QAudioOutput * const player);
    QAudioFormat format() const;
    static bool decodeOpus(const std::string &filePath,QByteArray &data);

    //if already playing ambiance then call stopCurrentAmbiance
    virtual std::string startAmbiance(const std::string &soundPath);
    virtual void stopCurrentAmbiance();
public:
    void addPlayer(QAudioOutput * const player);
    void removePlayer(QAudioOutput * const player);
private:
    std::vector<QAudioOutput *> playerList;
protected:
    QAudioFormat m_format;
    int volume;
protected:
    QAudioOutput *ambiance_player;
    QInfiniteBuffer *ambiance_buffer;
    QByteArray ambiance_data;
};
#endif

#endif // RSSNEWS_H
