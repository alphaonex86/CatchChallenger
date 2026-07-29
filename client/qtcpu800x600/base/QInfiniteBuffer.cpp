#include "QInfiniteBuffer.h"

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
    //nothing to loop on: without this the do/while below copies 0 byte per turn
    //and never reaches maxlen (the client hangs when the ambiance file is missing)
    if(d.isEmpty())
        return 0;

    do
    {
        qint64 sizetocopy=maxlen-outputpos;
        if((maxlen-outputpos)>(d.size()-m_pos))
            sizetocopy=d.size()-m_pos;
        //+outputpos: each wrap-around turn APPENDS, else it overwrote the bytes
        //copied by the previous turn and the loop end returned uninitialised data
        memcpy(output+outputpos,d.constData()+m_pos,sizetocopy);
        outputpos+=sizetocopy;
        m_pos+=sizetocopy;
        if(m_pos>=d.size())
            m_pos=0;
    } while(outputpos<maxlen);

    return maxlen;
}
