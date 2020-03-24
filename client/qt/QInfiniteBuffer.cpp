#include "QInfiniteBuffer.hpp"

QInfiniteBuffer::QInfiniteBuffer(QByteArray *buf, QObject *parent) :
    QBuffer(buf,parent)
{
    m_pos=0;
}

QInfiniteBuffer::QInfiniteBuffer(QObject *parent) :
    QBuffer(parent)
{
    m_pos=0;
}

qint64 QInfiniteBuffer::readData(char *output, qint64 maxlen)
{
    qint64 outputpos=0;
    const QByteArray &d=data();

    do
    {
        qint64 sizetocopy=maxlen-outputpos;
        if((maxlen-outputpos)>(d.size()-m_pos))
            sizetocopy=d.size()-m_pos;
        memcpy(output,d.constData()+m_pos,sizetocopy);
        outputpos+=sizetocopy;
        m_pos+=sizetocopy;
        if(m_pos>=d.size())
            m_pos=0;
    } while(outputpos<maxlen);

    return maxlen;
}
