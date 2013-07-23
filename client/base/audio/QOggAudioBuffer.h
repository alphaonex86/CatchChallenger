#ifndef QOGGAUDIOBUFFER_H
#define QOGGAUDIOBUFFER_H

#include <QBuffer>

class QOggAudioBuffer : public QBuffer
{
    Q_OBJECT
public:
    explicit QOggAudioBuffer(QObject *parent = 0);
    virtual qint64 readData(char * rawData,qint64 len);
signals:
    void readDone() const;
};

#endif // QOGGAUDIOBUFFER_H
