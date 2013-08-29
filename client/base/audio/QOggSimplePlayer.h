#ifndef QOGGSIMPLEPLAYER_H
#define QOGGSIMPLEPLAYER_H

#include <QObject>

#include <QAudioOutput>
#include <QBuffer>
#include <QMutex>
#include <QMutexLocker>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "QOggAudioBuffer.h"

class QOggSimplePlayer : public QObject
{
    Q_OBJECT
public:
    explicit QOggSimplePlayer(const QString &filePath, QThread * audioThread=NULL);
    virtual ~QOggSimplePlayer();
    QString getFilePath() const;
    void setVolume(qreal volume);
public slots:
    void start();
    void stop();
    void setLoop(const bool &loop);
private slots:
    void finishedPlaying(QAudio::State state);
    void readDone();
    void close();
    void open();
    void internalStop();
    void internalStart();
private:
    qreal volume;
    QAudioOutput *output;
    QOggAudioBuffer buffer;
    OggVorbis_File vf;
    int current_section;
    QString filePath;
    bool needPlay;
    bool loop;
    QMutex mutex;
signals:
    void internalOpen();
    void internalClose();
    void emitInternalStop();
    void emitInternalStart();
};

#endif // QOGGSIMPLEPLAYER_H
