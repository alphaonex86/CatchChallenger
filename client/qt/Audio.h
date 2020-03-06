#ifndef Audio_H
#define Audio_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../opusfile/opusfile.h"
#include "QInfiniteBuffer.h"

#include <QString>
#include <QAudioOutput>
#include <QList>
#include <QBuffer>

class Audio
{
public:
    Audio();
    ~Audio();
    static Audio audio;
    void setVolume(const int &volume);
    QStringList output_list();
    void setPlayerVolume(QAudioOutput * const player);
    QAudioFormat format() const;
    static bool decodeOpus(const std::string &filePath,QByteArray &data);

    //if already playing ambiance then call stopCurrentAmbiance
    bool startAmbiance(const std::string &soundPath);
    void stopCurrentAmbiance();
protected:
    void addPlayer(QAudioOutput * const player);
    void removePlayer(QAudioOutput * const player);
private:
    std::vector<QAudioOutput *> playerList;
    QAudioFormat m_format;
protected:
    int volume;
protected:
    QAudioOutput *ambiance_player;
    QInfiniteBuffer *ambiance_buffer;
    QByteArray ambiance_data;
};
#endif

#endif // RSSNEWS_H
