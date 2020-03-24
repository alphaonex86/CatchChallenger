#ifndef QINFINITEBUFFER_H
#define QINFINITEBUFFER_H

#include <QBuffer>

class QInfiniteBuffer : public QBuffer
{
public:
    QInfiniteBuffer(QByteArray *buf, QObject *parent = nullptr);
    QInfiniteBuffer(QObject *parent = nullptr);
    qint64 readData(char *output, qint64 maxlen) override;
private:
    qint64 m_pos;
};

#endif // QINFINITEBUFFER_H
