#include "QOggAudioBuffer.h"
#include <QByteArray>
#include <QDebug>

QOggAudioBuffer::QOggAudioBuffer(QObject *parent) :
    QBuffer(parent)
{
}

qint64 QOggAudioBuffer::readData(char * rawData, qint64 len)
{
    qint64 size=QBuffer::readData(rawData,len);
    if(size>0)
    {
        //qint64 position=pos();
        close();
        setData(buffer().mid(size));
        open(QIODevice::ReadWrite|QIODevice::Unbuffered);
        //seek(position);
        seek(0);
    }
    emit readDone();
    return size;
}
