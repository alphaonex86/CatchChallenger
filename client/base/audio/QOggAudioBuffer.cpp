#include "QOggAudioBuffer.h"
#include <QByteArray>
#include <QDebug>

QOggAudioBuffer::QOggAudioBuffer(QObject *parent) :
    QBuffer(parent)
{
}

qint64 QOggAudioBuffer::readData(char * rawData, qint64 len)
{
    QMutexLocker mutexLocker(&mutex);
    qint64 size=QBuffer::readData(rawData,len);
    if(size>0)
    {
        setData(buffer().mid(size));
        QBuffer::seek(0);
    }
    emit readDone();
    return size;
}

qint64 QOggAudioBuffer::writeData(const char *data, qint64 len)
{
    QMutexLocker mutexLocker(&mutex);
    return QBuffer::writeData(data,len);
}

bool QOggAudioBuffer::seek(qint64 off)
{
    QMutexLocker mutexLocker(&mutex);
    return QBuffer::seek(off);
}

void QOggAudioBuffer::close()
{
    QMutexLocker mutexLocker(&mutex);
    QBuffer::close();
}

void QOggAudioBuffer::clearData()
{
    QMutexLocker mutexLocker(&mutex);
    QBuffer::seek(0);
    QBuffer::setData(QByteArray());
}

bool QOggAudioBuffer::open(OpenMode openMode)
{
    QMutexLocker mutexLocker(&mutex);
    return QBuffer::open(openMode);
}

int QOggAudioBuffer::bufferUsage()
{
    QMutexLocker mutexLocker(&mutex);
    return data().size();
}
