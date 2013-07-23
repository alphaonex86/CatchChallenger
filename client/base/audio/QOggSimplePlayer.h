#ifndef QOGGSIMPLEPLAYER_H
#define QOGGSIMPLEPLAYER_H

#include <QObject>

#include <QAudioOutput>
#include <QBuffer>

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
public slots:
    void start();
    void stop();
private slots:
    void finishedPlaying(QAudio::State state);
    void readDone();
    void close();
    void open();
private:
    QAudioOutput *output;
    QOggAudioBuffer buffer;
    FILE * file;
    OggVorbis_File vf;
    int current_section;
    QString filePath;
    bool needPlay;
signals:
    void internalOpen();
    void internalClose();
};

#endif // QOGGSIMPLEPLAYER_H
