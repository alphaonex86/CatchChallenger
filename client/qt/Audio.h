#ifndef Audio_H
#define Audio_H

#ifndef CATCHCHALLENGER_NOAUDIO
#include "../opusfile/opusfile.h"

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
    void addPlayer(QAudioOutput * const player);
    void removePlayer(QAudioOutput * const player);
    void setPlayerVolume(QAudioOutput * const player);
    QAudioFormat format() const;
    static bool decodeOpus(const std::string &filePath,QByteArray &data);
private:
    std::vector<QAudioOutput *> playerList;
    int volume;
    QAudioFormat m_format;
};
#endif

#endif // RSSNEWS_H
