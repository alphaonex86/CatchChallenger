#ifndef QOGGAUDIOBUFFER_H
#define QOGGAUDIOBUFFER_H

#include <QBuffer>
#include <QMutex>
#include <QMutexLocker>

class QOggAudioBuffer : public QBuffer
{
    Q_OBJECT
public:
    explicit QOggAudioBuffer(QObject *parent = 0);
    virtual qint64 readData(char * rawData,qint64 len);
    qint64 writeData(const char *data, qint64 len);
    bool seek(qint64 off);
    void close();
    void clearData();
    bool open(OpenMode openMode);
    int bufferUsage();
private:
    QMutex mutex;
signals:
    void readDone() const;
};

#endif // QOGGAUDIOBUFFER_H
