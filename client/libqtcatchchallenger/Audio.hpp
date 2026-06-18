#ifndef Audio_H
#define Audio_H

#include <opusfile.h>
#include "../../general/base/lib.h"
#include "QInfiniteBuffer.hpp"

#include <QString>
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
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
    void setPlayerVolume(QAudioSink * const player);
    QAudioFormat format() const;
    static bool decodeOpus(const std::string &filePath,QByteArray &data);
    // Decode a standard GSF MiniGSF (+ its sibling .gsflib via the _lib tag) with
    // the software MP2k synth in general/mp2k — NO GBA CPU emulation: we read the
    // gsflib's ROM image, the gSongTable address (from our _cc_songtable tag) and
    // the song id, and synthesise the same 48 kHz stereo Int16 PCM the opus path
    // yields, so the ambiance loop/sink machinery is reused unchanged.
    static bool decodeGsf(const std::string &minigsfPath,QByteArray &data);
    // Dispatch by extension: .minigsf -> decodeGsf, else decodeOpus.
    static bool decodeAmbiance(const std::string &filePath,QByteArray &data);

    //if already playing ambiance then call stopCurrentAmbiance
    virtual std::string startAmbiance(const std::string &soundPath);
    virtual void stopCurrentAmbiance();
public:
    void addPlayer(QAudioSink * const player);
    void removePlayer(QAudioSink * const player);
private:
    std::vector<QAudioSink *> playerList;
protected:
    QAudioFormat m_format;
    int volume;
protected:
    QAudioSink *ambiance_player;
    QInfiniteBuffer *ambiance_buffer;
    QByteArray ambiance_data;
};

#endif // RSSNEWS_H
